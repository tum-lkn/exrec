import abc
import collections
import datetime
import json
import logging
import os
import subprocess
import time

import paramiko as pmk

import literals

START_TOR_EMULATION_SCRIPT = "start-rotor.sh"
STOP_TOR_EMULATION_SCRIPT = "stop-rotor.sh"

SECOND_IN_NS = 1e9
SECOND_IN_US = 1e6


def sleep_and_dot(duration):
    for i in range(0, duration):
        print('.', end='')
        time.sleep(1)
    print("")


class DataCollector(object):
    def __init__(self, path_to_central_storage):
        self.path_to_central_storage = f"{path_to_central_storage}/{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}"

    @abc.abstractmethod
    def set_exp_name(self, exp_name):
        raise NotImplementedError

    @abc.abstractmethod
    def start_monitoring(self):
        raise NotImplementedError

    @abc.abstractmethod
    def stop_monitoring(self):
        raise NotImplementedError

    @abc.abstractmethod
    def collect(self):
        """
        Collects the data from all vantage points and copies it to a central storage
        :return:
        """
        raise NotImplementedError


class ListDataCollector(DataCollector):
    def __init__(self, path_to_central_storage, collector_list):
        self.logger = logging.getLogger(self.__class__.__name__)
        super(ListDataCollector, self).__init__(path_to_central_storage)

        self.collectors = collector_list

    def set_exp_name(self, exp_name):
        for coll in self.collectors:
            coll.set_exp_name(exp_name)

    def start_monitoring(self):
        for coll in self.collectors:
            # Ugly, but we need to have the same timestamp for all...
            coll.path_to_central_storage = self.path_to_central_storage
            coll.start_monitoring()

    def stop_monitoring(self):
        for coll in self.collectors:
            coll.stop_monitoring()

    def collect(self):
        for coll in self.collectors:
            coll.collect()


class DockerStatsCollector(DataCollector):
    def __init__(self, path_to_central_storage, machines, cache_controller=None):
        self.logger = logging.getLogger(self.__class__.__name__)
        super(DockerStatsCollector, self).__init__(path_to_central_storage)

        self.machines = machines
        self.cache_controller = cache_controller

    def set_exp_name(self, exp_name):
        pass

    def start_monitoring(self):
        pass

    def stop_monitoring(self):
        pass

    def collect(self):
        for m in self.machines:
            _, stdout, _ = m.run_command(f"pidof ./app/build/main")
            pid = int(stdout.read().decode('ascii').strip("\n"))
            m.run_command(f"kill -SIGUSR1 {pid} && mkdir -p {self.path_to_central_storage} && sleep 2 && "
                          f"docker logs --tail 100 dpdk > /{self.path_to_central_storage}/docker_statistics_"
                          f"{m.address}.log")
            m.logger.info(f"{m.address}: Saved docker statistics")

        if self.cache_controller is not None:
            self.cache_controller.run_command(
                f"docker logs cache_ctr > /{self.path_to_central_storage}/docker_statistics_cache_controller.log"
            )


class DockerFullLogCollector(DataCollector):
    def __init__(self, path_to_central_storage, machines, cache_controller=None):
        self.logger = logging.getLogger(self.__class__.__name__)
        super(DockerFullLogCollector, self).__init__(path_to_central_storage)

        self.machines = machines
        self.cache_controller = cache_controller

    def set_exp_name(self, exp_name):
        pass

    def start_monitoring(self):
        pass

    def stop_monitoring(self):
        pass

    def collect(self):
        for m in self.machines:
            m.run_command(f"mkdir -p {self.path_to_central_storage} && sleep 10 && "
                          f"docker logs dpdk > /{self.path_to_central_storage}/docker_full_log_{m.address}.log")
            m.logger.info(f"{m.address}: Saved docker statistics")

        if self.cache_controller is not None:
            self.cache_controller.run_command(
                f"docker logs cache_ctr > /{self.path_to_central_storage}/docker_full_log_cache_controller.log"
            )


class TCPDumpAllToRsCollector(DataCollector):
    def __init__(self, path_to_central_storage, tors, snaplen=None):
        """

        :param exp_name (string): Name of experiment (identifier)
        :param path_to_central_storage (string): Path on orchestrator to NAS mount
        :param tors (list): List of servers/tors
        """
        self.logger = logging.getLogger(self.__class__.__name__)
        super(TCPDumpAllToRsCollector, self).__init__(path_to_central_storage)
        self.exp_name = None
        self.tors = tors

        self.snaplen = snaplen

    def set_exp_name(self, exp_name):
        self.exp_name = exp_name

    def __check_exp_name(self):
        if self.exp_name is None:
            raise ValueError("Experiment name not set")

    def start_monitoring(self):
        self.__check_exp_name()
        self.logger.info("Start monitoring on all ToRs")
        for tor in self.tors:
            fname_dplane_out = f"{self.exp_name}_dplane_{tor.name}.pcap"
            cmd_dplane_out = f"nohup tcpdump -i eth1 -w /root/{fname_dplane_out}"
            if self.snaplen is not None:
                cmd_dplane_out += f" -s {self.snaplen}"
            tor.run_command(cmd_dplane_out)
            tor.logger.info(f"{tor.name}: Started monitoring on data interface")

    def stop_monitoring(self):
        self.__check_exp_name()
        self.logger.info("Stop monitoring on all ToRs")
        for tor in self.tors:
            stdin, stdout, stderr = tor.run_command(f"killall tcpdump")
            tor.logger.info(f"{tor.name}: Stopped monitoring {stderr.read()}")

    def collect(self):
        self.__check_exp_name()
        for tor in self.tors:
            self.logger.info(f"Fetching data from {tor.name}")
            assert isinstance(tor, Server)
            fname_dplane_out = f"{self.exp_name}_dplane_{tor.name}.pcap"
            os.makedirs(f"{self.path_to_central_storage}", mode=0o755, exist_ok=True)
            tor.get_file(f"/root/{fname_dplane_out}", f"{self.path_to_central_storage}/{fname_dplane_out}")
            tor.get_all_files(remotepath="/root", localpath=f"{self.path_to_central_storage}",
                              filter_func=lambda x: 'iperf' in x or 'trace' in x or "ml" in x)


class MoongenTrafficCollector(DataCollector):
    def __init__(self, path_to_central_storage, tors):
        """

        :param exp_name (string): Name of experiment (identifier)
        :param path_to_central_storage (string): Path on orchestrator to NAS mount
        :param tors (list): List of servers/tors
        """
        self.logger = logging.getLogger(self.__class__.__name__)
        super(MoongenTrafficCollector, self).__init__(path_to_central_storage)
        self.exp_name = None
        self.tors = tors

    def set_exp_name(self, exp_name):
        self.exp_name = exp_name

    def __check_exp_name(self):
        if self.exp_name is None:
            raise ValueError("Experiment name not set")

    def start_monitoring(self):
        # Done by Moongen traffic generator
        pass

    def stop_monitoring(self):
        # Done by Moongen traffic generator
        pass

    def collect(self):
        self.__check_exp_name()
        for tor in self.tors:
            self.logger.info(f"Fetching data from {tor.name}")
            assert isinstance(tor, Server)
            fname_dplane_out = f"{self.exp_name}_{tor.name}"
            os.makedirs(f"{self.path_to_central_storage}", mode=0o755, exist_ok=True)
            tor.get_file(f"/root/moongen-scripts/{tor.name}.pcap",
                         f"{self.path_to_central_storage}/{fname_dplane_out}.pcap")
            tor.get_file(f"/root/moongen-scripts/{tor.name}.csv",
                         f"{self.path_to_central_storage}/{fname_dplane_out}_stats.csv")
            tor.get_all_files(remotepath="/root", localpath=f"{self.path_to_central_storage}",
                              filter_func=lambda x: 'iperf' in x or 'trace' in x)


class DemandObliviousController(object):
    """
    Represents OpenFlow DO-controller that installs the static rules for DO forwarding on OF switch.
    Basically, starts a Docker container
    """

    def __init__(self, git_path, rules):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.git_path = git_path
        self.rules = rules

    def start(self):
        self.logger.info(f"Starting OF controller with {self.rules}")
        subprocess.run(
            [f"{self.git_path}/do-controller/start_controller.sh", self.rules],
            stdout=subprocess.DEVNULL
        )

    def stop(self):
        self.logger.info("Stopping OF controller (DO)")
        subprocess.run(["docker", "stop", "of_ctr"])

    def get_dict(self):
        return {
            'rules': self.rules
        }


class DemandAwareController(object):
    """
    Represents a Demand-aware Controller, which sets the demand-aware links. It is assumed that the commands are
    sent from a remote machine.
    """

    def __init__(self, address, git_path):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.address = address
        self.git_path = git_path

        self.ssh_client = None

    def init_connections(self):
        self.ssh_client = pmk.SSHClient()
        self.ssh_client.load_system_host_keys()
        self.ssh_client.connect(self.address)

    def run_command(self, command):
        self.logger.info(f"{self.address}: {command}")
        return self.ssh_client.exec_command(
            command
        )

    @abc.abstractmethod
    def start(self):
        raise NotImplementedError

    @abc.abstractmethod
    def stop(self):
        raise NotImplementedError

    @abc.abstractmethod
    def get_dict(self):
        raise NotImplementedError


class StaticListDemandAwareController(DemandAwareController):
    """
    Old version of simple static demand-aware controller.
    Does not use the Docker container nor controls it the OCS

    Mainly kept for backwards compatibility
    """

    def __init__(self, address, git_path, list_of_links, interface):
        super(StaticListDemandAwareController, self).__init__(address, git_path)
        self.list_of_links = list_of_links
        self.interface = interface

    def start(self):
        for cache in self.list_of_links:
            command = f"python3.8 {self.git_path}/da-controller/src/single_send_cp_message.py " \
                      f"{cache['action']} {cache['src']} {cache['cache_id']} {cache['dst']} " \
                      f"{cache['port']} {self.interface}"
            self.run_command(command)
        time.sleep(2)

    def stop(self):
        pass

    def get_dict(self):
        return {
            'address': self.address,
            'interface': self.interface,
            'links': self.list_of_links
        }


class ScheduledChangeDemandAwareController(DemandAwareController):
    """

    """

    def __init__(self, address, git_path, t, list_of_links_t0, list_of_links_t1, clknet_interface, ocs_config,
                 tor_to_port="/root/mappings/tor_port_mapping_2rotor_2cache.json", ctr_type=None):
        super(ScheduledChangeDemandAwareController, self).__init__(address, git_path)
        self.list_of_links_t0 = list_of_links_t0
        self.list_of_links_t1 = list_of_links_t1
        self.clknet_interface = clknet_interface
        self.t = t
        self.ocs_config = ocs_config
        self.tor_to_port = tor_to_port
        self.ctr_type = ctr_type
        if self.ctr_type is None:
            self.ctr_type = literals.CACHE_CTR_SIMPLE
        self.scp_client = None

    def init_connections(self):
        super(ScheduledChangeDemandAwareController, self).init_connections()
        self.scp_client = pmk.SFTP.from_transport(self.ssh_client.get_transport())

    def dump_config(self):
        config_file = f"{self.git_path}/da-controller/mappings/config.json"
        self.logger.info(f"Dumping config to {config_file}")
        with self.scp_client.file(config_file, "w") as fp:
            json.dump(self.get_dict(), fp, indent=4)

    def start(self):
        self.dump_config()
        self.logger.info("Starting cache controller")
        command = f"{self.git_path}/da-controller/start_controller.sh scheduled_change.py " \
                  f"--config /root/mappings/config.json"
        self.run_command(command)

    def stop(self):
        self.logger.info("Stopping DA controller")
        self.run_command("docker stop da_ctr")

    def get_dict(self):
        return {
            'address': self.address,
            "clknet_iface": self.clknet_interface,
            "sleep": self.t,
            "link_list": [self.list_of_links_t0, self.list_of_links_t1],
            "ocs": self.ocs_config,
            "tor_to_port": self.tor_to_port,
            "ctr_type": self.ctr_type
        }


class ClockGenerator(object):
    def __init__(self, interface, period, git_path, python=True, vmax=8, duty=1.0):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.git_path = git_path
        self.interface = interface
        self.vmax = vmax
        self.python = python  # Has been disabled. We always use Moongen.
        self.duty = duty

        self.command = [f"{self.git_path}/clkgen/moongen/run_docker.sh", "periodic_send"]
        self.period = period * SECOND_IN_US

    def start(self):
        self.logger.info(
            f"Starting {'Python' if self.python else 'Moongen'} clkgen with period {self.period} on interface "
            f"{self.interface} and VLAN_MAX={self.vmax}")
        subprocess.run(
            self.command + [f"{self.interface}", f"{self.period}", f"{self.vmax}",
                            f"{self.duty if not self.python else ''}"],
            stdout=subprocess.DEVNULL
        )

    def stop(self):
        self.logger.info("Stopping clkgen")
        subprocess.run(["docker", "stop", "clkgen"])

    def get_dict(self):
        return {
            'python': self.python,
            'period': self.period,
            'vlan_max': self.vmax,
            'interface': self.interface,
            'duty': self.duty
        }


class PhysicalMachine(object):
    def __init__(self, address, git_path, global_sync_port=None):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.git_path = git_path
        self.address = address
        self.ssh_client = None
        self.scp_client = None
        self.global_sync_port = global_sync_port

        self.servers = list()
        self.config_file = None

        self.pending_tor_emu_start = None

    def reset(self):
        self.servers = list()
        self.config_file = None

    def add_server(self, server):
        if len(self.servers) >= 2:
            RuntimeError("Currently max. 2 ToRs per machine allowed")
        if server not in self.servers:
            self.servers.append(server)

    def init_connections(self):
        self.ssh_client = pmk.SSHClient()
        self.ssh_client.load_system_host_keys()
        self.ssh_client.connect(self.address)
        self.scp_client = pmk.SFTP.from_transport(self.ssh_client.get_transport())

    def terminate(self):
        self.ssh_client.close()

    def run_command(self, command):
        self.logger.info(f"{self.address}: {command}")
        return self.ssh_client.exec_command(
            command
        )

    def log_git_commit(self):
        stdin, stdout, stderr = self.ssh_client.exec_command(
            f"cd {self.git_path} && git rev-parse HEAD"
        )
        self.logger.info(f"{self.address}: Commit: {stdout.read()}")

    def build_docker(self):
        stdin, stdout, stderr = self.ssh_client.exec_command(
            f"{self.git_path}/end-host/docker/build_docker.sh"
        )
        self.logger.info(f"Built docker image on {self.address}")
        return stdin, stdout, stderr

    def set_config_file(self, base_fname):
        self.config_file = f"{base_fname}_{self.address}.json"

        configs = list()
        for serv in self.servers:
            configs.append(
                serv.get_dict()
            )

        self.logger.info(f"Dumping config to {self.config_file}")
        with self.scp_client.file(self.config_file, "w") as fp:
            json.dump(configs, fp, indent=4)

    def start_tor_emulation(self):
        pci_addresses = ""
        for serv in self.servers:
            pci_addresses += serv.pci_address
            pci_addresses += " "

        sync_port = ""
        if self.global_sync_port is not None:
            sync_port += self.servers[self.global_sync_port].mon_port_clk

        if self.config_file is None:
            RuntimeError("Config not dumped yet")

        # Remove old socket file
        self.run_command('rm /tmp/sock0')

        command = f"{self.git_path}/end-host/docker/{START_TOR_EMULATION_SCRIPT} {self.config_file} {pci_addresses} " \
                  f"{sync_port}"
        _, stdout, stderr = self.run_command(command)
        self.pending_tor_emu_start = (stdout, stderr)

    def wait_for_tor_emulation(self):
        if self.pending_tor_emu_start is None:
            self.logger.fatal("No ToR emulation start command issued")
        stdout, stderr = self.pending_tor_emu_start

        self.logger.info(f"Started ToR emulation on {self.address}: {stdout.read().decode('ascii')}")
        for i in range(10):
            _, stdout, stderr = self.run_command('ls /tmp/sock0')
            es = stdout.channel.recv_exit_status()
            if es == 0:
                break
            self.logger.info("ToR emulation not startet yet...")
            sleep_and_dot(8)

    def stop_tor_emulation(self):
        self.run_command(
            f"{self.git_path}/end-host/docker/{STOP_TOR_EMULATION_SCRIPT}"
        )

    def activate_da(self):
        _, stdout, _ = self.run_command(f"pidof ./app/build/main")
        pid = int(stdout.read().decode('ascii').strip("\n"))
        self.run_command(f"kill -SIGUSR2 {pid}")
        self.logger.info(f"{self.address}: toogled cache")


DemandObliviousSwitch = collections.namedtuple("DemandObliviousSwitch", ["port", "matchings"])

TcpConfig = collections.namedtuple("TcpConfig", ["wmem_max", "rmem_max", "optimize"])


class Server(object):
    def __init__(self, machine, vm_id: int, pci_address: str, sync_port: str, virtual_ip: str, do_links: list,
                 cores: list, num_racks: int, socket=None, da_links=None, tor_id: int = -1, vm_type=None,
                 print_network_stats: bool = True, print_thread_stats: bool = False,
                 shaping: int = 0, indirect_mode: int = literals.INDIRECT_MODE_ONLY_DIRECT,
                 tcp_config: TcpConfig = None):
        """

        :param machine: instance of PhysicalMachine where this server runs
        :param vm_id: id of qemu vm (1 or 2)
        :param pci_address (str): pci base address of the prots
        :param sync_port (str): pci address of port for syncing
        :param virtual_ip:
        :param do_links (list): list of instances of DemandObliviousSwitch
        :param cores (list):  list of core ids
        :param socket (str): path to vhost socket
        :param da_links (dict): dict of DA links where keys are pci address and value is the queue id
        :param tor_id (int): Id of the ToR in the setup. Used for addressing of control messages
        :param vm_type (string): Type of VM to spawn (maps to subfolder with vagrant scripts)
        :param print_network_stats (bool): Print network statistics to stdout (inside docker container)
        :param print_thread_stats (bool): Print cycle data to stdout (inside docker container)
        :param shaping (float): Apply the shaper. 0 = deactivated, 1 = equal among links, 0..1 limited share of total
                loop per link
        """
        self.logger = logging.getLogger(self.__class__.__name__)
        self.vm_id = vm_id
        self.tor_id = tor_id
        self.machine = machine
        self.pci_address = pci_address
        self.virtual_ip = virtual_ip
        self.virtual_mac = -1
        self.num_racks = num_racks
        self.do_links = do_links
        self.mon_port_clk = sync_port
        self.cores = cores
        self.socket = socket
        self.da_links = da_links if da_links is not None else dict()
        self.vm_type = vm_type
        if self.vm_type is None:
            self.vm_type = literals.VM_TYPE_VAGRANT

        self.print_network_stats = print_network_stats
        self.print_thread_stats = print_thread_stats
        self.shaping = shaping
        self.indirect_mode = indirect_mode

        self.tcp_config = tcp_config

        self.ssh_client = None
        self.sftp_client = None
        self.pending_commands = None

        self.machine.add_server(self)

    @property
    def name(self):
        return f"{self.machine.address}-{self.vm_id}"

    def get_dict(self, full=False):
        """
        Returns dict with full configuration

        :return:
        """
        config_dict = {
            'sync_port': f"{self.mon_port_clk}",
            'num_racks': self.num_racks,
            'do_links':
                [
                    {
                        'port': f"{self.pci_address}.{r.port}",
                        'matchings': r.matchings
                    }
                    for r in self.do_links
                ],
            'da_links': self.da_links,
            'cores': self.cores,
            'ip': self.virtual_ip,
            'id': self.tor_id,
            'print_stats': self.print_network_stats,
            'print_cycles': self.print_thread_stats,
            'shaping': self.shaping,
            'indirect_mode': self.indirect_mode
        }
        if self.socket is not None:
            config_dict["socket"] = self.socket
        if full:
            config_dict["tcp_config"] = self.tcp_config
        return config_dict

    def run_command(self, cmd):
        return self.ssh_client.exec_command(cmd)

    def put_file_from_string(self, remotepath, s):
        if self.sftp_client is None:
            self.sftp_client = pmk.SFTP.from_transport(self.ssh_client.get_transport())
        with self.sftp_client.file(remotepath, "w") as fp:
            fp.write(s)

    def put_file(self, remotepath, localpath):
        if self.sftp_client is None:
            self.sftp_client = pmk.SFTP.from_transport(self.ssh_client.get_transport())
        self.sftp_client.put(localpath, remotepath)

    def get_file(self, remotepath, localpath):
        if self.sftp_client is None:
            self.sftp_client = pmk.SFTP.from_transport(self.ssh_client.get_transport())
        self.sftp_client.get(remotepath, localpath)

    def get_all_files(self, remotepath, localpath, filter_func=None):
        if filter_func is None:
            filter_func = lambda x: True
        for fname in filter(filter_func, self.sftp_client.listdir(remotepath)):
            self.sftp_client.get(f"{remotepath}/{fname}", f"{localpath}/{fname}")

    def create_vm(self):
        self.destroy_vm()
        _, stdout, stderr = self.machine.run_command(
            f"{self.machine.git_path}/end-host/{self.vm_type}/create-vm.sh {self.vm_id}"
        )
        self.pending_commands = (stdout, stderr)

    def wait_for_vms(self):
        """
        Waits until VM boot has finished. Then creates an SSH connection to the VM
        :return:
        """
        if not self.pending_commands:
            return

        stdout, stderr = self.pending_commands
        exit_state = stdout.channel.recv_exit_status()
        self.pending_commands = None
        self.logger.info(
            f"Created VM {self.vm_id} of type {self.vm_type} on {self.machine.address}: {exit_state} {stderr.read()}")

        self.ssh_client = pmk.SSHClient()
        self.ssh_client.set_missing_host_key_policy(pmk.WarningPolicy())
        self.ssh_client.connect(self.machine.address, port=20000 + self.vm_id)

    def stop_vm(self):
        _, _, stderr = self.machine.run_command(
            f"cd /vagrant/{self.vm_id} && vagrant halt"
        )
        self.logger.info(f"Stopped VM {self.vm_id} on {self.machine.address}: {stderr.read()}")

    def destroy_vm(self):
        _, _, stderr = self.machine.run_command(
            f"{self.machine.git_path}/end-host/vagrant/delete-vm.sh {self.vm_id}"
        )
        self.logger.info(f"Destroyed VM {self.vm_id} on {self.machine.address}: {stderr.read()}")

    def set_vip(self):
        _, _, stderr = self.ssh_client.exec_command(
            f"ip addr add {self.virtual_ip}/24 dev eth1"
        )
        time.sleep(1)
        self.logger.info(f"Set IP on {self.name} to {self.virtual_ip}: {stderr.read()}")

        _, stdout, _ = self.ssh_client.exec_command(
            f"cat /sys/class/net/eth1/address"
        )
        mac = stdout.read()
        self.virtual_mac = mac.decode('ascii').strip("\n")
        self.logger.info(f"{self.name}: Virtual MAC is {self.virtual_mac}")

    def set_arp_entry(self, server):
        self.ssh_client.exec_command(
            f"arp -i eth1 -s {server.virtual_ip} {server.virtual_mac}"
        )
        self.logger.info(f"{self.name}: Added arp entry for {server.virtual_ip} to {server.virtual_mac}")

        self.ssh_client.exec_command(
            f"ping -I eth1 -c 1 8.8.8.8"
        )
        self.logger.info(f"{self.name}: Sent 1 ping to trigger VM to ToR assignment in DPDK")

    def set_tcp_config(self):
        if not self.tcp_config:
            return
        self.ssh_client.exec_command(f"sysctl -w net.core.wmem_max={self.tcp_config.wmem_max}")
        self.ssh_client.exec_command(f"sysctl -w net.core.rmem_max={self.tcp_config.rmem_max}")

        if self.tcp_config.optimize:
            self.ssh_client.exec_command(f"sysctl -w net.ipv4.tcp_no_metrics_save=1")
            self.ssh_client.exec_command(f"sysctl -w net.ipv4.tcp_congestion_control=htcp")
            self.ssh_client.exec_command(f"sysctl -w net.core.default_qdisc=fq")


class Iperf3Flow(object):
    IPERF = "iperf"
    IPERF3 = 'iperf3'
    TCP = "tcp"
    UDP = "udp"

    def __init__(self, generator_type, tp, arrival_time, source, destination, bandwidth, nbytes, src_port,
                 buffer: str = None, segment: int = 1400):
        self.generator_type = Iperf3Flow.IPERF3
        self.tp = tp
        self.bandwidth = bandwidth
        self.bytes = nbytes
        self.source = source
        self.destination = destination
        self.arrival_time = arrival_time
        self.src_port = src_port
        self.buffer = buffer  # Buffer size
        self.segment = segment

    def init(self):
        serv = self.destination
        buffer = f" -w {self.buffer}" if self.buffer else ""

        serv.run_command(
            f"iperf3 -s -i 2 -B {serv.virtual_ip} -p {self.src_port}{buffer} > "
            f"/root/iperf3_server_{serv.name}_{self.src_port}.log &"
        )
        serv.logger.info(f"{serv.name}: Started IPerf server")

    def stop_receiver(self):
        self.destination.run_command(
            f"killall iperf3"
        )
        self.destination.logger.info(f"{self.destination.name}: Stopped IPerf server")

    def start(self):
        self.init()
        return self.start_iperf_client_on_server()

    def start_iperf_client_on_server(self):
        serv = self.source
        command = f"iperf3 -c {self.destination.virtual_ip} {'-u' if self.tp == Iperf3Flow.UDP else ''} " \
                  f" -b {self.bandwidth} -B {serv.virtual_ip}"

        if self.segment is not None:
            command += f" -l {self.segment}"

        if self.bytes:
            command += f" -n {self.bytes}"
        else:
            # Setting bytes to none means to run forever
            command += f" -t 0"

        if self.src_port is not None:
            command += f" -p {self.src_port} --cport {self.src_port}"

        if self.buffer is not None:
            command += f" -w {self.buffer}"

        serv.logger.info(command)
        stdin, stdout, stderr = serv.run_command(
            f"{command} > /root/iperf3_client_{serv.name}_{self.tp}_{self.bandwidth}_{self.bytes}_"
            f"{self.destination.virtual_ip}_{self.src_port}.log &"
        )
        serv.logger.info(
            f"{serv.name}: Started IPerf3 {self.tp} client to {self.destination.virtual_ip} with "
            f"{self.bytes} bytes on iface {serv.virtual_ip}")
        return stdout, stderr

    def get_dict(self):
        return {
            'generator': self.generator_type,
            'transport': self.tp,
            'bandwidth': self.bandwidth,
            'bytes': self.bytes,
            'source': self.source.virtual_ip,
            'destination': self.destination.virtual_ip,
            'arrival_time': self.arrival_time,
            'port': self.src_port,
            "buffer": self.buffer,
            "segment": self.segment
        }

    def __str__(self):
        return f"{self.source.virtual_ip}:{self.src_port} - {self.destination.virtual_ip} " \
               f"Arrival: {self.arrival_time} Bytes: {self.bytes}"

    def __lt__(self, other):
        if self.arrival_time != other.arrival_time:
            return self.arrival_time < other.arrival_time
        if self.bytes != other.bytes:
            return self.bytes < other.bytes
        if self.src_port != other.src_port:
            return self.src_port < other.src_port

        raise RuntimeError()


class IperfFlow(Iperf3Flow):
    def __init__(self, generator_type, tp, arrival_time, source, destination, bandwidth, nbytes, src_port,
                 buffer: str = None, segment: int = 1400):
        super(IperfFlow, self).__init__(
            generator_type, tp, arrival_time, source, destination, bandwidth, nbytes, src_port,
            buffer, segment
        )
        self.generator_type = Iperf3Flow.IPERF

    def init(self):
        serv = self.destination
        buffer = f" -w {self.buffer}" if self.buffer else ""
        serv.run_command(
            f"iperf -s -i 2 -B {serv.virtual_ip} -p {self.src_port}{buffer} > "
            f"/root/iperf_server_{serv.name}_{self.src_port}.log &"
        )
        serv.logger.info(f"{serv.name}: Started IPerf server")

    def stop_receiver(self):
        self.destination.run_command(
            f"killall iperf"
        )
        self.destination.logger.info(f"{self.destination.name}: Stopped IPerf server")

    def start_iperf_client_on_server(self):
        serv = self.source
        command = f"iperf -c {self.destination.virtual_ip} {'-u' if self.tp == Iperf3Flow.UDP else ''} " \
                  f" -b {self.bandwidth} -B {serv.virtual_ip}:{self.src_port}" \
                  f" -p {self.src_port} "
        if self.bytes:
            command += f" -n {self.bytes}"
        else:
            command += " -t 0"

        if self.buffer is not None:
            command += f" -w {self.buffer}"

        serv.logger.info(command)
        stdin, stdout, stderr = serv.run_command(
            f"{command} > /root/iperf_client_{serv.name}_{self.tp}_{self.bandwidth}_{self.bytes}_"
            f"{self.destination.virtual_ip}_{self.src_port}_{self.buffer}.log"
        )
        serv.logger.info(
            f"{serv.name}: Started IPerf {self.tp} client to {self.destination.virtual_ip} with {self.bytes} "
            f"bytes on iface {serv.virtual_ip}")
        return stdout, stderr


class TrafficGenerator(object):
    """
    Creates the actual traffic sources for the measurement. Might take a pre-generated list of flows or trace to
    replay depending on the specific traffic type.
    """

    @abc.abstractmethod
    def start_traffic(self, machines):
        raise NotImplementedError

    @abc.abstractmethod
    def prepare(self):
        raise NotImplementedError

    @abc.abstractmethod
    def stop(self):
        raise NotImplementedError

    @abc.abstractmethod
    def get_dict(self):
        raise NotImplementedError


class DistributedIPerfTrafficGenerator(TrafficGenerator):
    """
    Distributed approach of spawning iPerf instances. Splits the list of flows into per ToR/Host lists and distributes
    them accordingly. Next, all iPerf server instances are started. It synchronizes the system times and schedules the
    start of the traffic generation. On each ToR/hosts, a small script is started that reads in the list of flows and
    starts the instances according to their arrival times.
    """

    def __init__(self, flowgen, name, duration=20, scheduling_offset=10):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.flowgen = flowgen
        self.name = name
        self.duration = duration
        self.scheduling_offset = scheduling_offset

        self.flows = list()
        self.destinations = list()
        self.sources = list()

    def prepare(self):
        """
        Distributes the flow lists and starts the server instances
        :return:
        """
        flows_per_source = collections.defaultdict(list)
        self.flowgen.generate()
        for flow in self.flowgen.flows:
            if flow.destination not in self.destinations:
                self.destinations.append(flow.destination)
            if flow.source not in self.sources:
                self.sources.append(flow.source)
            # Collect flows by source
            flows_per_source[flow.source].append(flow.get_dict())
            # Start iPerf server
            flow.init()
        self.logger.info("Started all iPerf servers")

        # Create json file for host
        for source in self.sources:
            this_flows = sorted(flows_per_source[source], key=lambda x: x["arrival_time"])
            source.put_file_from_string("/root/flowlist.json", json.dumps(this_flows))
            source.put_file("/root/flow_scheduler.py", "./helpers/remote/flow_scheduler.py")
        self.logger.info("Wrote flowlists to hosts/ToRs.")

    def start_traffic(self, machines):
        sched_time = time.strftime("%H:%M %Y-%m-%d", time.gmtime(time.time() + self.scheduling_offset))
        for source in self.sources:
            _, stdout, stderr = source.run_command(
                f"echo 'python3.7 /root/flow_scheduler.py /root/flowlist.json' | at {sched_time}"
            )
            self.logger.info(stderr.read().decode('ascii').strip('\n'))

        self.logger.info(f"Scheduled flow schedulers at {sched_time}")

        sleep_and_dot(self.duration)

    def stop(self):
        """
        Stop running iperf servers
        :return:
        """
        for dst in self.destinations:
            dst.run_command(f"killall iperf")
            dst.logger.info(f"{dst.name}: Stopped IPerf server")

    def get_dict(self):
        return {
            'type': 'distributed_iperf_traffic',
            'flowgen': self.flowgen.get_dict()
        }


MoongenFlow = collections.namedtuple(
    "MoongenFlow",
    ["destination", "rate", "rate_control", "pattern", "pktsize", "dport"]
)


class MoongenTrafficGenerator(TrafficGenerator):
    def __init__(self, servers, name, duration=20, snaplen=100):
        """

        :param servers (list): of tuples of Server and MoongenFlow
        :param name:
        :param duration:
        """
        self.logger = logging.getLogger(self.__class__.__name__)
        self.servers = servers
        self.name = name
        self.duration = duration
        self.snaplen = snaplen

    def start_traffic(self, machines):
        self.logger.info("Start MG traffic {len(self.servers)}")
        base_fname = f"/root/moongen-scripts/"
        cmds = list()

        for server, flow in self.servers:
            self.logger.info(f"Start MG on {server.name}")
            this_fname = f"{base_fname}{server.name}"
            parameter = f" -s {self.snaplen} -f {this_fname}.pcap -o {this_fname}.csv -t 1"
            if flow is not None:
                parameter += f" --rate {flow.rate} --rc {flow.rate_control} " \
                             f"--pattern {flow.pattern} --pktsize {flow.pktsize} -u {flow.dport} " \
                             f"--dstip {flow.destination.virtual_ip} --srcip {server.virtual_ip}"

            _, stdout, _ = server.run_command(
                f"/root/moongen-scripts/run_docker.sh {parameter}"
            )
            cmds.append(stdout)

        for stdout in cmds:
            # Wait until all containers are started
            stdout.channel.recv_exit_status()
        self.logger.info("Measuring time starts...")
        sleep_and_dot(self.duration)

    def prepare(self):
        for server, flow in self.servers:
            self.logger.info(f"Remove IP setting on {server.name}")
            server.run_command(
                f"ip addr del {server.virtual_ip}/24 dev eth1"
            )

    def stop(self):
        for server, _ in self.servers:
            server.run_command(
                f"kill -15 $(pidof ./MoonGen/build/MoonGen)"
            )

    def get_dict(self):
        return {
            'type': 'moongen_traffic',
            'snaplen': self.snaplen,
            'duration': self.duration,
            'flows': [
                (s.virtual_ip, flow.destination.virtual_ip, flow.rate, flow.rate_control, flow.pattern, flow.pktsize,
                 flow.dport) if flow is not None else None for s, flow in self.servers

            ]
        }


class MachineLearningTrafficGenerator(TrafficGenerator):
    """
    Starts distributed ML using horovod to generate application traffic.
    Per ToR one MPI process is started. Model training is taken from tf benchmarks
    """

    def __init__(self, servers, name, model=None, batch_size=None, num_batches=None, iface=None, cpu=True, duration=60,
                 num_warmup_batches=None):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.servers = servers
        self.model = model if model is not None else "resnet50"
        self.batch_size = batch_size
        self.num_batches = num_batches
        self.num_warmup_batches = num_warmup_batches
        self.iface = iface if iface is not None else "eth1"
        self.cpu = cpu
        self.name = name
        self.duration = duration

        # Used to change directory or similar stuff
        self._pre_mpi_command = f"cd tf_benchmark/benchmarks && "

    def get_dict(self):
        return {
            'type': self.__class__.__name__,
            'model': self.model,
            'batch_size': self.batch_size,
            'num_batches': self.num_batches,
            'iface': self.iface,
            'cpu': self.cpu,
            'duration': self.duration,
            'num_warmup_batches': self.num_warmup_batches
        }

    def build_benchmark_command(self):
        command = f"python3 scripts/tf_cnn_benchmarks/tf_cnn_benchmarks.py " \
                  f"--model {self.model} " \
                  f"--variable_update horovod --display_every=1 "
        if self.cpu:
            command += f"--device=cpu --data_format=NHWC "
        if self.num_batches:
            command += f"--num_batches={self.num_batches} "
        if self.batch_size:
            command += f"--batch_size={self.batch_size} "
        if self.num_warmup_batches:
            command += f"--num_warmup_batches={self.num_warmup_batches} "
        return command

    def start_traffic(self, machines):
        hostlist = ""
        for serv in self.servers:
            hostlist += f"{serv.virtual_ip},"
        hostlist = hostlist[:-1]  # remove the last ,
        command = f"{self._pre_mpi_command} mpirun --allow-run-as-root --verbose --tag-output --oversubscribe " \
                  f"-H {hostlist} -np {len(self.servers)} " \
                  f"-mca btl_tcp_if_include {self.iface} -x NCCL_DEBUG=INFO " \
                  f"-x NCCL_SOCKET_IFNAME={self.iface} " \
                  f"-bind-to none -map-by slot -mca pml ob1 -mca btl ^openib " \
                  f"-x LD_LIBRARY_PATH -x SSH_CONNECTION -x LANG -x XDG_SESSION_ID -x USER " \
                  f"-x PWD -x HOME -x SSH_CLIENT -x SSH_TTY -x MAIL -x TERM -x SHELL -x SHLVL -x LOGNAME -x PATH " \
                  f" {self.build_benchmark_command()}"

        self.logger.info(f"Command: {command}")
        _, stdout, stderr = self.servers[0].run_command(f"{command} > /root/ml_training.log 2>&1")
        self.logger.info("Started ML application.")
        if not self.duration:
            self.logger.info("Waiting for exit...")
            exit_state = stdout.channel.recv_exit_status()

            output = stderr.read().decode('ascii').strip('\n')
            self.logger.info(
                f"ML application exited with state {exit_state} {output if exit_state != 0 else ''}")
        else:
            sleep_and_dot(self.duration)

    def prepare(self):
        pass

    def stop(self):
        pass


class HorovodBenchmarkTrafficGenerator(MachineLearningTrafficGenerator):
    def __init__(self, servers, name, model=None, batch_size=None, num_batches=None, iface=None, cpu=True, duration=60,
                 num_warmup_batches=None):
        super(HorovodBenchmarkTrafficGenerator, self).__init__(
            servers, name, model, batch_size, num_batches, iface, cpu, duration, num_warmup_batches
        )
        self.logger = logging.getLogger(self.__class__.__name__)

        # Used to change directory or similar stuff
        self._pre_mpi_command = ""

    def build_benchmark_command(self):
        command = f"python3 horovod_source/horovod/examples/tensorflow_synthetic_benchmark.py " \
                  f"--model {self.model} " \
                  f"--num-batches-per-iter=1 "
        if self.cpu:
            raise ValueError("Using CPU instead of GPUs is not possible.")
        if self.num_batches:
            command += f"--num-iters={self.num_batches} "
        if self.batch_size:
            command += f"--batch-size={self.batch_size} "
        if self.num_warmup_batches:
            command += f"--num-warmup-batches={self.num_warmup_batches} "
        return command


class Experiment(object):
    def __init__(self, machines, servers, clkgen, flowgen, exp_name, do_controller, data_collector,
                 da_controller=None):
        """

        :param machines:
        :param servers:
        :param clkgen:
        :param flowgen: list of tg configs
        :param exp_name:
        :param do_controller (DemandObliviousController):
        :param da_controller (DemandAwareController):
        """
        self.logger = logging.getLogger(self.__class__.__name__)
        self.machines = machines
        self.servers = servers
        self.clkgen = clkgen
        self.do_controller = do_controller
        self.flowgen = flowgen
        assert isinstance(self.flowgen, TrafficGenerator)
        self.collector = data_collector
        assert isinstance(self.collector, DataCollector)
        self.da_controller = da_controller
        assert self.da_controller is None or isinstance(self.da_controller, DemandAwareController)

        self.base_exp_name = exp_name

    def get_dict(self):
        return {
            'commit': subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).decode('ascii').strip(),
            'tors': [s.get_dict(full=True) for s in self.servers],
            'clkgen': self.clkgen.get_dict(),
            'do_controller': self.do_controller.get_dict(),
            'da_controller': self.da_controller.get_dict() if self.da_controller is not None else None,
            'flowgen': self.flowgen.get_dict()
        }

    def dump_config(self):
        os.makedirs(f"{self.collector.path_to_central_storage}", exist_ok=True)
        with open(f"{self.collector.path_to_central_storage}/full_config.json", "w") as fd:
            json.dump(self.get_dict(), fd, indent=4)

    def get_full_exp_name(self):
        return f"{self.base_exp_name}_{len(self.servers)}_{self.clkgen.period}_" \
               f"{self.flowgen.name}"

    def get_config_path(self, base_path):
        return f"{base_path}/end-host/docker/config/{self.base_exp_name}_{len(self.servers)}"

    def __prepare_servers(self):
        for m in self.machines:
            m.init_connections()
            m.log_git_commit()
            m.set_config_file(self.get_config_path(m.git_path))
            m.start_tor_emulation()

        for m in self.machines:
            assert (isinstance(m, PhysicalMachine))
            m.wait_for_tor_emulation()

        sleep_and_dot(25)

        for serv in self.servers:
            serv.create_vm()

        for serv in self.servers:
            assert isinstance(serv, Server)
            serv.wait_for_vms()
            serv.set_vip()
            time.sleep(1)
            serv.set_tcp_config()

        sleep_and_dot(10)

        self.flowgen.prepare()
        self.collector.set_exp_name(self.get_full_exp_name())
        self.collector.start_monitoring()
        for serv in self.servers:
            for dst in self.servers:
                if dst != serv:
                    serv.set_arp_entry(dst)

    def __prepare_controllers(self):
        assert isinstance(self.do_controller, DemandObliviousController)
        self.do_controller.start()
        if self.da_controller:
            self.da_controller.init_connections()
            self.da_controller.start()

    def __prepare_clkgen(self):
        self.clkgen.start()
        time.sleep(10)

    def __start_traffic_gen(self):
        self.flowgen.start_traffic(self.machines)

    def __clean_up(self):
        self.flowgen.stop()
        self.collector.stop_monitoring()
        self.collector.collect()

        for serv in self.servers:
            serv.destroy_vm()

        if self.da_controller:
            self.da_controller.stop()
        for m in self.machines:
            m.stop_tor_emulation()
            m.terminate()
        self.clkgen.stop()
        self.do_controller.stop()

    def run(self, interactive=False):
        try:
            self.dump_config()

            self.__prepare_servers()
            self.__prepare_controllers()
            self.__prepare_clkgen()
            if interactive:
                input("Press key to continue...")
            self.__start_traffic_gen()
        except KeyboardInterrupt:
            pass
        finally:
            self.__clean_up()
