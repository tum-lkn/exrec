import itertools as it
import logging

import literals
from entities import ClockGenerator, Server, PhysicalMachine, Experiment, DemandObliviousSwitch, \
    DemandObliviousController, ListDataCollector, TCPDumpAllToRsCollector, DockerStatsCollector
from helpers import traffic_inputs, clock_generator, da_controllers
from real_application_dml_workload.config import *

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    M_ADDRESSES = [
        SERVER1,
        SERVER2,
        SERVER3,
        SERVER4
    ]

    # vIPs
    VIRTUAL_IPS_BASE = "10.5.0."

    MATCHINGS = [
        [2, 0, 3],
        [1, 3, 0],
        [0, 2, 1],
        [3, 1, 2]
    ]

    MATCHINGS_2 = [
        [0, 3, 2],
        [3, 0, 1],
        [2, 1, 0],
        [1, 2, 3]
    ]

    CACHES = [
        {
            f"{PCI_ADDR_CARD_1}.2": 60001,
            f"{PCI_ADDR_CARD_1}.3": 60001,
            f"{PCI_ADDR_CARD_1}.1": 60001
        },
        {
            f"{PCI_ADDR_CARD_1}.2": 60001,
            f"{PCI_ADDR_CARD_1}.3": 60001,
            f"{PCI_ADDR_CARD_1}.1": 60001
        },
        {
            f"{PCI_ADDR_CARD_1}.2": 60002,
            f"{PCI_ADDR_CARD_1}.3": 60002,
            f"{PCI_ADDR_CARD_1}.1": 60002
        },
        {
            f"{PCI_ADDR_CARD_1}.2": 60002,
            f"{PCI_ADDR_CARD_1}.3": 60002,
            f"{PCI_ADDR_CARD_1}.1": 60001
        }
    ]

    VLAN_MAX = 4

    USE_PYTHON_CLK = False
    USE_PYTHON_CLK, CLKGEN_PORT = clock_generator.set_clockgen(USE_PYTHON_CLK)
    EXPERIMENT_NAME = f"{MILESTONE_NAME}_3da"

    DATA_PATH = clock_generator.set_data_path(f"{MILESTONE_BASE_PATH}/{EXPERIMENT_NAME}/")

    NUM_RACKS = 4

    for clkperiod, model in it.product(
            [
                CLK_PERIOD_ML_GPU
            ], ML_NETWORKS
    ):
        MACHINES = [
            PhysicalMachine(addr, GIT_PATH, global_sync_port=GLOBAL_SYNC) for addr in M_ADDRESSES
        ]

        SERVERS = it.product(
            MACHINES, [PCI_ADDR_CARD_1]
        )
        myclkgen = ClockGenerator(CLKGEN_PORT, clkperiod, GIT_PATH, USE_PYTHON_CLK, vmax=VLAN_MAX, duty=DUTY_ML_GPU)
        myrotorctr = DemandObliviousController(GIT_PATH, "static_rules_x4_all_3cache.py")

        myservers = [
            Server(
                machine=serv[0],
                vm_id=(1 if '5e' in serv[1] else 2),
                pci_address=serv[1],
                sync_port=f"{MON_PORT_CLK if '5e' in serv[1] else PORT_SYNC_2}",
                num_racks=NUM_RACKS,
                virtual_ip=f"{VIRTUAL_IPS_BASE}{i + 1}",
                do_links=[
                    # We use a rotor which does not cycle (static matchings) as third DA link here
                    DemandObliviousSwitch(DO_PORT, MATCHINGS[i])
                ],
                da_links=CACHES[i],
                cores=CORES[serv[1]],
                socket=("/tmp/sock0" if '5e' in serv[1] else None),
                tor_id=(i + 1) % NUM_RACKS,
                vm_type=literals.VM_TYPE_VAGRANT_ML_GPU,
                shaping=1,
                print_network_stats=True,
                print_thread_stats=True
            ) for i, serv in enumerate(SERVERS)
        ]

        myflows = traffic_inputs.get_ml_horovod_gpu_finish(myservers, model=model)

        mycachectr = da_controllers.get_staticlist_x4_3da_all_flows(myservers)

        my_data_collect = ListDataCollector(
            DATA_PATH,
            [
                DockerStatsCollector(DATA_PATH, MACHINES),
                TCPDumpAllToRsCollector(DATA_PATH, myservers, snaplen=100)
            ]
        )

        Experiment(
            MACHINES,
            myservers,
            myclkgen,
            flowgen=myflows,
            exp_name=EXPERIMENT_NAME,
            do_controller=myrotorctr,
            data_collector=my_data_collect,
            da_controller=mycachectr
        ).run(interactive=True)

        for m in MACHINES:
            m.reset()
    print("Done")
