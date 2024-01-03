import numpy as np
import matplotlib.pyplot as plt
import sys

METRIC_NAME = "system"
METRICS_UNIT = "ms"

CPU_METRIC_COL_BASE = 4
CPU_METRIC_COL_NUM = {
    "user": 0,
    "nice": 1,
    "system": 2,
    "softirq": 3,
    "irq": 4,
    "idle": 5,
    "iowait": 6,
    "steal": 7,
    "guest": 8,
    "guest nice": 9
}


def cpu_metric_tuple(metric_name):
    return (CPU_METRIC_COL_BASE + CPU_METRIC_COL_NUM[metric_name], f"{metric_name} CPU time [{METRICS_UNIT}]", f"CPU time ({metric_name})")


metric_column = 3
metric_label = "time spent on disk write [ms]"
metric_legend = "disk write"
metric_column = 14
metric_label = "RAM free pages"
metric_legend = "RAM free pages"
metric_column, metric_label, metric_legend = cpu_metric_tuple(METRIC_NAME)

# load data from csv file
data = np.loadtxt(sys.argv[1], delimiter=",",
                  skiprows=0, usecols=(1, 2, metric_column))
data = np.transpose(data)

TIMESEC_COL = 0
LATENCY_COL = 1
VAL_COL = 2

print(f"load: {max(data[VAL_COL])}, latency: {max(data[LATENCY_COL])}")
plt.rcParams["font.size"] = 23

# generate graph area
# ax1 for the load, ax2 for the latency
fig, ax1 = plt.subplots()
ax2 = plt.twinx(ax1)

# draw graph
ax1.plot(data[TIMESEC_COL], data[VAL_COL],
         label=metric_legend, marker="o")
ax2.plot(data[TIMESEC_COL], data[LATENCY_COL], label="latency of sending\nmessage",
        color="orange", marker="^")

# ax2.hlines(9.91, 0, 60, "red", "dashed")

# set title and labels
# plt.title("load and latency (monitor runnig on a dedicated core)")
ax1.set_xlabel("elapsed time [s]")
ax1.set_ylabel(metric_label)
ax2.set_ylabel("latency [ms]")
ax1.yaxis.set_label_position('right') 
ax1.yaxis.set_ticks_position("right")
ax2.yaxis.set_label_position('left') 
ax2.yaxis.set_ticks_position("left")

# ax2.set_ylim(0, 20000000)

# range
# ax1.set_ylim(2000000, 4200000)
# ax1.set_ylim(0, 200)
# ax1.set_xlim(-1, 52)
# ax1.set_xlim(30, 36)
#ax2.set_ylim(0, 30)
# ax2.set_xlim(-1, 52)
# ax2.set_xlim(30, 36)

#ax2.vlines(46.62, 0, 20, "red", "dashed", linewidth=1.5)
#ax2.vlines(81.52, 0, 20, "red", "dashed", linewidth=1.5)

# grid
plt.grid()

# legend
handler1, label1 = ax1.get_legend_handles_labels()
handler2, label2 = ax2.get_legend_handles_labels()
ax1.legend(handler2 + handler1, label2 + label1, loc="upper left")

# title
plt.title(sys.argv[1])

plt.show()
