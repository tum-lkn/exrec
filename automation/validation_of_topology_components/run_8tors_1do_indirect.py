import itertools as it
import logging

import literals
from helpers import clock_generator, traffic_inputs
from entities import ClockGenerator, Server, PhysicalMachine, Experiment, DemandObliviousSwitch, \
    DemandObliviousController, ListDataCollector, MoongenTrafficCollector, DockerStatsCollector
from validation_of_topology_components.config import *


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    M_ADDRESSES = [
        SERVER1,
        SERVER2,
        SERVER3,
        SERVER4
    ]

    MATCHINGS = [
        [2, 3, 4, 5, 6, 7, 0],
        [1, 4, 3, 6, 7, 0, 5],
        [4, 1, 2, 7, 0, 5, 6],
        [3, 2, 1, 0, 5, 6, 7],

        [6, 7, 0, 1, 4, 3, 2],
        [5, 0, 7, 2, 1, 4, 3],
        [0, 5, 6, 3, 2, 1, 4],
        [7, 6, 5, 4, 3, 2, 1]
    ]

    USE_PYTHON_CLK = False
    USE_PYTHON_CLK, CLKGEN_PORT = clock_generator.set_clockgen(USE_PYTHON_CLK)
    EXPERIMENT_NAME = f"{MILESTONE_NAME}_1do_indirect"

    DATA_PATH = clock_generator.set_data_path(f"{MILESTONE_BASE_PATH}/{EXPERIMENT_NAME}/")

    MACHINES = [
        PhysicalMachine(addr, GIT_PATH, global_sync_port=GLOBAL_SYNC) for addr in M_ADDRESSES
    ]

    SERVERS = it.product(
        MACHINES, [PCI_ADDR_CARD_1, PCI_ADDR_CARD_2]
    )

    myclkgen = ClockGenerator(CLKGEN_PORT, CLKPERIOD, GIT_PATH, USE_PYTHON_CLK, vmax=VLAN_MAX, duty=0.9)
    myrotorctr = DemandObliviousController(GIT_PATH, "static_rules_x8.py")

    myservers = [
        Server(
            machine=serv[0],
            vm_id=(1 if '5e' in serv[1] else 2),
            pci_address=serv[1],
            sync_port=f"{MON_PORT_CLK if '5e' in serv[1] else PORT_SYNC_2}",
            num_racks=NUM_RACKS,
            virtual_ip=f"{literals.VIRTUAL_IPS_BASE}{i + 1}",
            do_links=[
                DemandObliviousSwitch(DO_PORT, MATCHINGS[i])
            ],
            cores=CORES[serv[1]],
            socket=("/tmp/sock0" if '5e' in serv[1] else None),
            tor_id=(i+1) % NUM_RACKS,
            print_network_stats=PRINT_STATS,
            print_thread_stats=PRINT_THREAD_STATS,
            shaping=0,
            indirect_mode=literals.INDIRECT_MODE_MAX
        ) for i, serv in enumerate(SERVERS)
    ]

    myflows = traffic_inputs.get_flows_Y_udp_moongen_8tors(myservers, rate=5)

    my_data_collect = ListDataCollector(
        DATA_PATH,
        [
            DockerStatsCollector(DATA_PATH, MACHINES),
            MoongenTrafficCollector(DATA_PATH, myservers)
        ]
    )

    Experiment(
        MACHINES,
        myservers,
        myclkgen,
        flowgen=myflows,
        exp_name=EXPERIMENT_NAME,
        do_controller=myrotorctr,
        data_collector=my_data_collect
    ).run(interactive=False)

    for m in MACHINES:
        m.reset()
    print("Done")
