import re
import json
from datetime import datetime


# TODO(user) update according to your setup
NAME_TO_IP_MAPPING = {
    'server1-1': "10.5.0.1",
    'server1-2': "10.5.0.2",
    'server2-1': "10.5.0.3",
    'server2-2': "10.5.0.4",
    'server3-1': "10.5.0.5",
    'server3-2': "10.5.0.6",
    'server4-1': "10.5.0.7",
    'server4-2': "10.5.0.8",
}


def extract_connections_from_file(fname):
    connections = list()
    connection = None
    with open(fname, "r") as fd:
        for line in fd.readlines():
            line = line[:-1]
            if connection is None:
                if re.match("TCP connection [0-9]*:", line):
                    connection = {
                        "id": int(line.replace("TCP connection", "")[:-1])
                    }
            else:
                if "host" in line:
                    h = line[1:].split(":")[1:3]
                    if "src" not in connection:
                        connection["src"] = h[0].strip(" ")
                        connection["sport"] = int(h[1])
                    else:
                        connection["dst"] = h[0].strip(" ")
                        connection["dport"] = int(h[1])
                else:
                    k = line.split(":")[0][1:]
                    v = line.split(":")[1:]
                    if k in ["first packet", "last packet"]:
                        v = datetime.strptime(":".join(v).strip(" "),
                                              "%a %b %d %H:%M:%S.%f %Y").timestamp()
                    if k == "elapsed time":
                        v = ":".join(v).strip(" ")
                    if k == "complete conn" and len(v) == 1:
                        v = v[0].strip(" ")
                    connection[k] = v
                    if k == "total packets":
                        connections.append(connection)
                        connection = None
    return connections


def get_host_for_fname(fname):
    for h, ip in NAME_TO_IP_MAPPING.items():
        if h in fname:
            return ip
    raise RuntimeError("No mapping to IP address found.")


if __name__ == "__main__":
    print("go")
    import os
    import pandas as pd
    import config
    import preprocessing_functions as pref

    MILESTONE = "scenario2_8tors"
    BASE_FOLDER = f"{config.BASE_DATA_PATH}/{MILESTONE}/"

    pcap_files = pref.find_all_pcap_files_without_ftype_files(BASE_FOLDER, ftype="tcptrace")
    print(pcap_files)
    
    num_pcaps = pref.parallel_pcap_parser(pcap_files, num_workers=16, parsing_func=pref.parse_pcap_file_tcptrace)

    print(f"Parsed {num_pcaps} pcaps.")

    MILESTONE_FOLDERS = [
        f"{MILESTONE}_{case}_{load}" for case in [config.CASE_2DO, config.CASE_3DO, config.CASE_2DO_1DA]
        for load in [0.3, 0.4, 0.5, 0.7]
    ]

    output = list()
    for case in MILESTONE_FOLDERS:
        if not os.path.isdir(os.path.join(BASE_FOLDER, case)):
            continue
        for this_dir in [
            sf for sf in os.listdir(os.path.join(BASE_FOLDER, case))
            if os.path.isdir(os.path.join(BASE_FOLDER, case, sf))
        ]:
            print(case, this_dir)
            with open(os.path.join(BASE_FOLDER, case, this_dir, "full_config.json"), "r") as fd:
                    config = json.load(fd)
            for fname in filter(
                    lambda x: "tcptrace" in x,
                    os.listdir(
                        os.path.join(BASE_FOLDER, case, this_dir)
                    )
            ):
                print(fname)
                connections = extract_connections_from_file(os.path.join(BASE_FOLDER, case, this_dir, fname))
                for con in connections:
                    con["host"] = get_host_for_fname(fname)
                    con["case"] = case
                    con["run"] = this_dir
                    con["shaping"] = config["tors"][0].get("shaping", 0)

                output += connections

    pd.DataFrame(output).to_hdf(os.path.join(BASE_FOLDER, "aggregated_flow_data.h5"), key="agg_flow_data")
    print("Done")
