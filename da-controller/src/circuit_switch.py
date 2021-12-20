import warnings
import collections
import logging
import os
import json

import ncclient.manager as nc_man
from ncclient.xml_ import new_ele, new_ele_ns, sub_ele_ns, to_xml


warnings.simplefilter("ignore", DeprecationWarning)


OCSConfig = collections.namedtuple("OCSConfig", "host port user password")

DUPLEX_PORT_OFFSET = 32


class OpticalCircuitSwitch(object):
    def __init__(self, config, tor_to_port_mapping):
        """

        :param config:
        :param tor_to_port_mapping: Dict
        """
        self.logger = logging.getLogger(self.__class__.__name__)
        self.config = config
        self.tor_to_port_mapping = tor_to_port_mapping

        self.netconf_man = None
        self.pending_transaction = None
        self.__used_ports = None

        self.__init()

    def __init(self):
        if self.netconf_man is not None:
            return
        self.netconf_man = nc_man.connect_ssh(
            host=self.config.host,
            port=self.config.port,
            username=self.config.user,
            password=self.config.password,
            # TODO(user) Replace fingerprint to match your system setup
            unknown_host_cb=lambda host, fp: fp == 'XXXX',
            look_for_keys=False
        )
        self.netconf_man.async_mode = False

    def init_transaction(self):
        if self.pending_transaction is not None:
            return
        root = new_ele('config')
        configuration = sub_ele_ns(root, 'cross-connects', ns="http://www.polatis.com/yang/optical-switch")
        self.pending_transaction = (root, configuration)
        self.__used_ports = list()

    def set_duplex_link_between_tors(self, src_tor_id, src_cachelink_id, dst_tor_id, dst_cachelink_id):
        self.init_transaction()
        ingress_port = self.tor_to_port_mapping[str(src_tor_id)][str(src_cachelink_id)]
        egress_port = self.tor_to_port_mapping[str(dst_tor_id)][str(dst_cachelink_id)]

        if ingress_port not in self.__used_ports and egress_port not in self.__used_ports:
            pair = sub_ele_ns(self.pending_transaction[1], 'pair', ns="http://www.polatis.com/yang/optical-switch")
            sub_ele_ns(
                pair,
                'ingress',
                ns="http://www.polatis.com/yang/optical-switch"
            ).text = str(ingress_port)
            sub_ele_ns(
                pair,
                'egress',
                ns="http://www.polatis.com/yang/optical-switch"
            ).text = str(egress_port+DUPLEX_PORT_OFFSET)

            pair = sub_ele_ns(self.pending_transaction[1], 'pair', ns="http://www.polatis.com/yang/optical-switch")
            sub_ele_ns(
                pair,
                'ingress',
                ns="http://www.polatis.com/yang/optical-switch"
            ).text = str(egress_port)
            sub_ele_ns(
                pair,
                'egress',
                ns="http://www.polatis.com/yang/optical-switch"
            ).text = str(ingress_port+DUPLEX_PORT_OFFSET)

            # Track used ports in current transaction to avoid duplicates
            self.__used_ports.append(ingress_port)
            self.__used_ports.append(egress_port)

    def unset_duplex_link_between_tors(self, src_tor_id, src_cachelink_id, dst_tor_id, dst_cachelink_id):
        self.init_transaction()
        ingress_port = self.tor_to_port_mapping[str(src_tor_id)][str(src_cachelink_id)]
        if ingress_port not in self.__used_ports:
            pair = sub_ele_ns(
                self.pending_transaction[1],
                'pair',
                ns="http://www.polatis.com/yang/optical-switch",
                attrs={'operation': 'delete'}
            )
            sub_ele_ns(
                pair,
                'ingress',
                ns="http://www.polatis.com/yang/optical-switch"
            ).text = str(ingress_port)

            # Track used ports in current transaction to avoid duplicates
            self.__used_ports.append(ingress_port)

        egress_port = self.tor_to_port_mapping[str(dst_tor_id)][str(dst_cachelink_id)]
        if egress_port not in self.__used_ports:
            pair = sub_ele_ns(
                self.pending_transaction[1],
                'pair',
                ns="http://www.polatis.com/yang/optical-switch",
                attrs={'operation': 'delete'}
            )
            sub_ele_ns(
                pair,
                'ingress',
                ns="http://www.polatis.com/yang/optical-switch"
            ).text = str(egress_port)

            # Track used ports in current transaction to avoid duplicates
            self.__used_ports.append(egress_port)

    def commit(self):
        if self.pending_transaction is None:
            self.logger.warning("Nothing to execute")
            return
        self.logger.info(to_xml(self.pending_transaction[0], encoding='UTF-8', pretty_print=True))
        edit_config_result = self.netconf_man.edit_config(target='running', config=self.pending_transaction[0])
        self.logger.warning(edit_config_result)
        self.pending_transaction = None
        self.__used_ports = None


def init_ocs(args):
    ocs_config_dict = args["ocs"]
    tor_to_port_mapping_path = args["tor_to_port"]

    if not os.path.exists(tor_to_port_mapping_path):
        raise RuntimeError("No Tor Id to OCS port mapping provided.")
    with open(tor_to_port_mapping_path, "r") as fd:
        mapping = json.load(fd)

    return OpticalCircuitSwitch(
        config=OCSConfig(
            host=ocs_config_dict["host"],
            port=ocs_config_dict["port"],
            user=ocs_config_dict["user"],
            password=ocs_config_dict["password"]
        ),
        tor_to_port_mapping=mapping
    )
