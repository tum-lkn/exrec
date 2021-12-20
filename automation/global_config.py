# Host configs
SERVER1 = "server1"
SERVER2 = "server2"
SERVER3 = "server3"
SERVER4 = "server4"

OCS_HOST_NAME = "polatis-oxc"
DA_CTR_HOST = "rotor-ctr"

# PCI-Addresses of Intel X710
# TODO(user) update according to your setup
PCI_ADDR_CARD_1 = "0000:5e:00"
PCI_ADDR_CARD_2 = "0000:af:00"

CORES = {
    PCI_ADDR_CARD_1: [10, 12, 14, 16, 18],
    PCI_ADDR_CARD_2: [11, 13, 15, 17, 19],
}

# Second port of onboard nic
MON_PORT_CLK = "0000:01:00.1"
PORT_SYNC_2 = "0000:01:00.1"
DO_PORT = 0
DO_PORT_2 = 1
DO_PORT_3 = 2

# Path to this project
GIT_PATH = "/home/user/exrec/"

# Path to store data, e.g., a mounted NAS
DATA_BASE_PATH = "/mnt/nas-exrec/"

SNAPLEN = 100

# Clock config
GLOBAL_SYNC = 0

CLK_PERIOD_MILESTONE2 = 0.005
CLK_PERIOD_ML_GPU = 0.005
DUTY_ML_GPU = 0.9
