from scapy.all import *
import logging

CTR_PORT = 12490
CTR_IP = "10.0.2.1"


class OnlyOCSCacheController(object):
    def __init__(self, clknet_interface, circuit_switch, ctr_ip=CTR_IP, ctr_port=CTR_PORT):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.clknet_iface = clknet_interface
        self.ctr_port = ctr_port
        self.ctr_ip = ctr_ip

        self.circuit_switch = circuit_switch

    def reconfigure_links(self, list_of_links):
        """

        :param list_of_links: list of dicts
        :return:
        """
        # Set new OCS Links
        self.logger.info("Setting new OCS links")
        for link in list_of_links:
            self.circuit_switch.set_duplex_link_between_tors(
                link["src"], link["cache_id"], link["dst"], link["cache_id"])

        self.circuit_switch.commit()

        self.logger.info("Reconfiguration done")


class SimpleCacheController(object):
    def __init__(self, clknet_interface, circuit_switch, ctr_ip=CTR_IP, ctr_port=CTR_PORT):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.clknet_iface = clknet_interface
        self.ctr_port = ctr_port
        self.ctr_ip = ctr_ip

        self.circuit_switch = circuit_switch

    def send_cplane_message(self, action, src_tor_id, src_cache_id, dst_tor_id, tp_port):
        """

        ACTIONS:
            0:  FLOW ADD
            1:  FLOW REMOVE
            2:  CACHE CLEAR

        :param action:
        :param src_tor_id:
        :param src_cache_id:
        :param dst_tor_id:
        :param tp_port:
        :return:
        """

        bytes_to_send = b''
        bytes_to_send += (action.to_bytes(1, byteorder='little'))
        bytes_to_send += (src_tor_id.to_bytes(1, byteorder='little'))
        bytes_to_send += (src_cache_id.to_bytes(1, byteorder='little'))
        bytes_to_send += (dst_tor_id.to_bytes(1, byteorder='little'))
        bytes_to_send += (tp_port.to_bytes(2, byteorder='big'))
        sendp(
            Ether(dst='ff:ff:ff:ff:ff:ff')/IP(dst=self.ctr_ip)/UDP(dport=self.ctr_port)/Raw(load=bytes_to_send),
            iface=self.clknet_iface
        )
        self.logger.info(f"Sent CPlane message: {action} {src_tor_id} {src_cache_id} {dst_tor_id} {tp_port}")

    def reconfigure_links(self, list_of_links):
        """

        :param list_of_links: list of dicts
        :return:
        """

        # 1. Reset cache config on ToRs
        self.logger.info("Resetting cache config")
        for link in list_of_links:
            # Assume that every ToR has also a new cachelink
            self.send_cplane_message(
                action=2,
                src_tor_id=link["src"],
                src_cache_id=0,
                dst_tor_id=0,
                tp_port=0
            )

        # 2. Set new OCS Links -- automatically overwrites old circuits
        self.logger.info("Setting new OCS links")
        for link in list_of_links:
            self.circuit_switch.set_duplex_link_between_tors(
                link["src"], link["cache_id"], link["dst"], link["cache_id"])

        self.circuit_switch.commit()

        # 3. Set new Cache config on ToRs
        self.logger.info("Resetting OCS links")
        for link in list_of_links:
            # Assume that every ToR has also a new cachelink
            self.send_cplane_message(
                action=0,
                src_tor_id=link["src"],
                src_cache_id=link["cache_id"],
                dst_tor_id=link["dst"],
                tp_port=link["port"]
            )

        self.logger.info("Reconfiguration done")
