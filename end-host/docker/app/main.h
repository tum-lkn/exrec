//
// Created by Johannes Zerwas <johannes.zerwas@tum.de>.
//


#ifndef DOCKER_MAIN_H
#define DOCKER_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>

#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include <rte_common.h>

#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_bus_pci.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_malloc.h>
#include <rte_kni.h>
#include <rte_ring.h>

#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/virtio_net.h>
#include <linux/virtio_ring.h>
#include <sys/eventfd.h>
#include <sys/param.h>
#include <rte_vhost.h>
#include <rte_pause.h>

#include "config.h"
#include "constants.h"
#include "shaping.h"

#include "ringbuffer.h"

/* Options for configuring ethernet port */
static struct rte_eth_conf port_conf = {
        .txmode = {
                .mq_mode = ETH_MQ_TX_NONE,
                .offloads = (DEV_TX_OFFLOAD_IPV4_CKSUM |
                        DEV_TX_OFFLOAD_TCP_CKSUM |
                        DEV_TX_OFFLOAD_VLAN_INSERT |
                        DEV_TX_OFFLOAD_MULTI_SEGS |
                        DEV_TX_OFFLOAD_TCP_TSO),
        },
};

/* Mempool for mbufs */
static struct rte_mempool *pktmbuf_pool = NULL;


/* Structure type for recording network specific stats */
struct kni_interface_stats {
    /* number of pkts received from NIC, and sent to KNI */
    uint64_t rx_packets_rotors[MAX_NUM_ROTORS];
    uint64_t rx_packets_caches[MAX_NUM_CACHE_LINKS];

    uint64_t rx_packets_rotors_nonlocal[MAX_NUM_DESTINATION_RACKS];

    /* number of pkts received from NIC, but failed to send to KNI */
    uint64_t rx_dropped_rotors[MAX_NUM_ROTORS];
    uint64_t rx_dropped_caches[MAX_NUM_CACHE_LINKS];

    /* number of pkts taken from queue, and sent to NIC */
    uint64_t tx_packets[MAX_NUM_DESTINATION_RACKS];
    uint64_t tx_packets_nonlocal[MAX_NUM_DESTINATION_RACKS];
    uint64_t tx_packets_rotors[MAX_NUM_ROTORS];
    uint64_t tx_dropped_rotors[MAX_NUM_ROTORS];
    uint64_t tx_packets_cache[MAX_NUM_CACHE_LINKS];
    uint64_t tx_dropped_cache[MAX_NUM_CACHE_LINKS];

    /* number of pkts received from KNI, but failed to send to NIC */
    uint64_t tx_dropped[MAX_NUM_DESTINATION_RACKS + MAX_NUM_CACHE_LINKS];

    /* number of pkts queued from KNI */
    uint64_t tx_queued[MAX_NUM_DESTINATION_RACKS + MAX_NUM_CACHE_LINKS];
};

static struct kni_interface_stats *kni_stats[MAX_NUM_TORS];

/*
 * Resource utilization / cycle statistics
 */
#define NUM_TEMP_CYCLE_SAMPLES 100000

struct cycle_value {
    cbuf_handle_t value_queue;
    uint64_t temp_value;
};

#define NUM_JUMP_LOCATIONS 6
struct thread_stats {
    struct cycle_value cycles_rotor[MAX_NUM_ROTORS];
    struct cycle_value num_rotor[MAX_NUM_ROTORS];
    struct cycle_value cycles_cache[MAX_NUM_CACHE_LINKS];
    struct cycle_value num_cache[MAX_NUM_CACHE_LINKS];
    struct cycle_value cycles_total;
    struct cycle_value num_total;

    struct cycle_value locations_cache[MAX_NUM_CACHE_LINKS * NUM_JUMP_LOCATIONS];
    struct cycle_value locations_rotor[MAX_NUM_ROTORS * NUM_JUMP_LOCATIONS];
    struct cycle_value cycle_locations_cache[MAX_NUM_CACHE_LINKS * NUM_JUMP_LOCATIONS];
    struct cycle_value cycle_locations_rotor[MAX_NUM_ROTORS * NUM_JUMP_LOCATIONS];
};
static struct thread_stats *thread_stats_array[MAX_NUM_TORS];

static rte_atomic32_t kni_stop = RTE_ATOMIC32_INIT(0);

struct vlan_hdr {
    uint16_t eth_type;
    uint16_t vlan_id;
};

/* The active matching in terms of the VLAN tag
 * Values <= MIN_MATCHING_VLAN_ID mean open circuit (no matching at all)
 */
static rte_atomic16_t active_matching = RTE_ATOMIC16_INIT(MIN_MATCHING_VLAN_ID);
static rte_atomic16_t cache_active = RTE_ATOMIC16_INIT(0);

struct message {
    char data[MAX_NUM_ROTORS];
};

/*
 *  VHOST related stuff
 */

/* Macros for printing using RTE_LOG */
#define RTE_LOGTYPE_VHOST_CONFIG RTE_LOGTYPE_USER1
#define RTE_LOGTYPE_VHOST_DATA   RTE_LOGTYPE_USER2
#define RTE_LOGTYPE_VHOST_PORT   RTE_LOGTYPE_USER3

enum {
    VIRTIO_RXQ, VIRTIO_TXQ, VIRTIO_QNUM
};


#define REQUEST_DEV_REMOVAL    1
#define ACK_DEV_REMOVAL        0

/* we implement non-extra virtio net features */
#define VIRTIO_NET_FEATURES    0

#ifndef MAX_QUEUES
#define MAX_QUEUES 128
#endif

#define MBUF_CACHE_SIZE    128
#define MBUF_DATA_SIZE    RTE_MBUF_DEFAULT_BUF_SIZE

#define BURST_RX_WAIT_US 15    /* Defines how long we wait between retries on RX */
#define BURST_RX_RETRIES 4        /* Number of retries on RX. */

#define JUMBO_FRAME_MAX_SIZE    0x2600

/* State of virtio device. */
#define DEVICE_MAC_LEARNING 0
#define DEVICE_RX            1
#define DEVICE_SAFE_REMOVE    2

/* Configurable number of RX/TX ring descriptors */
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 512

#define INVALID_PORT_ID 0xFF

/* Maximum long option length for option parsing. */
#define MAX_LONG_OPT_SZ 64

enum {
    ADD_FLOW,
    REMOVE_FLOW,
    CLEAR_CACHE
};

/* number of devices/queues to support*/
static uint32_t num_devices = 1;

static int mergeable = 1;

/* Disable TX checksum offload */
static uint32_t enable_tx_csum = 1;

/* Disable TSO offload */
static uint32_t enable_tso = 0;

static int client_mode;
static int dequeue_zero_copy;

static int builtin_net_driver;

static uint16_t vmdq_queue_base;
static uint16_t queues_per_pool;

static struct vhost_dev_tailq_list vhost_dev_list =
        TAILQ_HEAD_INITIALIZER(vhost_dev_list);


#define MBUF_TABLE_DRAIN_TSC    ((rte_get_tsc_hz() + US_PER_S - 1)/ US_PER_S * BURST_TX_DRAIN_US)
#define VLAN_HLEN       4

// Budget allocations
#define BUDGET_ALLOCATION_ONLY_DIRECT 0
#define BUDGET_ALLOCATION_INDIRECT_MAX 1
#define BUDGET_ALLOCATION_INDIRECT_FIXED 2


/*
 *  --------- PROTOTYPES ------------
 */
void vs_vhost_net_setup(struct vhost_dev *dev);

void vs_vhost_net_remove(struct vhost_dev *dev);

uint16_t vs_enqueue_pkts(struct vhost_dev *dev, uint16_t queue_id,
                         struct rte_mbuf **pkts, uint32_t count);

uint16_t vs_dequeue_pkts(struct vhost_dev *dev, uint16_t queue_id,
                         struct rte_mempool *mbuf_pool,
                         struct rte_mbuf **pkts, uint16_t count);

static void init_local_queues(void);
static void init_nonlocal_queues(void);
void tag_packet(struct rte_mbuf *pkt, uint16_t vlan_id, uint64_t loop_cycle_start, unsigned tor_id,
        unsigned rotor_id, uint64_t cycles_per_period);
int32_t drain_queue_by_budget(uint32_t port_id, struct rte_ring *queue, int32_t budget, int16_t act_matching,
        uint64_t *stats_tx_queue, uint64_t *stats_tx_rotor, uint64_t *stats_dropped_rotor, uint64_t loop_cycle_start,
        unsigned tor_id, unsigned rotor_id, struct tor_params *p);

static uint16_t send_burst_all_packets(uint8_t port_id, uint16_t queue_id, struct rte_mbuf** pkts, uint16_t num_pkts);
static void check_all_ports_link_status(void);

static void print_rotor_budgets(void);
#endif //DOCKER_MAIN_H
