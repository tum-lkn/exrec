# %%
import collections

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

import config

# %%
MILESTONE = "scenario2_8tors"
BASE_OUT_PATH = f"./plots/{MILESTONE}"

LOAD_TO_MARKER = {
    0.3: 'v',
    0.4: '>',
    0.5: '*',
    0.7: 's'
}

LOAD_TO_LINESTYLE = {
    0.3: '-',
    0.4: ':',
    0.5: '-.',
    0.7: '--'
}

CASE_TO_COLOR = {
    config.CASE_2DO: (0.9, 0, 0),
    config.CASE_3DO: (0, 0.74, 0.03),
    config.CASE_2DO_1DA: (0, 0, 1),
}

LOADS = [0.3, 0.4, 0.5, 0.7]

# %%
data = pd.read_hdf(
    f"{config.BASE_DATA_PATH}/{MILESTONE}/aggregated_flow_data.h5",
    key="agg_flow_data"
).set_index(["case", "run"], inplace=False)

# %%

case_to_text = {
    config.CASE_2DO: '2 \\textsc{DO}',
    config.CASE_2DO_1DA: '2 \\textsc{DO}, 1 \\textsc{DA}',
    config.CASE_3DO: '3 \\textsc{DO}',
}

# %% Plot Demand Completion Time

ref_case = config.CASE_3DO
plot_means = collections.defaultdict(list)
plot_stds = collections.defaultdict(list)

plt.figure()
for nrotor in [config.CASE_2DO, config.CASE_3DO, config.CASE_2DO_1DA]:
    for load in [0.3, 0.4, 0.5, 0.7]:
        values = list()
        num_flows = list()
        case = f"{MILESTONE}_{nrotor}_{load}"
        if case not in data.index.levels[0]:
            plot_stds[nrotor].append(0)
            plot_means[nrotor].append(0)
            continue
        for run in np.unique(data.loc[case].index.values):
            this_data = data.loc[case, run]
            this_data_dst = this_data[(this_data["host"] == this_data["dst"]) & (this_data["complete conn"] == "yes")]
            this_data_src = this_data[(this_data["host"] == this_data["src"]) & (this_data["complete conn"] == "yes")]
            values.append(
                this_data_dst["last packet"].max() - this_data_src["first packet"].min()
            )
            num_flows.append(len(this_data_dst))
        print(case, np.mean(values), np.std(values), num_flows)
        plot_means[nrotor].append(np.mean(values))
        plot_stds[nrotor].append(np.std(values))

for nrotor in [config.CASE_2DO, config.CASE_3DO, config.CASE_2DO_1DA]:
    plt.errorbar(
        range(4),
        np.array(plot_means[nrotor]) / np.array(plot_means[ref_case]),
        yerr=np.array(plot_stds[nrotor]) / np.array(plot_means[ref_case]),
        marker=config.case_to_marker[nrotor],
        markeredgecolor='k', markerfacecolor='w', markeredgewidth=0.6, markersize=3,
        label=config.case_to_text[nrotor].replace('otor', '').replace(", ", ","),
        color=CASE_TO_COLOR[nrotor],
        linestyle=''
    )

plt.xticks(range(4), ["{:2.2f}".format(x / 0.7) for x in [0.3, 0.4, 0.5, 0.7]])
plt.ylabel("DCT (normed)", labelpad=-0.6)
plt.xlabel("Normalized Load", labelpad=-0.6)
plt.legend(frameon=False, loc="lower left",
           ncol=3, columnspacing=0.,
           handletextpad=0.0, bbox_to_anchor=(-0.5, 1.02), borderpad=0)
plt.subplots_adjust(left=0.23, bottom=0.23, right=0.9, top=0.85)
plt.savefig(f"{BASE_OUT_PATH}/ebar_dct_load_{MILESTONE}.pdf")
plt.show()

# %% Linestyles
LINESTYLE_PER_CASE = {
    config.CASE_2DO: "-",
    config.CASE_3DO: "--",
    config.CASE_2DO_1DA: "-."
}

# %% FCT all flows
for load in LOADS[:1]:
    plt.figure()
    for nrotor in [config.CASE_2DO, config.CASE_3DO, config.CASE_2DO_1DA]:
        case = f"{MILESTONE}_{nrotor}_{load}"
        fct_medium = list()
        fct_large = list()
        try:
            this_data = data.loc[case]
        except KeyError:
            continue
        for run in np.unique(data.loc[case].index.values):
            this_data_src = this_data[
                (this_data["host"] == this_data["src"]) & (this_data["dport"] < 60000) & (this_data["sport"] < 60000)
                ].set_index(["src", "dst", "sport", "dport"]).sort_index(inplace=False)
            this_data_dst = this_data[
                (this_data["host"] == this_data["dst"]) & (this_data["dport"] < 60000) & (this_data["sport"] < 60000)
                ].set_index(["src", "dst", "sport", "dport"]).sort_index(inplace=False)

            fct_medium.append(
                (this_data_dst["last packet"] - this_data_src["first packet"]).values
            )

            this_data_src = this_data[
                (this_data["host"] == this_data["src"]) & (this_data["dport"] >= 60000)
                ].set_index(["src", "dst", "sport", "dport"]).sort_index(inplace=False)
            this_data_dst = this_data[
                (this_data["host"] == this_data["dst"]) & (this_data["dport"] >= 60000)
                ].set_index(["src", "dst", "sport", "dport"]).sort_index(inplace=False)

            fct_large.append(
                (this_data_dst["last packet"] - this_data_src["first packet"]).values)
        fct_medium = np.concatenate(fct_medium)
        fct_large = np.concatenate(fct_large)
        plt.plot(
            sorted(fct_medium), np.arange(1.0 / len(fct_medium), 1.000001, 1.0 / len(fct_medium)),
            color=CASE_TO_COLOR[nrotor], linestyle=LINESTYLE_PER_CASE[nrotor],
            marker=config.case_to_marker[nrotor],
            markeredgecolor=CASE_TO_COLOR[nrotor],
            markerfacecolor='w',
            markevery=4000,
            markersize=4
        )
        plt.plot(
            sorted(fct_large), np.arange(1.0 / len(fct_large), 1.001, 1.0 / len(fct_large)),
            label=f"{case_to_text[nrotor]}",
            color=CASE_TO_COLOR[nrotor], linestyle=LINESTYLE_PER_CASE[nrotor],
            marker=config.case_to_marker[nrotor],
            markeredgecolor=CASE_TO_COLOR[nrotor],
            markerfacecolor='w',
            markevery=50,
            markersize=4
        )

    plt.axvline(x=0.04, linestyle=':', color='k')
    plt.text(x=-0.2, y=1.05, s="40ms")

    plt.axvline(x=1.2, linestyle=':', color='k')
    plt.text(x=1.0, y=1.05, s="1.2s")

    plt.text(x=0.5, y=-0.2, s="Small")
    plt.text(x=2.5, y=0.2, s="Large")
    plt.gca().arrow(0.85, -0.1, -0.15, 0.15, head_width=0.1, head_length=0.1, fc='k', ec='k', clip_on=False)

    leg_handles = plt.legend(
        loc="lower left",
        frameon=False,
        labelspacing=0.1,
        columnspacing=0.3,
        handletextpad=0.3,
        handlelength=1.2,
        borderpad=0, ncol=6, bbox_to_anchor=(-0.05, 1.05),
        framealpha=0.7
    )
    plt.ylabel("P(X$\leq$x)")
    plt.ylim(0, 1)
    plt.xlabel("FCT [s]", labelpad=-0.6)
    plt.subplots_adjust(left=0.21, bottom=0.22, right=0.99, top=0.85)
    plt.savefig(f"{BASE_OUT_PATH}/cdf_fct_all_split_{load}.pdf")
    plt.show()
