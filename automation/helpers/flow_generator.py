import heapq
import time
import itertools
import numpy.random as rng

import entities


class MixedFixedStepFlowGenerator(object):
    def __init__(self, servers, seed=1248, num_flows=15, size_medium=1, rate=2.0, period=0.05, base_port=50000,
                 pairs_large=None, size_large=1, num_large=0, base_port_large=60000):
        """
        Consider poisson process with fixed step sizes, i.e., poisson distributed num. of arrivals in interval
        :param servers: List of servers (used to uniformly draw connection pairs
        :param seed: seed for rng
        :param num_flows: total number of flows
        :param rate: arrivals per second
        :param period:
        :param base_port:
        """
        self.servers = servers
        self.seed = seed
        self.num_flows = int(num_flows)
        self.size_medium = size_medium
        self.rate = rate
        self.period = period
        self.base_port = base_port
        self.pairs_large = pairs_large
        self.size_large = size_large
        self.num_large = num_large
        self.base_port_large = base_port_large

        self.flows = None
        self.flows_backup = None

    @property
    def name(self):
        return f"MixedFixedStepFlowGen_{len(self.servers)}_{self.seed}_{self.base_port}_" \
               f"{self.num_flows}_{self.size_medium}_{self.rate}_{self.period}_{self.pairs_large}_" \
               f"{self.size_large}_{self.num_large}"

    def reset(self):
        self.flows = list(self.flows_backup)
        heapq.heapify(self.flows)

    def generate(self):
        rng.seed(self.seed)

        self.flows = list()

        # Generate large flows
        cnt = 0
        for i in range(self.num_large):
            for p in self.pairs_large:
                this_flow = entities.IperfFlow(
                    generator_type=entities.Iperf3Flow.IPERF,
                    tp=entities.Iperf3Flow.TCP,
                    arrival_time=0,
                    source=p[0],
                    destination=p[1],
                    bandwidth=0,
                    nbytes=f"{self.size_large}M",
                    src_port=self.base_port_large + cnt
                )
                cnt += 1
                self.flows.append(this_flow)

        # Generate medium size flows
        arrival_time = 0
        pairs = list(filter(lambda x: x[0] != x[1] and x not in self.pairs_large,
                            itertools.product(self.servers, repeat=2)))
        cnt = 0
        while len(self.flows) <= self.num_flows:
            # Get number of arriving flows
            this_num_flows = rng.poisson(lam=self.rate*self.period)
            print(this_num_flows)
            for i in range(this_num_flows):
                src, dst = pairs[rng.randint(0, len(pairs))]

                this_flow = entities.IperfFlow(
                    generator_type=entities.Iperf3Flow.IPERF,
                    tp=entities.Iperf3Flow.TCP,
                    arrival_time=arrival_time,
                    source=src,
                    destination=dst,
                    bandwidth=0,
                    nbytes=f"{self.size_medium}M",
                    src_port=self.base_port + cnt
                )
                cnt += 1
                self.flows.append(this_flow)

                if len(self.flows) >= self.num_flows:
                    self.flows_backup = list(self.flows)
                    return

            # go to next time
            arrival_time += self.period

        self.flows_backup = list(self.flows)

    def get_next_event(self):
        if self.flows is None:
            self.generate()
            heapq.heapify(self.flows)
            time.sleep(3)
        try:
            return heapq.heappop(self.flows)
        except IndexError as e:
            raise e

    def get_dict(self):
        return {
            'name': self.name
        }
