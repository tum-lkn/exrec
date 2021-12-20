import subprocess
import time

SECOND_IN_NS = 1e9


def event_loop(flows):
    """

    :param flows: list of dicts with flow parameters. MUST ALREADY BE SORTED BY ARRIVAL TIME
    :return:
    """
    current_flow_idx = 0
    for f in flows:
        f["arrival_time"] = time.time_ns() + f["arrival_time"] * SECOND_IN_NS
        print(f["arrival_time"])
    time_next_event = flows[current_flow_idx]["arrival_time"]
    print(f"{len(flows)} flows to schedule.")
    repeat = True
    while repeat:
        try:
            if time.time_ns() >= time_next_event:
                flow = flows[current_flow_idx]
                command = f"{flow['generator']} -c {flow['destination']} " \
                          f" -b {flow['bandwidth']} -B {flow['source']}:{flow['port']} -l 1400 -n {flow['bytes']}" \
                          f" -p {flow['port']} {'-u' if flow['transport']=='udp' else ''}"
                logfile = open("/root/{flow['generator']}_client_{flow['source']}_{flow['transport']}_"
                               "{flow['bandwidth']}_{flow['bytes']}_{flow['destination']}_{flow['port']}.log", "w")
                subprocess.Popen(command, shell=True, stderr=logfile, stdout=logfile)

                current_flow_idx += 1
                time_next_event = flows[current_flow_idx]["arrival_time"]
            if time_next_event < time.time_ns():
                print("WARNING: Sending takes to long")
        except KeyboardInterrupt:
            break
        except IndexError:
            break
    print("Done")


if __name__ == "__main__":
    import sys
    import json

    if len(sys.argv) < 2:
        print("Path to flowlist is missing.")
        exit(1)
    fname = sys.argv[1]

    with open(fname, "r") as fd:
        flowlist = json.load(fd)

    event_loop(flowlist)
