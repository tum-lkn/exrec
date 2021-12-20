import logging
import os
import json
import time
from sys import argv
import argparse

import circuit_switch
import cache_controller


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', help="Config dict as json", type=str)

    args = parser.parse_args()
    with open(args.config, "r") as fd:
        config = json.load(fd)

    logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s:%(name)s:%(funcName)s: %(message)s")

    myocs = circuit_switch.init_ocs(config)
    if config["ctr_type"] == "simple":
        myctr = cache_controller.SimpleCacheController(config["clknet_iface"], myocs)
    elif config["ctr_type"] == "only_ocs":
        myctr = cache_controller.OnlyOCSCacheController(config["clknet_iface"], myocs)
    else:
        raise ValueError("CTR type not known")

    list_of_list_of_links = config["link_list"]
    assert len(list_of_list_of_links) == 2  # Initial config + new config

    logging.info("Setting initial cache links")
    myctr.reconfigure_links(list_of_list_of_links[0])

    if list_of_list_of_links[1] is not None:
        time.sleep(config["sleep"])
        myctr.reconfigure_links(list_of_list_of_links[1])
