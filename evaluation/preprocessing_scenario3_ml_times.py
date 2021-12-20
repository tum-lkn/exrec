import pandas as pd
import os
import numpy as np
import json


class Case:
    def __init__(self, base_dir, folder, sub_folder):
        self.base_dir = base_dir
        self.folder = folder
        self.sub_folder = sub_folder
        self.raw_durations = None
        self.mean_step_durations = None
        self.sum_step_durations = None
        self.config = None

    def init(self, num_tors=4):
        raw_durations = parse_log_file(
            os.path.join(self.base_dir, self.folder, self.sub_folder, 'ml_training.log'),
            num_tors
        )
        self.mean_step_durations = {key: np.mean(val) for key, val in raw_durations.items()}
        self.sum_step_durations = {key: np.sum(val) for key, val in raw_durations.items()}
        self.raw_durations = raw_durations
        
        self.config = parse_config(os.path.join(self.base_dir, self.folder, self.sub_folder, 'full_config.json'))
        
        
def parse_config(fname):
    with open(fname, "r") as fd:
        cfg_dict = json.load(fd)
    return cfg_dict


# %%
def parse_log_file(fname, num_tors=4):
    """
    Parses log file for step timestamps and returns mean step duration per node
    """
    tor_strings = {f"[1,{i}]": i for i in range(num_tors)}
    raw_times = {i: list() for i in range(num_tors)}
    durations = {i: list() for i in range(num_tors)}
    eval_raw_times = False
    with open(fname, "r") as fd:
        for line in fd.readlines():
            if "<stdout>:158" in line:
                eval_raw_times = True
                print("Match:", line)
                raw_times[tor_strings[line[:5]]].append(float(line.split(':')[1]))
            elif "<stdout>:batch_duration=" in line:
                durations[tor_strings[line[:5]]].append(float(line.split('=')[1]))
    if eval_raw_times:
        for i in range(4):
            durations[i] = np.array(raw_times[i][1:]) - (list(raw_times[i][:-1]))
    return durations


if __name__ == "__main__":
    import config

    ML_FOLDERS = [
        'scenario3_4tors_2do_1da',
        'scenario3_4tors_1do_2da',
        'scenario3_4tors_1do_2da',
        'scenario3_4tors_2do'
    ]

    cases = list()
    data = list()
    for fld in ML_FOLDERS:
        for subfld in os.listdir(os.path.join(config.BASE_DATA_PATH, "scenario3_4tors", fld)):
            if "ml_training.log" not in os.listdir(os.path.join(config.BASE_DATA_PATH, fld, subfld)):
                continue
            case = Case(config.BASE_DATA_PATH, fld, subfld)
            case.init(num_tors=4)
            cases.append(case)
            mdl = case.config["flowgen"]["model"]
            duty = case.config["clkgen"].get("duty", 1)
            data += [
                (fld, mdl, subfld, i, j, case.raw_durations[i][j], duty) for i in range(4) for j in range(len(case.raw_durations[i]))
            ]
    df = pd.DataFrame(
        data,
        columns=['Case', 'Model', 'Subdir', 'Process', 'Step', 'Duration', "Duty"]
    )

    # 3. Save them all into a hdf5 file
    df.to_hdf('ml_workload_step_times_stats.h5', key='df', mode='w')
