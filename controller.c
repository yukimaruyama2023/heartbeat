#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define USE_MMAP

#ifdef USE_MMAP

#define MEM_FILE "/dev/mem"
#define FILENAME_DISKSTATS_ADDR "physical_address.txt"
#define FILENAME_STAT_ADDR "stat_phys_addr.txt"

#else

#define MEM_FILE "/dev/kmem"
#define FILENAME_DISKSTATS_ADDR "address.txt"
#define FILENAME_STAT_ADDR "stat_addr.txt"

#endif

#define BUF_SIZE 256
#define MSG_LEN 88
#define DEST_ADDR "192.168.23.223"
#define DEST_PORT 22224
#define RECV_ADDR "192.168.23.99"
#define RECV_PORT 22222
#define NCPUS 6
#define NSTATS 10
#define PAGE_MASK ~0xfff
#define PAGE_SIZE 4096

uint64_t *diskstat_pages[NCPUS];
uint64_t *stat_pages[NCPUS];

void read_addr(const char *filename, off_t *addr) {
    FILE *fp = fopen(filename, "r");
    for (int i = 0; i < NCPUS; ++i) {
        fscanf(fp, "%lx", &addr[i]);
    }
    fclose(fp);
}

void init_mmaps(int mem_fd, off_t *addr, uint64_t **pages_dst) {
    for (int i = 0; i < NCPUS; ++i) {
        pages_dst[i] = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_PRIVATE, mem_fd, addr[i] & PAGE_MASK);
        if (diskstat_pages[i] == (void *)-1) {
            perror("mmap");
            exit(1);
        }

        // printf("%p\n", diskstat_pages[i]);
    }
}

void ummaps() {
    for (int i = 0; i < NCPUS; ++i) {
        munmap(diskstat_pages[i], PAGE_SIZE);
        munmap(stat_pages[i], PAGE_SIZE);
    }
}

uint64_t read_cpuinfo(int kmem_fd, off_t *addr) {
    uint64_t write_nsec = 0;
    for (int i = 0; i < NCPUS; ++i) {
        if (lseek(kmem_fd, addr[i], SEEK_SET) == (off_t)-1) {
            perror("lseek");
            exit(1);
        }
        uint64_t val;
        ssize_t n;
        n = read(kmem_fd, &val, 8);
        if (n == (ssize_t)-1) {
            perror("read");
            exit(1);
        }
        if (n != 8) {
            fprintf(stderr, "n is %ld, not 8 (i = %d)\n", n, i);
            exit(1);
        }
        // printf("val[%d] = %lu\n", i, val);
        write_nsec += val;
    }
    return write_nsec;
}

uint64_t read_diskstat2(off_t *addr) {
    uint64_t write_nsec = 0;
    for (int i = 0; i < NCPUS; ++i) {
        write_nsec += diskstat_pages[i][(addr[i] & ~PAGE_MASK) / sizeof(uint64_t)];
    }
    return write_nsec;
}

void read_stat2(off_t *addr, uint64_t *stats_dst) {
    for (int i = 0; i < NSTATS; ++i) {
        stats_dst[i] = 0;
        for (int j = 0; j < NCPUS; ++j) {
            stats_dst[i] += stat_pages[j][(addr[i] & ~PAGE_MASK) / sizeof(uint64_t) + i];
        }
    }
}

void message_gen(char *msg, int kmem_fd, off_t *addr_diskstats, off_t *addr_stat) {
    static uint64_t old_write_nsec = 0;
    static struct timespec old_time;
    struct timespec now_time;

    if (old_write_nsec == 0) {
        timespec_get(&old_time, TIME_UTC);
        old_write_nsec = read_cpuinfo(kmem_fd, addr_diskstats);
    }

    uint64_t cpu_stats[NSTATS];
    timespec_get(&now_time, TIME_UTC);
#ifdef USE_MMAP
    uint64_t write_nsec = read_diskstat2(addr_diskstats);
    read_stat2(addr_stat, cpu_stats);
#else
    uint64_t write_nsec = read_cpuinfo(kmem_fd, addr_diskstats);
#endif
    long diff_time = (now_time.tv_sec - old_time.tv_sec) * 1000000000 + now_time.tv_nsec - old_time.tv_nsec;
    long diff_write_nsec = write_nsec - old_write_nsec;

    // double ret = 1.0 * diff_write_nsec / diff_time;

    old_time = now_time;
    old_write_nsec = write_nsec;

    // printf("%ld / %ld\n", write_nsec, diff_time);
    memcpy(msg, &write_nsec, sizeof(write_nsec));
    memcpy(msg + 8, cpu_stats, sizeof(cpu_stats));
}

int main() {
    struct sockaddr_in send_addr, recv_addr;
    int send_sd, recv_sd, kmem_fd;
    char msg[BUF_SIZE], buf[BUF_SIZE];
    off_t diskstat_addr[NCPUS];
    off_t stat_addr[NCPUS];

    // struct sched_param param;
    // int policy = SCHED_FIFO;
    // param.sched_priority = sched_get_priority_max(policy);
    // sched_setscheduler(0, policy, &param);

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    inet_aton(DEST_ADDR, &send_addr.sin_addr);
    send_addr.sin_port = htons(DEST_PORT);

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    inet_aton(RECV_ADDR, &recv_addr.sin_addr);
    recv_addr.sin_port = htons(RECV_PORT);

    if ((send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket send");
        exit(1);
    }

    if ((recv_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket recv");
        exit(1);
    }

    if (bind(recv_sd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (connect(send_sd, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    kmem_fd = open(MEM_FILE, O_RDONLY);
    if (kmem_fd < 0) {
        perror("open" MEM_FILE);
        exit(1);
    }
    read_addr(FILENAME_DISKSTATS_ADDR, diskstat_addr);
    read_addr(FILENAME_STAT_ADDR, stat_addr);
#ifdef USE_MMAP
    init_mmaps(kmem_fd, diskstat_addr, diskstat_pages);
    init_mmaps(kmem_fd, stat_addr, stat_pages);
#endif
    int val = 1;
    ioctl(recv_sd, FIONBIO, &val);

    while (1) {
        message_gen(msg, kmem_fd, diskstat_addr, stat_addr);
        // wait for the request
        if (recv(recv_sd, buf, BUF_SIZE, 0) < 0) {
            if (errno == EAGAIN) continue;
            perror("recv");
            exit(1);
        }
        // double val;
        // memcpy(&val, msg, 8);
        //  printf("%lf\n", val);
        //  printf("received: %s\n", msg);
        if (send(send_sd, msg, MSG_LEN, 0) < 0) {
            perror("send");
            exit(1);
        }
    }
#ifdef USE_MMAP
    ummaps();
#endif
    close(recv_sd);
    close(send_sd);
    close(kmem_fd);

    return 0;
}
