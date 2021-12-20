//
// Created by Johannes Zerwas <johannes.zerwas@tum.de>.
// Adapted example code from https://github.com/udp/json-parser/examples
// See copyright below
//
/* vim: set et ts=4
 *
 * Copyright (C) 2015 Mirko Pasqualetti  All rights reserved.
 * https://github.com/udp/json-parser
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ROTOR_EMULATION_CONFIG_H
#define ROTOR_EMULATION_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>

#include <rte_memory.h>
#include <rte_ethdev.h>
#include <rte_atomic.h>
#include <rte_vhost.h>

#include "json.h"
#include "constants.h"

#define OPTION_SYNC_PORT "sync_port"
#define OPTION_ROTORS "rotors"
#define OPTION_ID "id"
#define OPTION_NUM_RACKS "num_racks"
#define OPTION_PORT "port"
#define OPTION_MATCHINGS "matchings"
#define OPTION_CORES "cores"
#define OPTION_CACHE "cache"
#define OPTION_IP_ADDR "ip"
#define OPTION_SOCKET_FILE "socket"

#define OPTION_PRINT_STATS "print_stats"
#define OPTION_PRINT_CYCLES "print_cycles"
#define OPTION_SHAPING "shaping"
#define OPTION_INDIRECT "indirect_mode"


/* Max number of destination racks, corresponds to max number of cycles (?) */
#define MAX_NUM_DESTINATION_RACKS 8
#define MAX_NUM_ROTORS 4
#define MAX_NUM_CACHE_LINKS 4
#define MAX_NUM_LINKS (MAX_NUM_ROTORS+MAX_NUM_CACHE_LINKS)

#define VLAN_ID_OFFSET 2

#define MAX_NUM_TORS 2
#define MAX_NUM_LOCAL_QUEUES (MAX_NUM_DESTINATION_RACKS+MAX_NUM_CACHE_LINKS)
#define MAX_NUM_NONLOCAL_QUEUES (MAX_NUM_DESTINATION_RACKS)

#define MAX_NUM_PHYS_PORTS_PER_KNI 2

#define SYNC_UDP_PORT 12480
#define CPLANE_UDP_PORT 12490

#define RING_SIZE (128*2048)
#define RING_SIZE_STATS (1024*128)

/* How many packets to attempt to read from NIC in one go */
#define PKT_BURST_SZ            32

/* Ports set in promiscuous mode on by default. */
#define PROMISCUOUS_MODE 1
/* Monitor link status continually. off by default. */
#define MONITOR_LINKS 0

/*
 * VHost stuff
 */
struct vhost_queue {
    struct rte_vhost_vring vr;
    uint16_t last_avail_idx;
    uint16_t last_used_idx;
};

struct device_statistics {
    uint64_t tx;
    uint64_t tx_total;
    rte_atomic64_t rx_atomic;
    rte_atomic64_t rx_total_atomic;
};

struct vhost_dev {
    /**< Number of memory regions for gpa to hpa translation. */
    uint32_t nregions_hpa;
    /**< Device MAC address (Obtained on first TX packet). */
    struct rte_ether_addr mac_address;
    /**< RX VMDQ queue number. */
    uint16_t vmdq_rx_q;
    /**< Vlan tag assigned to the pool */
    uint32_t vlan_tag;
    /**< ToR that the device is added to. */
    uint16_t rx_thread_id;
    /**< A device is set as ready if the MAC address has been set. */
    volatile uint8_t ready;
    /**< Device is marked for removal from the data core. */
    volatile uint8_t remove;

    int vid;
    uint64_t features;
    size_t hdr_len;
    uint16_t nr_vrings;
    struct rte_vhost_memory *mem;
    struct device_statistics stats;
    TAILQ_ENTRY(vhost_dev)
    global_vdev_entry;
    TAILQ_ENTRY(vhost_dev)
    lcore_vdev_entry;

#define MAX_QUEUE_PAIRS    4
    struct vhost_queue queues[MAX_QUEUE_PAIRS * 2];
} __rte_cache_aligned;

TAILQ_HEAD(vhost_dev_tailq_list, vhost_dev
);


/*
 * Struct for rotor switch
 */
struct rotor_switch {
    uint16_t port_id; /* Port ID */
    int32_t vid_to_qid_mapping[MAX_NUM_DESTINATION_RACKS + VLAN_ID_OFFSET];  /* 0-th entry is not used */
};

#define MAX_NUM_FLOWS 65536

struct flow_mapping {
    /* Control plane stuff */
    char flows_to_cache[MAX_NUM_FLOWS];
    uint16_t all_flows_to_cache;
};

struct cache_switch {
    uint16_t port_id;/* Port ID */
    struct flow_mapping *mappings[MAX_NUM_DESTINATION_RACKS];
};

/*
 * Structure of ToRs
 */
struct tor_params {
    uint8_t id;
    uint16_t sync_port;     /* cplane nic */
    unsigned lcore_rx;      /* lcore ID for RX */
    unsigned lcore_tx;      /* lcore ID for TX */
    unsigned lcore_tx2;     /* lcore ID for TX 2 */
    unsigned lcore_sync;    /* lcore ID for sync */

    uint32_t nb_dst_racks;  /* Number of destination racks in setup */

    long num_rotors;
    long num_caches;
    struct rotor_switch *rotors[MAX_NUM_ROTORS];
    struct cache_switch *cache[MAX_NUM_CACHE_LINKS];
    const char *my_address;

    /* VHOST stuff */
    struct vhost_dev *vdev;
    uint32_t device_num;

    /* Queues */
    struct rte_ring *queues[MAX_NUM_LOCAL_QUEUES];
    struct rte_ring *nonlocal_queues[MAX_NUM_NONLOCAL_QUEUES];

    /* Queue budgets */
    uint32_t local_budgets[MAX_NUM_ROTORS * MAX_NUM_LOCAL_QUEUES];
    uint32_t nonlocal_budgets[MAX_NUM_ROTORS * MAX_NUM_NONLOCAL_QUEUES];

    rte_atomic16_t lock_budget;  /* semaphore for budgets */


    /* Statistics printing */
    int print_stats;
    int print_cycles;

    /* Shaping */
    int shaping;       // shaping factor. expresses the share of this ToR in a shaping cycle.
    // Higher numbers mean less. Actual share is given in combination with sum over all ToRs
    uint64_t cycles_per_period;
} __rte_cache_aligned;

struct tor_params *tor_params_array[MAX_NUM_TORS];

struct rx_thread_params {
    /* VHOST stuff */
    uint32_t device_num;
    /* Flag to synchronize device removal. */
    volatile uint8_t dev_removal_flag;
    struct vhost_dev_tailq_list vdev_list;

    int tor_id;
} __rte_cache_aligned;

struct rx_thread_params *rx_thread_params_array[MAX_NUM_TORS];

uint32_t num_tors;
int master_tor;

uint16_t indirect_routing;

#define AVERAGE_PACKET_SIZE 1500
#define LINK_SPEED 10000     /* per bit/us */
#define DEFAULT_SLOT_IN_MS 5000
/* Use only 90% of the slot */
#define DEFAULT_BUDGET_PER_SLOT (LINK_SPEED * DEFAULT_SLOT_IN_MS / AVERAGE_PACKET_SIZE / 8 * 0.9)
static uint32_t budget_per_slot = DEFAULT_BUDGET_PER_SLOT;

/* Socket file paths. Can be set by user */
char *socket_files;
int nb_sockets;

/*
 * FUNCTIONS
 */

int process_array(json_value *value);

int process_object(json_value *value, int tor_id);

int extract_matchings_of_rotor(json_value *matching_array, int tor_id, int rotor_id, int *num_racks);

int extract_rotors(json_value *array, int tor_id);

int extract_caches(json_value *jvalue, int tor_id);

int extract_cores(json_value *array, int tor_id);

int parse_config(const char *arg);

void print_usage(const char *prgname);

void print_config(void);

int parse_args(int argc, char **argv);

int parse_args_eal(int argc, char **argv, int *rte_argc, char **rte_argv);

int parse_config_rte(const char *arg, int *rte_argc, char **rte_argv);

int process_array_rte(json_value *jvalue, int *rte_argc, char **rte_argv);

int process_object_rte(json_value *jvalue, int *rte_argc, char **rte_argv);

int extract_cores_rte(json_value *array, int *rte_argc, char **rte_argv);

#endif //ROTOR_EMULATION_CONFIG_H
