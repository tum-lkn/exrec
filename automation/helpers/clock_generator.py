import os


# TODO(user) update with your interfaces.
#  True uses python, hence, kernel for sending messages.
#  False means Moongen/DPDK
# i.e., provide PCI address
CLKGEN_PORTS = {
    True: "enp4s0",
    False: "0000:04:00.0"
}


def set_clockgen(use_python=False):
    return use_python, CLKGEN_PORTS[use_python]


def set_data_path(data_path):
    return data_path
