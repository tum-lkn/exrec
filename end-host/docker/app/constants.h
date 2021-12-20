//
// Created by Johannes Zerwas <johannes.zerwas@tum.de>.
//

#ifndef ROTOR_EMULATION_CONSTANTS_H
#define ROTOR_EMULATION_CONSTANTS_H

#include <rte_log.h>

/* Macros for printing using RTE_LOG */
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#define MAX_NAME_LEN 32

/* Max size of a single packet */
#define MAX_PACKET_SZ           2048

/* Size of the data buffer in each mbuf */
#define MBUF_DATA_SZ (MAX_PACKET_SZ + RTE_PKTMBUF_HEADROOM)

/* Number of mbufs in mempool that is created */
#define NB_MBUF                 (32768 * 64)

/* How many objects (mbufs) to keep in per-lcore mempool cache */
#define MEMPOOL_CACHE_SZ        PKT_BURST_SZ

/* Number of RX ring descriptors */
#define NB_RXD                  1024

/* Number of TX ring descriptors */
#define NB_TXD                  512

/* Total octets in ethernet header */
#define KNI_ENET_HEADER_SIZE    14

/* Total octets in the FCS */
#define KNI_ENET_FCS_SIZE       4

#define KNI_US_PER_SECOND       1000000
#define KNI_SECOND_PER_DAY      86400

#define KNI_MAX_KTHREAD 32

#define MIN_MATCHING_VLAN_ID 1


#define TIMER_RESOLUTION_CYCLES 2000000000ULL /* around 500ms at 2 Ghz */

/* EtherType reversed so that CPU stores in BE */
#define BE_RTE_ETHER_TYPE_IPV4 0x0008
#define BE_RTE_ETHER_TYPE_VLAN 0x0081


#endif //ROTOR_EMULATION_CONSTANTS_H
