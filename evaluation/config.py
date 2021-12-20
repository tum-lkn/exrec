# TODO(user) update according to your setup
BASE_DATA_PATH = "/path/to/data"

NUM_OF_THREADS = 4
NUM_OF_THREADS_FOR_STATS = 4


IP_ADDRESSES = ["10.5.0.{}".format(x) for x in range(1, 9)]

# TODO(user) update according to your setup
SERVER1 = "server1"
SERVER2 = "server2"
SERVER3 = "server3"
SERVER4 = "server4"

CASE_1DO = '1do'
CASE_1DO_INDIRECT = '1do_indirect'
CASE_2DO = '2do'
CASE_1DO_1DA = '1do_1da'
CASE_3DO = '3do'
CASE_3DA = '3da'
CASE_2DO_1DA = '2do_1da'
CASE_1DO_2DA = '1do_2da'

case_to_marker = {
    CASE_2DO: '*',
    CASE_3DO: 'o',
    CASE_2DO_1DA: 'v',
}

case_to_text = {
    CASE_2DO_1DA: '2 \\textsc{DO}, 1 \\textsc{DA}',
    CASE_1DO_2DA: '1 \\textsc{DO}, 2 \\textsc{DA}',
    CASE_3DO: '3 \\textsc{DO}',
    CASE_3DA: "3 \\textsc{DA}",
}

RESNET50 = "ResNet50"
RESNET101 = "ResNet101"
VGG16 = "VGG16"
VGG19 = "VGG19"
DENSENET121 = "DenseNet121"

model_to_line = {
    RESNET50: "-",
    DENSENET121: "-.",
    VGG16: ":",
    VGG19: "--"
}


UDP_FLOW_COLORS = {
    CASE_2DO: {
        0: (0.9, 0, 0),
        1: (0, 0, 1),
    },
    CASE_1DO_1DA: {
        0: (0.9, 0, 0),
        1: (0, 0, 1),
    },
    CASE_1DO: {
        0: (0.9, 0, 0),
        1: (0, 0, 1),
    }
}

SCALING = 2.5
