from entities import MoongenFlow, MoongenTrafficGenerator, HorovodBenchmarkTrafficGenerator, \
    DistributedIPerfTrafficGenerator
from . import flow_generator


def get_flows_tcp_distr_iperf_simple_set_8_scaling(servers, load=0.3):
    rate = 320 + (load - 0.3) / 0.1 * 106
    return DistributedIPerfTrafficGenerator(
        flow_generator.MixedFixedStepFlowGenerator(
            servers=servers,
            seed=10,
            num_flows=rate * 1,
            size_medium=6.25,
            rate=rate,
            period=0.05,
            base_port=50000,
            pairs_large=[(servers[i], servers[j]) for i, j in [(7, 0), (1, 2), (3, 4), (5, 6)]],
            size_large=167 + 83 * (load - 0.2) / 0.1,
            num_large=1,
            base_port_large=60000
        ), name="large_distr_iperf_tcp_simple_set_scaling", duration=150, scheduling_offset=60
    )


def get_flows_Y_udp_moongen_8tors(servers, rate):
    """
    Two senders, one receiver: S2 -> S0 and S5 -> S0

    :param servers:
    :param rate: Rate in strange numbers: 12 ~ 1Gbits, 8 ~ 1.5, 4 ~3Gbit/s, 2 ~ 6Gbits
    :return:
    """
    assert len(servers) == 8
    flows = [
        (servers[5], MoongenFlow(
            destination=servers[0],
            rate=rate,
            rate_control="sw",
            pattern="custom",
            pktsize=1500,
            dport=60002
        )),
        (servers[7], MoongenFlow(
            destination=servers[0],
            rate=rate,
            rate_control="sw",
            pattern="custom",
            pktsize=1500,
            dport=60001
        )), (servers[0], None), (servers[1], None), (servers[2], None), (servers[4], None), (servers[3], None),
        (servers[6], None)
    ]

    return MoongenTrafficGenerator(
        servers=flows,
        name="mg_udp_2sender_1receiver_8tors",
        duration=15,
        snaplen=100
    )


def get_ml_horovod_gpu_finish(servers, model='vgg19'):
    return HorovodBenchmarkTrafficGenerator(
        servers=servers,
        name="horovod_ml",
        model=model,
        batch_size=None,
        num_warmup_batches=1,
        num_batches=50,
        iface=None,
        cpu=False,
        duration=None
    )
