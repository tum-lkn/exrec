import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy.stats as ss

import config

# %%
map_case = lambda x: f"ml_workload_gpu_{x}"

cases = [
    config.CASE_3DO,
    config.CASE_2DO_1DA,
    config.CASE_1DO_2DA,
    config.CASE_3DA
]


models = [config.DENSENET121, config.RESNET50, config.VGG16, config.VGG19]

# %% Prepare data
df_step_times = pd.read_hdf(
   f"{config.BASE_DATA_PATH}/ml_workload_step_times_stats.h5"
)

# %%
def get_data_num_runs(df, num_runs):
    unique_runs = df.Subdir.unique()
    indices = None

    for i in range(num_runs):
        this_subdir = unique_runs[i]
        if indices is None:
            indices = (df.Subdir == this_subdir)
        else:
            indices = indices | (df.Subdir == this_subdir)
    return df[indices].groupby(["Subdir", "Step"]).mean()

# %%

CASE_TO_COLOR = {
    config.CASE_1DO_2DA:  (0.9, 0, 0),
    config.CASE_3DO: (0, 0.74, 0.03),
    config.CASE_2DO_1DA: (0, 0, 1),
    config.CASE_3DA: (0.8, 0.5, 0.95)
}

# %%
plt.figure()
data = {}
fig, ax = plt.subplots()

stds = list()
nums = list()
x_values = list()
colors = list()
markers = list()

NUM_RUNS_TO_INCLUDE = 3

REF_CASE = config.CASE_3DA
ref_values = dict()
for mdl in models:
    ref_values[mdl] = np.mean(
        get_data_num_runs(
            df_step_times.set_index(["Case", "Model", "Duty"]).loc[map_case(REF_CASE), mdl, 0.9],
            NUM_RUNS_TO_INCLUDE
        )["Duration"].mean()
    )

for i, case in enumerate(cases):
    print(f"Case {case}")

    means = list()
    stds = list()
    nums = list()
    x_values = list()
    cnt = i

    for mdl in models:
        try:
            x_data = get_data_num_runs(
                df_step_times.set_index(["Case", "Model", "Duty"]).loc[map_case(case), mdl, 0.9],
                NUM_RUNS_TO_INCLUDE
            )["Duration"]
        except KeyError as e:
            print(e)
            cnt += len(cases) + 1
            continue

        if type(x_data) is np.float64:
            x_data = [x_data]
        else:
            x_data = x_data.values
        means.append(np.mean(x_data/ref_values[mdl]))
        stds.append(np.std(x_data / ref_values[mdl]))
        nums.append(len(x_data)-1)
        x_values.append(cnt)
        colors.append(CASE_TO_COLOR[case])

        cnt += len(cases) + 1
    ax.bar(
        x_values,
        means,
        yerr=stds * ss.t.ppf((1 + 0.9) / 2., nums),
        color=CASE_TO_COLOR[case],
        label=config.case_to_text[case]
    )

ax.legend(
    loc='upper left',
    fontsize=8,
    ncol=4,
    handlelength=1,
    handletextpad=0.3,
    columnspacing=0.5,
    bbox_to_anchor=(-0.3, 1.25, 0, 0),
    borderpad=0.2,
    frameon=False
)
plt.xlabel("Model", labelpad=0)
plt.ylabel("BCT (normed)")
ax.grid(axis="y")
plt.axhline(y=1, color='k', linestyle='--')
plt.xlim(-1, (len(cases) + 1) * len(models) - 1)
plt.xticks(np.arange(len(cases) / 2.0 - 0.5, (len(cases) + 1) * len(models), len(cases) + 1),
           ["DN121", "RN50", "VGG16", "VGG19"])
plt.ylim([0.9, 1.45])

plt.savefig("plots/distributed-ml/batch_duration.pdf")
plt.show()
