from preprocessing_functions import *
from config import *


# %%
FOLDER_2_DO = 'scenario1_8tors/scenario1_8tors_2do'
SUB_FOLDER_2_DO = '<choose run here>'
C2_DO = Case(config.BASE_DATA_PATH, FOLDER_2_DO, SUB_FOLDER_2_DO, isTcp=False)

FOLDER_1_DO_1_DA = 'scenario1_8tors/scenario1_8tors_1do_1da'
SUB_FOLDER_1_DO_1_DA = '<choose run here>'
C1_DO_1_DA = Case(config.BASE_DATA_PATH, FOLDER_1_DO_1_DA, SUB_FOLDER_1_DO_1_DA, isTcp=False)

FOLDER_1_DO = 'scenario1_8tors/scenario1_8tors_1do'
SUB_FOLDER_1_DO = '<choose run here>'
C1_DO = Case(config.BASE_DATA_PATH, FOLDER_1_DO, SUB_FOLDER_1_DO, isTcp=False)

FOLDER_1_DO_INDIRECT = 'scenario1_8tors/scenario1_8tors_1do_indirect'
SUB_FOLDER_1_DO_INDIRECT = '<choose run here>'
C1_DO_INDIRECT = Case(config.BASE_DATA_PATH, FOLDER_1_DO_INDIRECT, SUB_FOLDER_1_DO_INDIRECT, isTcp=False)

CASES = {
    config.CASE_1DO_1DA: C1_DO_1_DA,
    config.CASE_2DO: C2_DO,
    config.CASE_1DO: C1_DO,
    config.CASE_1DO_INDIRECT: C1_DO_INDIRECT
}


# %% Plot accumulated data
CASES[config.CASE_1DO].init(cutting_time=0.0001)
CASES[config.CASE_2DO].init(cutting_time=0.0001)
CASES[config.CASE_1DO_INDIRECT].init(cutting_time=0.0001)
CASES[config.CASE_1DO_1DA].init(cutting_time=0.0001)

# %%
idx_to_linestyle = {
    0: 'dotted',
    1: 'dashed'
}


# %%
plt.figure()

start_time = 6.399
stop_time = start_time + 0.06
resolution = 0.005
cycle_background_color = 'white'
reconf_background_color = 'dimgray'

x_factor = 10_000

case = config.CASE_1DO_1DA
fig, ax = plt.subplots()
case_str = case.replace('_', '\_')
for idx in [0, 1]:
    tmp_flow = CASES[case].flows[idx]
    plot_flow_over_time(
        tmp_flow, start_time, stop_time, resolution=resolution,
        color=UDP_FLOW_COLORS[case][idx],
        scaling=15,
        offset=6.401,
        xfactor=1_000,
        linestyle=idx_to_linestyle[idx]
    )
plt.xlabel("Time [ms]")
upper_ylim = 10

# Plot shaded areas
slots = list(range(
    int(start_time*x_factor), int(stop_time*x_factor), 50
))
for t_slot in slots:
    # Day
    ax.fill_between(
        [t_slot, t_slot + 45],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=cycle_background_color,
        alpha=0.4
    )

    plt.plot(
        [t_slot + 45, t_slot + 45],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )
    # Reconf
    ax.fill_between(
        [t_slot + 45, t_slot + 50],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=reconf_background_color,
        alpha=0.2
    )

    plt.plot(
        [t_slot + 50, t_slot + 50],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )

plt.xticks(
    slots+[max(slots)+50],
    [25+i*5 for i in range(len(slots)+1)]
)

ax.grid()
texty = 10.2
textx_shift = 10
for i in range(0,12):
    plt.text(start_time*x_factor+15-textx_shift+i*50, texty, f"M{ (i % 7) + 1}")

plt.ylim([0, upper_ylim])
plt.ylabel("Throughput [Gbps]", labelpad=2)
plt.xlabel("Time [ms]", labelpad=0.5)
plt.subplots_adjust(
    left=0.1, bottom=0.3, right=0.98, top=0.87
)
fname = case_str.replace('\_', '_')
plt.savefig(
    f"./plots/scenario1/8tors_{fname}.pdf"
)
plt.show()

# %% --- 2 DO
plt.figure()

start_time = 0.7055
stop_time = start_time + 0.06
resolution = 0.005
cycle_background_color = 'white'
reconf_background_color = 'dimgray'

x_factor = 10_000

slot_size = 0.0045 * x_factor
reconf_size = 0.0005 * x_factor

case = config.CASE_2DO
fig, ax = plt.subplots()
case_str = case.replace('_', '\_')
for idx in [0, 1]:
    tmp_flow = CASES[case].flows[idx]
    plot_flow_over_time(
        tmp_flow, start_time, stop_time, resolution=resolution,
        color=UDP_FLOW_COLORS[case][idx],
        scaling=15,
        offset=6.401,
        xfactor=1_000,
        linestyle=idx_to_linestyle[idx]
    )
plt.xlabel("Time [ms]")
upper_ylim = 10

# Plot shaded areas
slots = list(range(
    int(start_time*x_factor), int(stop_time*x_factor), int(slot_size+reconf_size)
))
for t_slot in slots:
    # Day
    ax.fill_between(
        [t_slot, t_slot + slot_size],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=cycle_background_color,
        alpha=0.4
    )

    plt.plot(
        [t_slot + slot_size, t_slot + slot_size],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )
    # Reconf
    ax.fill_between(
        [t_slot + slot_size, t_slot + slot_size + reconf_size],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=reconf_background_color,
        alpha=0.2
    )

    plt.plot(
        [t_slot + slot_size+reconf_size, t_slot + slot_size+reconf_size],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )

plt.xticks(
    slots+[max(slots)+slot_size+reconf_size],
    [25+i*5 for i in range(len(slots)+1)]
)

ax.grid()
texty = 10.2
textx_shift = 10
for i in range(0,12):
    plt.text(start_time*x_factor+15-textx_shift+i*50, texty+1.3, f"M{ (i % 7) + 1}")
    plt.text(start_time * x_factor + 15 - textx_shift + i * 50, texty, f"M{((i+3) % 7) + 1}")

plt.ylim([0, upper_ylim])
plt.ylabel("Throughput [Gbps]", labelpad=2)
plt.xlabel("Time [ms]", labelpad=0.5)
plt.subplots_adjust(
    left=0.1, bottom=0.28, right=0.98, top=0.83
)
fname = case_str.replace('\_', '_')
plt.savefig(
    f"./plots/scenario1/8tors_{fname}.pdf"
)
plt.show()

# %% --- 1 DO
plt.figure()

start_time = 6.4047
stop_time = start_time + 0.06
resolution = 0.005
cycle_background_color = 'white'
reconf_background_color = 'dimgray'

x_factor = 10_000

case = config.CASE_1DO
fig, ax = plt.subplots()
case_str = case.replace('_', '\_')
for idx in [0, 1]:
    tmp_flow = CASES[case].flows[idx]
    plot_flow_over_time(
        tmp_flow, start_time, stop_time, resolution=resolution,
        color=UDP_FLOW_COLORS[case][idx],
        scaling=15,
        offset=6.401,
        xfactor=1_000,
        linestyle=idx_to_linestyle[idx]
    )
plt.xlabel("Time [ms]")
upper_ylim = 10

# Plot shaded areas
slots = list(range(
    int(start_time*x_factor), int(stop_time*x_factor), 50
))
for t_slot in slots:
    # Day
    ax.fill_between(
        [t_slot, t_slot + 45],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=cycle_background_color,
        alpha=0.4
    )

    plt.plot(
        [t_slot + 45, t_slot + 45],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )
    # Reconf
    ax.fill_between(
        [t_slot + 45, t_slot + 50],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=reconf_background_color,
        alpha=0.2
    )

    plt.plot(
        [t_slot + 50, t_slot + 50],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )

plt.xticks(
    slots+[max(slots)+50],
    [25+i*5 for i in range(len(slots)+1)]
)

ax.grid()
texty = 10.2
textx_shift = 10
for i in range(0,12):
    plt.text(start_time*x_factor+15-textx_shift+i*50, texty, f"M{ (i % 7) + 1}")

plt.ylim([0, upper_ylim])
plt.ylabel("Throughput [Gbps]", labelpad=2)
plt.xlabel("Time [ms]", labelpad=0.5)
plt.subplots_adjust(
    left=0.1, bottom=0.3, right=0.98, top=0.87
)
fname = case_str.replace('\_', '_')
plt.savefig(
    f"./plots/scenario1/8tors_{fname}.pdf"
)
plt.show()

# %% --- 1 DO indirect
plt.figure()

start_time = 6.4123
stop_time = start_time + 0.06
resolution = 0.005
cycle_background_color = 'white'
reconf_background_color = 'dimgray'

x_factor = 10_000

case = config.CASE_1DO_INDIRECT

fig, ax = plt.subplots()
case_str = case.replace('_', '\_')
for idx in [0, 1]:
    tmp_flow = CASES[case].flows[idx]
    plot_flow_over_time(
        tmp_flow, start_time, stop_time, resolution=resolution,
        color=UDP_FLOW_COLORS[config.CASE_1DO][idx],
        scaling=15,
        offset=6.401,
        xfactor=1_000
    )
plt.xlabel("Time [ms]")
upper_ylim = 10

# Plot shaded areas
slots = list(range(
    int(start_time*x_factor), int(stop_time*x_factor), 50
))
for t_slot in slots:
    # Day
    ax.fill_between(
        [t_slot, t_slot + 45],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=cycle_background_color,
        alpha=0.4
    )

    plt.plot(
        [t_slot + 45, t_slot + 45],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )
    # Reconf
    ax.fill_between(
        [t_slot + 45, t_slot + 50],
        y1=[0, 0],
        y2=[upper_ylim, upper_ylim],
        facecolor=reconf_background_color,
        alpha=0.2
    )

    plt.plot(
        [t_slot + 50, t_slot + 50],
        [0, 1e4],
        color='black',
        linewidth=0.5
    )

plt.xticks(
    slots+[max(slots)+50],
    [25+i*5 for i in range(len(slots)+1)]
)

ax.grid()
texty = 10.2
textx_shift = 10
for i in range(0,12):
    plt.text(start_time*x_factor+15-textx_shift+i*50, texty, f"M{ (i % 7) + 1}")


plt.ylim([0, upper_ylim])
plt.ylabel("Throughput [Gbps]", labelpad=2)
plt.xlabel("Time [ms]", labelpad=0.5)
plt.subplots_adjust(
    left=0.1, bottom=0.3, right=0.98, top=0.87
)
fname = case_str.replace('\_', '_')
plt.savefig(
    f"./plots/scenario1/8tors_{fname}.pdf"
)
plt.show()
