import pandas as pd
import os
import matplotlib.pyplot as plt
import numpy as np
import re
import subprocess
import logging
import config

logging.basicConfig(level=logging.INFO)


from concurrent import futures


class Flow(object):
    def __init__(self, capture_point, src_ip, dst_ip, sport, dport, isTcp=True, case=None):
        self.capture_point = capture_point
        self.src_ip = src_ip
        self.dst_ip = dst_ip
        self.sport = int(sport)
        self.dport = int(dport)
        self.isTcp = isTcp
        if self.isTcp:
            self.tp = "tcp"
        else:
            self.tp = "udp"
        self.df_rate = None
        self.cutting_time = None
        self.case = case

    def __hash__(self):
        return hash((self.src_ip, self.dst_ip, self.sport, self.dport))

    def __eq__(self, other):
        return self.src_ip, self.dst_ip, self.sport, self.dport

    def __ne__(self, other):
        return not (self == other)

    def __str__(self):
        str_sport = f'{self.sport}'
        str_dport = f'{self.dport}'
        str_rep = self.src_ip + ":" + self.dst_ip + ":" + str_sport + ":" + str_dport
        return str_rep

    def set_flow_rate(
            self,
            data,
            cutting_time=0.01  # default value is 10 milliseconds
    ):
        self.cutting_time = cutting_time

        df_tmp = data[(data['src'] == self.src_ip) &
                      (data['dst'] == self.dst_ip) &
                      (data[f"{self.tp}_sport"] == self.sport) &
                      (data[f"{self.tp}_dport"] == self.dport) &
                      (data['tor'] == self.capture_point)].copy()
        df_tmp.reset_index(inplace=True)

        y = df_tmp.groupby(
            pd.cut(
                df_tmp['time'],
                np.arange(0, df_tmp['time'].max() + cutting_time, cutting_time)
            )
        )['len'].sum()

        new_df = pd.DataFrame()
        new_df['transmitted_bytes'] = y
        new_df['rate'] = (8 * y.values) / cutting_time

        print(f"Set rate of flow: {self} case {self.case.folder} {self.case.sub_folder}")

        self.df_rate = new_df

    def get_total_transmitted_bytes(self):
        return self.df_rate.transmitted_bytes.sum()

    def get_average_rate(self):
        return self.get_total_transmitted_bytes() / self.get_flow_duration()

    def get_flow_duration(self):
        if self.df_rate.empty:
            print(f"Empty frame: {self.case.folder} {self.case.sub_folder} {self.__str__()}")
            return np.NAN

        first_idx = self.df_rate.reset_index().rate.gt(0).idxmax()
        last_idx = self.df_rate.reset_index().rate.iloc[::-1].gt(0).idxmax()

        duration = (last_idx - (first_idx - 1)) * self.cutting_time
        return duration


class Case:
    def __init__(self, base_dir, folder, sub_folder, isTcp=True):
        self.base_dir = base_dir
        self.folder = folder
        self.sub_folder = sub_folder
        self.isTcp = isTcp
        self.raw_data = {}
        self.flows = None
        self.df = None

    def init(self, cutting_time=0.01, reinit=False):
        """
        Method inits case.

        Args:
            cutting_time: to be used for initiliazation.
            reinit: True if existing h5 should be removed first.

        Returns:

        """

        # Remove h5 file if it exists. Otherwise, data will be read from file.
        if reinit and self.h5_exists():
            os.remove(self.get_h5_path())

        self.set_raw_data()
        self.set_data_frame()
        self.set_all_flows()
        self.set_all_flows_rate(cutting_time)

    def set_all_flows_rate(self, cutting_time=0.01, parallel=True):
        logging.info(f"{self.__class__.__name__} Set all flows rate.")

        if parallel:
            parallel_set_flow_rates(self.df, self.flows, cutting_time)
        else:
            for flow in self.flows:
                flow.set_flow_rate(self.df, cutting_time=cutting_time)

    def get_path(self):
        return self.base_dir + self.folder + "/" + self.sub_folder

    def h5_exists(self):
        if os.path.exists(self.get_h5_path()):
            logging.info(f"File {self.get_h5_path()} already exists!")
            return True
        return False

    def get_h5_path(self):
        file_name = f"{self.sub_folder}.h5"
        absolute_path = os.path.join(self.get_path(), file_name)

        return absolute_path

    def set_raw_data(self):
        # Early return if h5 file already exists.
        if self.h5_exists():
            logging.info("h5 already exists -> do not create new one!")
            return None

        # Read in raw data from csv files.
        logging.info("h5 DOES not exist... reading from raw csv data.")
        if not self.raw_data:
            get_raw_dict(
                self.get_path(),
                self.raw_data,
                isTcp=self.isTcp
            )

    def set_data_frame(self):
        # Early return if h5 file already exists.
        if self.h5_exists():
            self.df = pd.read_hdf(self.get_h5_path())
            logging.info(f"Data read from h5 file!")
            return None

        # Read df from raw data.
        logging.info(f"{self.__class__.__name__} {self.folder} {self.sub_folder} Read in data.")
        if self.df is None:
            list_with_dfs = []
            for key, data in self.raw_data.items():
                data['tor'] = key

                list_with_dfs.append(data)
            self.df = pd.concat(list_with_dfs)
            if self.isTcp:
                self.df.drop(['udp_sport', 'udp_dport'], axis=1, inplace=True)
            else:
                self.df.drop(['tcp_sport', 'tcp_dport'], axis=1, inplace=True)
            self.df.dropna(inplace=True)

    def save_df_to_hdf5(self, path_to_file=None, force_rewrite=False):
        if path_to_file is None:
            path_to_file = self.get_h5_path()

        if force_rewrite and os.path.exists(self.get_h5_path()): os.remove(self.get_h5_path())

        if os.path.exists(path_to_file):
            logging.info(f"{self.__class__.__name__} "
                         f"{self.folder} "
                         f"{self.sub_folder} "
                         f"File {path_to_file} already exists!")
            return False

        self.df.to_hdf(path_to_file, key='df', format='table', mode='w')

        return True

    def set_all_flows(self):
        logging.info(f"{self.__class__.__name__} {self.folder} {self.sub_folder} Setting all flows.")
        if self.flows is None:
            self.flows = get_all_flows(self.df, tp="tcp" if self.isTcp else "udp", case=self)

    def get_flow_raw_data(self, flow):
        df_tmp = self.df[
            (self.df['tor'] == flow.capture_point) &
            (self.df['src'] == flow.src_ip) &
            (self.df['dst'] == flow.dst_ip) &
            (self.df[f"{flow.tp}_sport"] == flow.sport) &
            (self.df[f"{flow.tp}_dport"] == flow.dport)].copy()

        return df_tmp


def parallel_set_flow_rates(df, flows, cutting_time):
    logging.info("parallel_set_flow_rates")

    with futures.ThreadPoolExecutor(max_workers=config.NUM_OF_THREADS) as executor:
        to_do = []

        for flow in flows:
            logging.info(f"Submit flow {flow}.")
            future = executor.submit(flow.set_flow_rate, *[df], **{'cutting_time': cutting_time})
            to_do.append(future)
            print(f'Scheduled for {flow.__str__()}: {future}')

        results = []
        for future in futures.as_completed(to_do):
            res = future.result()
            print(f"{future} result: {res}")
            results.append(res)


def plot_flow_over_time(flow, start_time, stop_time, resolution, scaling=1, color='b',
                        linestyle='--', offset=0, xfactor=1):
    # Prepare data
    rate = flow.df_rate.reset_index()['rate']
    rate = rate / 1e9 * scaling

    # Draw plot
    plt.plot(
        rate,
        color=color,
        markevery=3,
        markersize=2,
        linestyle=linestyle
    )

    # Label, Ticks and Ylim and Xlim
    plt.xlabel("Time [s]")
    plt.ylabel("Throughput [GBit/s]")
    xticklabels = ["%.1f" % ((x - offset) * xfactor) for x in np.arange(0, flow.get_flow_duration(), resolution)]
    plt.xticks(
        range(0, int(flow.get_flow_duration() / flow.cutting_time), int(resolution / flow.cutting_time)),
        xticklabels
    )
    plt.xlim(start_time / flow.cutting_time, stop_time / flow.cutting_time)


def find_all_pcap_files_without_ftype_files(rootdir, ftype="csv"):
    """
    Takes a directory. Parses all unparsed pcap files.

    Args:
        rootdir: the dir from which to start parsing.

    Returns:
        list with all unparsed pcap files.

    """

    all_unparsed_pcaps = []

    for subdir, dirs, files in os.walk(rootdir):
        print(f"Dirs: {dirs}")

        print(f"Current folder: {subdir}")
        for file in files:
            if f".{ftype}" in file:
                continue
            if ".pcap" in file and f"{file[:-5]}.{ftype}" not in files:
                print(os.path.join(subdir, file))

                all_unparsed_pcaps.append(os.path.join(subdir, file))

    print(all_unparsed_pcaps)
    return all_unparsed_pcaps


def parse_pcap_file(pcap_file):
    """
    Takes a pcap file and parses it to a csv file.

    Args:
        pcap_file: the absolute file path

    Returns:
        None
    """

    filename = pcap_file[:-5]
    print(f'Filename is {filename}')

    csv_file = open(f"{filename}.csv", "w")

    subprocess.call([
        'tshark',
        '-r', f'{pcap_file}',
        '-T', 'fields',
        '-E', 'header=y',
        '-e', 'frame.time_epoch',
        '-e', 'ip.src',
        '-e', 'frame.len',
        '-e', 'ip.dst',
        '-e', 'udp.srcport',
        '-e', 'udp.dstport',
        '-e', 'tcp.srcport',
        '-e', 'tcp.dstport',
    ],
        stdout=csv_file
    )


def parse_pcap_file_tcptrace(pcap_file):
    """
    Takes a pcap file and parses it to a csv file.

    Args:
        pcap_file: the absolute file path

    Returns:
        None
    """

    filename = pcap_file[:-5]
    print(f'Filename is {filename}')

    csv_file = open(f"{filename}.tcptrace", "w")

    subprocess.call([
        'tcptrace',
        '-l', f'{pcap_file}'
    ],
        stdout=csv_file
    )


def parallel_pcap_parser(pcap_files, num_workers=4, parsing_func=parse_pcap_file):
    """
    Runs multiple parallel threads for parsing pcap files. As this is an IO intensive task, we use threads!

    Args:
        pcap_files: list of pcap files.
        num_workers: to be used for parsing.

    Returns:
        Amount of parsed files.
    """
    with futures.ThreadPoolExecutor(max_workers=num_workers) as executor:
        to_do = []
        for pcap_file in pcap_files:
            future = executor.submit(parsing_func, pcap_file)
            to_do.append(future)
            print(f'Scheduled for {pcap_file}: {future}')

        results = []
        for future in futures.as_completed(to_do):
            res = future.result()
            print(f"{future} result: {res}")
            results.append(res)

    return len(results)


def get_all_flows(df, tp="tcp", case=None):
    """
    This file takes a dataframe that has src ip, dst ip, src port and dst port as columns. From this,
    it extracts the unique flows, i.e., the unique combinations of src ip,dst ip,src port,dst port.

    Args:
        df: Dataframe with many flows.

    Returns:
        A list with all flows.
    """

    df_tmp = df['tor'] + ":" + df['src'] + ":" + df['dst'] + ":" + df[tp + "_sport"].astype(str) + ":" + df[
        tp + "_dport"].astype(str)

    df_tmp = df_tmp.unique()

    all_flows = []

    for flow in df_tmp:
        splitted_flow = flow.split(':')
        all_flows.append(
            Flow(
                capture_point=splitted_flow[0],
                src_ip=splitted_flow[1],
                dst_ip=splitted_flow[2],
                sport=float(splitted_flow[3]),
                dport=float(splitted_flow[4]),
                isTcp=tp == "tcp",
                case=case
            )
        )
    return all_flows


def get_raw_dict(base_dir, raw_data_dict, isTcp=True):
    """
    This function works for the milestone experiments so far.

    Args:
        base_dir:
        folders:

    Returns:

    """
    logging.info("Call get_raw_dict")

    print(base_dir)

    files = get_files_from_folder(base_dir)
    call_thread_executor(files, parse_csv_file, *[raw_data_dict, base_dir, isTcp])


def call_thread_executor(tasks, function, *args, **kwargs):
    logging.info(f"call_thread_executor with {len(tasks)} many tasks.")

    with futures.ThreadPoolExecutor(max_workers=config.NUM_OF_THREADS) as executor:
        to_do = []
        for task in tasks:
            future = executor.submit(function, task, *args)
            to_do.append(future)
            print(f'Scheduled for {task}: {future}')

        results = []
        for future in futures.as_completed(to_do):
            res = future.result()
            print(f"{future} result: {res}")
            results.append(res)
    return len(results)


def parse_csv_file(filename, raw_data, base_dir, isTcp):
    matching_str = ''
    matching_str += '.*'
    # TODO(user) update hostnames according to your setup
    matching_str += '(server1|server2|server4|server3)-'
    matching_str += '(\d)'
    matching_str += '.csv'

    generic_re = re.compile(matching_str)

    match = generic_re.match(filename)

    if (match):
        print(f"Its a match! {filename}")

        server = match.groups()[0]
        tor = match.groups()[1]
        entry_name = server + "_" + tor
        print(f"Create new entry name: {entry_name}")
        data = read_csv_file(
            os.path.join(
                base_dir, filename
            ),
            tcp=isTcp
        )
        if data is not None:
            raw_data[entry_name] = data
        else:
            print("No data.")
    return (filename, match)


def get_files_from_folder(dir):
    return [
        f for f in os.listdir(dir) if os.path.isfile(
            os.path.join(dir, f)
        )
    ]


def read_csv_file(fname, ports=True, align_time=True, tcp=False):
    """
    Read the csv file. Take a filename as input
    """

    print("Read csv: {}".format(fname))
    if tcp:
        data = pd.read_csv(
            fname, header=0, sep='\t',
            names=['time', 'src', 'len', 'dst', 'udp_sport', 'udp_dport', 'tcp_sport', 'tcp_dport']
        )
    elif ports:
        data = pd.read_csv(
            fname, header=0, sep='\t',
            names=['time', 'src', 'len', 'dst', 'udp_sport', 'udp_dport', 'tcp_sport', 'tcp_dport']
        )
    else:
        data = pd.read_csv(
            fname, header=0, sep='\t', names=['time', 'src', 'len', 'dst']
        )
    if len(data) == 0:
        return None
    if align_time:
        data['time'] = data['time'] - data['time'][0]
    return data
