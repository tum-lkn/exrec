#include "main.h"

/* Print out statistics on packets handled */
static void print_stats(void) {
    unsigned i, tor_id;
    struct rte_eth_stats stat_info;
    struct rte_eth_link link;
    int stat;
    printf("\n**Statistics**\n"
           "======  ============  ============  ============ \n");
    for (tor_id = 0; tor_id < num_tors; tor_id++) {
        if (!tor_params_array[tor_id]->print_stats) {
            continue;
        }
        printf("ToR %i\n", tor_id);
        printf("======  ============  ============ =========== ===========  ===========\n"
               " QID    tx_packets    tx_dropped   tx_queued   rx_nonlocal  tx_nonlocal\n"
               "------  ------------  ------------ ----------- -----------  -----------\n");
        for (i = 0; i < tor_params_array[tor_id]->nb_dst_racks; i++) {
            printf("%7d %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            "\n",
                    i,
                    kni_stats[tor_id]->tx_packets[i],
                    kni_stats[tor_id]->tx_dropped[i],
                    kni_stats[tor_id]->tx_queued[i],
                    kni_stats[tor_id]->rx_packets_rotors_nonlocal[i],
                    kni_stats[tor_id]->tx_packets_nonlocal[i]
            );
        }

        printf("======  ============  ============  ============  ===========\n"
               " RID    tx_dropped    tx_packets    rx_dropped    rx_packets \n"
               "------  ------------  ------------  ------------  -----------\n");
        for (i = 0; i < tor_params_array[tor_id]->num_rotors; i++) {
            printf("%7d %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            "\n",
                    i,
                    kni_stats[tor_id]->tx_dropped_rotors[i],
                    kni_stats[tor_id]->tx_packets_rotors[i],
                    kni_stats[tor_id]->rx_dropped_rotors[i],
                    kni_stats[tor_id]->rx_packets_rotors[i]
            );

            stat = rte_eth_stats_get(tor_params_array[tor_id]->rotors[i]->port_id, &stat_info);
            if (stat == 0) {
                printf("   In: %"
                PRIu64
                " (%"
                PRIu64
                " bytes)\n"
                "  Out: %"
                PRIu64
                " (%"
                PRIu64
                " bytes)\n"
                "  Err: %"
                PRIu64
                "\n",
                        stat_info.ipackets,
                        stat_info.ibytes,
                        stat_info.opackets,
                        stat_info.obytes,
                        stat_info.ierrors + stat_info.oerrors
                );
            } else if (stat == -ENOTSUP)
                printf("Port %i: Operation not supported\n", tor_params_array[tor_id]->rotors[i]->port_id);
            else
                printf("Port %i: Error fetching statistics\n", tor_params_array[tor_id]->rotors[i]->port_id);
        }


        printf("======  ============  ============  ============ ============ ============ ===========\n"
               " CID    tx_dropped    tx_packets    tx_queued    tx_qdropped   rx_dropped   rx_packets \n"
               "------  ------------  ------------  ------------ ------------ ------------ -----------\n");
        for (i = 0; i < tor_params_array[tor_id]->num_caches; i++) {
            printf("%7d %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            " %13"
            PRIu64
            "\n",
                    i, kni_stats[tor_id]->tx_dropped_cache[i],
                    kni_stats[tor_id]->tx_packets_cache[i],
                    kni_stats[tor_id]->tx_queued[i + tor_params_array[tor_id]->nb_dst_racks],
                    kni_stats[tor_id]->tx_dropped[i + tor_params_array[tor_id]->nb_dst_racks],
                    kni_stats[tor_id]->rx_dropped_caches[i],
                    kni_stats[tor_id]->rx_packets_caches[i]
            );

            memset(&link, 0, sizeof(link));
            rte_eth_link_get_nowait(tor_params_array[tor_id]->cache[i]->port_id, &link);

            stat = rte_eth_stats_get(tor_params_array[tor_id]->cache[i]->port_id, &stat_info);
            if (stat == 0) {
                printf(
                        "   In: %"
                PRIu64
                " (%"
                PRIu64
                " bytes)\n"
                "   Out: %"
                PRIu64
                " (%"
                PRIu64
                " bytes)\n"
                "   Err: %"
                PRIu64
                "\n"
                "   Link State: Up: %u Speed: %u Mbps\n",
                        stat_info.ipackets,
                        stat_info.ibytes,
                        stat_info.opackets,
                        stat_info.obytes,
                        stat_info.ierrors + stat_info.oerrors,
                        link.link_status, link.link_speed
                );
            } else if (stat == -ENOTSUP)
                printf("Port %i: Operation not supported\n", tor_params_array[tor_id]->cache[i]->port_id);
            else
                printf("Port %i: Error fetching statistics\n", tor_params_array[tor_id]->cache[i]->port_id);
        }
    }
    printf("======  ============  ============ ==========\n");
}

/*
 * Prints number of cycles per link and in total since the last call.
 * Resets the counters afterwards.
 */
static void print_thread_stats(void) {
    unsigned int i, loc_idx, sample_idx, tor_id;
    uint64_t sum_cycles, num_samples = 0, sum_num_samples, value;

    // printf("Go stats!\n");
    for (tor_id = 0; tor_id < num_tors; tor_id++) {
        if (!tor_params_array[tor_id]->print_cycles) {
            continue;
        }
        num_samples = circular_buf_size(thread_stats_array[tor_id]->cycles_total.value_queue);
        printf("%"
        PRIu64
        " samples\n", num_samples);
        sum_cycles = 0;
        sum_num_samples = 0;
        for (sample_idx = 0; sample_idx < num_samples; sample_idx++) {
            circular_buf_get(thread_stats_array[tor_id]->cycles_total.value_queue, &value);
            // printf("%"PRIu64"\n", value);
            sum_cycles += value;
            circular_buf_get(thread_stats_array[tor_id]->num_total.value_queue, &value);
            sum_num_samples += value;
        }
        printf("Cycles\tToR %i\tTotal\t%"
        PRIu64
        "\t%"
        PRIu64
        "\t%"
        PRIu64
        "\n", tor_id, sum_cycles, num_samples, sum_num_samples);
        for (i = 0; i < tor_params_array[tor_id]->num_rotors; i++) {
            sum_cycles = 0;
            sum_num_samples = 0;
            num_samples = circular_buf_size(thread_stats_array[tor_id]->cycles_rotor[i].value_queue);
            for (sample_idx = 0; sample_idx < num_samples; sample_idx++) {
                circular_buf_get(thread_stats_array[tor_id]->cycles_rotor[i].value_queue, &value);
                sum_cycles += value;
                circular_buf_get(thread_stats_array[tor_id]->num_rotor[i].value_queue, &value);
                sum_num_samples += value;
            }
            printf(
                    "Cycles\tToR %i\tRotor %i\t%"
            PRIu64
            "\t%"
            PRIu64
            "\t%"
            PRIu64
            "\n",
                    tor_id, i, sum_cycles, num_samples, sum_num_samples);
            for (loc_idx = 0; loc_idx < NUM_JUMP_LOCATIONS; loc_idx++) {
                num_samples = circular_buf_size(
                        thread_stats_array[tor_id]->locations_rotor[i * NUM_JUMP_LOCATIONS + loc_idx].value_queue
                );
                sum_cycles = 0;
                sum_num_samples = 0;
                for (sample_idx = 0; sample_idx < num_samples; sample_idx++) {
                    circular_buf_get(
                            thread_stats_array[tor_id]->cycle_locations_rotor[i * NUM_JUMP_LOCATIONS +
                                                                              loc_idx].value_queue,
                            &value
                    );
                    sum_cycles += value;
                    circular_buf_get(
                            thread_stats_array[tor_id]->locations_rotor[i * NUM_JUMP_LOCATIONS + loc_idx].value_queue,
                            &value
                    );
                    sum_num_samples += value;
                }
                printf(
                        "Exit \tToR %i\tRotor %i\tLocation %i\t%"
                PRIu64
                "\t%"
                PRIu64
                "\t%"
                PRIu64
                "\n",
                        tor_id, i, loc_idx, sum_cycles, sum_num_samples, num_samples);
            }
        }
        for (i = 0; i < tor_params_array[tor_id]->num_caches; i++) {
            num_samples = circular_buf_size(thread_stats_array[tor_id]->cycles_cache[i].value_queue);
            sum_cycles = 0;
            sum_num_samples = 0;
            for (sample_idx = 0; sample_idx < num_samples; sample_idx++) {
                circular_buf_get(thread_stats_array[tor_id]->cycles_cache[i].value_queue, &value);
                sum_cycles += value;
                circular_buf_get(thread_stats_array[tor_id]->num_cache[i].value_queue, &value);
                sum_num_samples += value;
            }
            printf(
                    "Cycles\tToR %i\tCache %i\t%"
            PRIu64
            "\t%"
            PRIu64
            "\t%"
            PRIu64
            "\n",
                    tor_id, i, sum_cycles, num_samples, sum_num_samples);
            for (loc_idx = 0; loc_idx < NUM_JUMP_LOCATIONS; loc_idx++) {
                num_samples = circular_buf_size(
                        thread_stats_array[tor_id]->locations_cache[i * NUM_JUMP_LOCATIONS + loc_idx].value_queue
                );
                sum_cycles = 0;
                sum_num_samples = 0;
                for (sample_idx = 0; sample_idx < num_samples; sample_idx++) {
                    circular_buf_get(
                            thread_stats_array[tor_id]->cycle_locations_cache[i * NUM_JUMP_LOCATIONS +
                                                                              loc_idx].value_queue,
                            &value
                    );
                    sum_cycles += value;
                    circular_buf_get(
                            thread_stats_array[tor_id]->locations_cache[i * NUM_JUMP_LOCATIONS + loc_idx].value_queue,
                            &value);
                    sum_num_samples += value;
                }
                printf(
                        "Exit \tToR %i\tCache %i\tLocation %i\t%"
                PRIu64
                "\t%"
                PRIu64
                "\t%"
                PRIu64
                "\n",
                        tor_id, i, loc_idx, sum_cycles, sum_num_samples, num_samples);
            }
        }

    }
}

/*
 * Prints the currently available rotor budgets
 */
static void print_rotor_budgets(void) {
    uint32_t tor_idx, rotor_id, dst_tor_idx;
    for (tor_idx = 0; tor_idx < num_tors; tor_idx++) {
        printf("====== ToR %u ====== \n", tor_idx);
        for (rotor_id = 0; rotor_id < tor_params_array[tor_idx]->num_rotors; rotor_id++) {
            printf("Rotor %u:", rotor_id);
            for (dst_tor_idx = 0; dst_tor_idx < tor_params_array[tor_idx]->nb_dst_racks; dst_tor_idx++) {
                printf("\t %u: L: %u  N: %u",
                       dst_tor_idx,
                       tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + dst_tor_idx],
                       tor_params_array[tor_idx]->nonlocal_budgets[rotor_id * MAX_NUM_NONLOCAL_QUEUES + dst_tor_idx]
                );
            }
            printf("\n");
        }
    }
}

static void
unregister_drivers(int socket_num) {
    int i, ret;

    for (i = 0; i < socket_num; i++) {
        ret = rte_vhost_driver_unregister(socket_files + i * PATH_MAX);
        if (ret != 0)
            RTE_LOG(ERR, VHOST_CONFIG,
                    "Fail to unregister vhost driver for %s.\n",
                    socket_files + i * PATH_MAX);
    }
}

/* Custom handling of signals to handle stats and kni processing */
static void
signal_handler(int signum) {
    /* When we receive a USR1 signal, print stats */
    if (signum == SIGUSR1) {
        print_stats();
    }

    /* When we receive a USR2 signal, reset stats */
    if (signum == SIGUSR2) {
        int16_t curr;
        // memset(&kni_stats, 0, sizeof(kni_stats));
        //printf("\n** Statistics have been reset **\n");
        printf("\n** Toggling cache link**\n");
        curr = rte_atomic16_read(&cache_active);
        printf("\n** Toggling cache link %i -> %i**\n", curr, 1 - curr);
        rte_atomic16_set(&cache_active, (int16_t) 1 - curr);
        return;
    }

    /* When we receive a RTMIN or SIGINT signal, stop kni processing */
    if (signum == SIGRTMIN || signum == SIGINT) {
        printf("\nSIGRTMIN/SIGINT received. KNI processing stopping.\n");
        /* Unregister vhost driver. */
        unregister_drivers(nb_sockets);
        rte_atomic32_inc(&kni_stop);
        return;
    }
}

static inline void
free_pkts(struct rte_mbuf **pkts, uint16_t n) {
    RTE_LOG(DEBUG, APP, "Free %u packets\n", n);
    while (n--)
        rte_pktmbuf_free(pkts[n]);
}

static uint32_t vhost_send_burst_all_packets(struct vhost_dev *dst_vdev, struct rte_mbuf **pkts, uint32_t num_pkts) {
    uint32_t sent = 0;
    while (1) {
        if (builtin_net_driver) {
            sent += vs_enqueue_pkts(dst_vdev, VIRTIO_RXQ, pkts + sent, num_pkts - sent);
        } else {
            sent += rte_vhost_enqueue_burst(dst_vdev->vid, VIRTIO_RXQ, pkts + sent, num_pkts - sent);
        }
        if (sent >= num_pkts) {
            return num_pkts;
        }
    }
    return num_pkts;
}

static uint32_t hash_ip_to_tor_id(uint32_t ip_addr, unsigned num_tors) {
    uint32_t ip_da;

    // Extract last byte of IP address
    ip_da = rte_be_to_cpu_32(ip_addr) & 0xFF;
    RTE_LOG(DEBUG, APP, "IP_addr & 0xFF: %u\n", ip_da);

    // Take modulo of last byte of IP as poor hash function
    return ip_da % num_tors;
}

/*
 * Route a single received packet. Check if it is indirect traffic for another ToR or should be forwarded to a host.
 */
static __rte_always_inline void ingress_route(struct rte_mbuf *pkt, uint32_t nb_dst_racks,
                                              struct rte_ring **nonlocal_queues, uint32_t this_tor_id,
                                              struct vhost_dev *local_vdev,
                                              uint64_t *rx_packets, uint64_t *rx_dropped,
                                              uint64_t *rx_packets_nonlocal) {
    unsigned num, dst_tor_id, ret;
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;

    /* We assume always Ethernet. */
    eth_hdr = rte_pktmbuf_mtod(pkt,
    struct rte_ether_hdr *);
    // RTE_LOG(INFO, APP, "Ingress route L2->len: %u\n", pkt->pkt_len);
    /* Only IPv4: that means VLAN packets are not allowed. */
    if (likely(eth_hdr->ether_type == BE_RTE_ETHER_TYPE_IPV4)) {
        ipv4_hdr = (struct rte_ipv4_hdr *) (eth_hdr + 1);
        dst_tor_id = hash_ip_to_tor_id(ipv4_hdr->dst_addr, nb_dst_racks);

        if (dst_tor_id == this_tor_id) {
            /* Local delivery */
            num = rte_vhost_enqueue_burst(local_vdev->vid, VIRTIO_RXQ, &pkt, 1);
            if (likely(num)) {
                (*rx_packets) += num;
                free_pkts(&pkt, 1);
            } else {
                /* Free mbufs not transmitted to vhost */
                free_pkts(&pkt, 1);
                (*rx_dropped) += 1;
            }
            // Packet routed. Exit.
            return;
        } else {
            RTE_LOG(DEBUG, APP, "Received nonlocal packet for %u (This ToR %u)\n", dst_tor_id, this_tor_id);

            ret = rte_ring_sp_enqueue(nonlocal_queues[dst_tor_id], (void *) pkt);
            if (likely(!ret)) {
                // RTE_LOG(INFO, APP, "Enqueued packet %#08x to non-local queue %u\n", pkt, dst_tor_id);
                rx_packets_nonlocal[dst_tor_id] += 1;
                return;
            }
            /* Forwarding not possible, count packet as dropped */
            RTE_LOG(INFO, APP, "No route for packet. Dropping.\n");
            free_pkts(&pkt, 1);
            (*rx_dropped) += 1;
        }
    }
}


/**
 * Interface to burst rx and enqueue mbufs into rx_q
 */
static void
kni_ingress(struct tor_params *p, int16_t act_matching, int tor_id) {
    uint8_t i;
    unsigned nb_rx, pkt_idx, num;
    struct rte_mbuf *pkts_burst[PKT_BURST_SZ];
    struct vhost_dev *dst_vdev;

    if (p == NULL)
        return;

    if (p->device_num == 0) {
        RTE_LOG(INFO, APP, "No vdevs for this ToR\n");
        rte_delay_ms(1000);
        return;
    }
    dst_vdev = p->vdev;

    /* (1) Fetch and forward packets from DA links */
    for (i = 0; i < p->num_caches; i++) {
        /* Burst rx from eth */
        nb_rx = rte_eth_rx_burst(p->cache[i]->port_id, 0, pkts_burst, PKT_BURST_SZ);
        if (unlikely(nb_rx > PKT_BURST_SZ)) {
            RTE_LOG(ERR, APP, "Error receiving from cache\n");
            return;
        }

        num = vhost_send_burst_all_packets(dst_vdev, pkts_burst, nb_rx);

        if (num)
            // RTE_LOG(INFO, APP, "Forwarded %u packets to KNI\n", num);
            kni_stats[tor_id]->rx_packets_caches[i] += num;

        if (unlikely(num < nb_rx)) {
            /* Free mbufs not tx to kni interface */
            free_pkts(&pkts_burst[num], nb_rx - num);
            kni_stats[tor_id]->rx_dropped_caches[i] += nb_rx - num;
        }
        free_pkts(pkts_burst, nb_rx);
    }

    nb_rx = 0;
    num = 0;

    /* (2) Fetch packets from Rotors */
    for (i = 0; i < p->num_rotors; i++) {
        /* Burst rx from eth */
        nb_rx = rte_eth_rx_burst(p->rotors[i]->port_id, 0, pkts_burst, PKT_BURST_SZ);
        if (unlikely(nb_rx > PKT_BURST_SZ)) {
            RTE_LOG(ERR, APP, "Error receiving from eth\n");
            return;
        }

        for (pkt_idx = 0; pkt_idx < nb_rx; pkt_idx++) {
            ingress_route(pkts_burst[pkt_idx], p->nb_dst_racks,
                          p->nonlocal_queues, p->id, dst_vdev,
                          &kni_stats[tor_id]->rx_packets_rotors[i], &kni_stats[tor_id]->rx_dropped_rotors[i],
                          kni_stats[tor_id]->rx_packets_rotors_nonlocal);
        }
    }
}

/**
 * Takes a packet and returns the array idx of the corresponding dst queue
 * Takes the last byte of the destination IP address and uses this as idx for queue
 * @param packet
 * @return
 */
static int32_t get_dst_qid(struct rte_mbuf *packet, struct tor_params *p) {
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_udp_hdr *tp_hdr;
    struct rte_tcp_hdr *tp_tcp_hdr;
    uint32_t qid;
    uint16_t tp_dst, i;

    /* We assume always Ethernet. */
    eth_hdr = rte_pktmbuf_mtod(packet,
    struct rte_ether_hdr *);

    /* Only IPv4: that means VLAN packets are not allowed. */
    if (likely(eth_hdr->ether_type == BE_RTE_ETHER_TYPE_IPV4)) {
        ipv4_hdr = (struct rte_ipv4_hdr *) (eth_hdr + 1);

        qid = hash_ip_to_tor_id(ipv4_hdr->dst_addr, p->nb_dst_racks);

        /* Check dst port of packet */
        if (ipv4_hdr->next_proto_id == IPPROTO_UDP) {
            tp_hdr = (struct rte_udp_hdr *) ((unsigned char *) ipv4_hdr + sizeof(struct rte_ipv4_hdr));
            tp_dst = rte_be_to_cpu_16(tp_hdr->dst_port);
        } else if (ipv4_hdr->next_proto_id == IPPROTO_TCP) {
            tp_tcp_hdr = (struct rte_tcp_hdr *) ((unsigned char *) ipv4_hdr + sizeof(struct rte_ipv4_hdr));
            tp_dst = rte_be_to_cpu_16(tp_tcp_hdr->dst_port);
        } else if (ipv4_hdr->next_proto_id == IPPROTO_ICMP) {
            // ICMP goes to Rotor
            return qid;
        } else {
            return -1;
        }
        for (i = 0; i < p->num_caches; i++) {
            // check if flow should go to cache -- queues for cache links are just after the VOQs
            if (p->cache[i]->mappings[qid]->all_flows_to_cache) {
                return i + p->nb_dst_racks;
            }
            if (p->cache[i]->mappings[qid]->flows_to_cache[tp_dst]) {
                return i + p->nb_dst_racks;
            }
        }

        // Forward packet to rotor VOQ
        return qid;
    }
    return -1;
}

/**
 * Puts packet to local queues and updates statistics.
 *
 * @param packet
 */
static void map_local_dst_to_queue(struct rte_mbuf *packet, struct tor_params *p, uint16_t tor_id) {
    int ret;
    int32_t qid;

    qid = get_dst_qid(packet, p);
    // RTE_LOG(WARNING, APP, "ToR %u Qid: %u\n", tor_id, qid);
    if (likely(qid >= 0)) {
        // put packet to queue
        ret = rte_ring_sp_enqueue(p->queues[qid], (void *) packet);
        if (likely(!ret)) {
            kni_stats[tor_id]->tx_queued[qid] += 1;
        } else {
            /* Free mbufs not tx to queue */
            free_pkts(&packet, 1);
            kni_stats[tor_id]->tx_dropped[qid] += 1;
        }
    } else {
        /* Free mbufs not tx to NIC */
        free_pkts(&packet, 1);
        kni_stats[tor_id]->tx_dropped[qid] += 1;
    }
}

static void link_queues(struct vhost_dev *vdev, struct rte_mbuf *packet, struct rx_thread_params *p) {
    struct rte_ether_hdr *pkt_hdr;
    int tor_id;

    /* Learn MAC address of guest device from packet */
    pkt_hdr = rte_pktmbuf_mtod(packet,
    struct rte_ether_hdr *);

    /* Take the last byte of the mac address to get VM id */
    tor_id = pkt_hdr->s_addr.addr_bytes[5] - 1;   // Last byte = 1 means VM 1 means ToR 0

    p->tor_id = tor_id;
    tor_params_array[tor_id]->vdev = vdev;
    tor_params_array[tor_id]->device_num = 1;
    RTE_LOG(INFO, APP, "Linking vdev %u to ToR %u\n", vdev->vid, p->tor_id);
    vdev->ready = DEVICE_RX;
}


/**
 * Interface to dequeue mbufs from tx_q and enqueues
 */
static void
egress_stage1(struct rx_thread_params *p) {
    uint8_t j;
    unsigned num = 0;
    struct rte_mbuf *pkts_burst[PKT_BURST_SZ]
    __rte_cache_aligned;
    struct vhost_dev *vdev;

    if (p == NULL)
        return;

    /*
    * Process vhost devices
    */
    TAILQ_FOREACH(vdev, &p->vdev_list,
                  lcore_vdev_entry)
    {
        if (unlikely(vdev->remove)) {
            vdev->ready = DEVICE_SAFE_REMOVE;
            continue;
        }

        if (likely(!vdev->remove)) {
            if (builtin_net_driver) {
                num = vs_dequeue_pkts(vdev, VIRTIO_TXQ, pktmbuf_pool,
                                      pkts_burst, PKT_BURST_SZ);
            } else {
                num = rte_vhost_dequeue_burst(vdev->vid, VIRTIO_TXQ,
                                              pktmbuf_pool, pkts_burst, PKT_BURST_SZ);
            }
        }

        if (unlikely(vdev->ready == DEVICE_MAC_LEARNING) && num) {
            // Find ToR id
            link_queues(vdev, pkts_burst[0], p);
        }

        if (unlikely(num > PKT_BURST_SZ)) {
            RTE_LOG(ERR, APP, "Error receiving from KNI\n");
            return;
        }
        if (num) {
            RTE_LOG(DEBUG, APP, "Fetched %u packets, %i \n", num, vdev->vid);
        }
        /* Assign to tx queues */
        for (j = 0; j < num; j++) {
            map_local_dst_to_queue(pkts_burst[j], tor_params_array[p->tor_id], p->tor_id);
        }
    }
}

static uint16_t send_burst_all_packets(uint8_t port_id, uint16_t queue_id, struct rte_mbuf **pkts, uint16_t num_pkts) {
    uint32_t sent = 0;
    if (!num_pkts) {
        return 0;
    }
    while (1) {
        sent += rte_eth_tx_burst(port_id, queue_id, pkts + sent, num_pkts - sent);
        if (sent >= num_pkts) {
            return num_pkts;
        }
    }
    return num_pkts;
}

/**
 * Collect cycle sample. Statistics on how packets are processed.
 */
static void
collect_cycle_sample(uint64_t loop_cycle_start, uint16_t tor_id, uint16_t link_id, bool cache, uint16_t location) {
    unsigned int i;
    uint64_t cycle_value;
    cycle_value = rte_rdtsc() - loop_cycle_start;
    if (!cache) {
        // printf("Write rotor cycle sample\n");
        thread_stats_array[tor_id]->cycles_rotor[link_id].temp_value += cycle_value;
        thread_stats_array[tor_id]->num_rotor[link_id].temp_value += 1;
        thread_stats_array[tor_id]->cycle_locations_rotor[location +
                                                          NUM_JUMP_LOCATIONS * link_id].temp_value += cycle_value;
        thread_stats_array[tor_id]->locations_rotor[location + NUM_JUMP_LOCATIONS * link_id].temp_value += 1;
        if (thread_stats_array[tor_id]->num_rotor[link_id].temp_value > NUM_TEMP_CYCLE_SAMPLES) {
            circular_buf_put(
                    thread_stats_array[tor_id]->cycles_rotor[link_id].value_queue,
                    thread_stats_array[tor_id]->cycles_rotor[link_id].temp_value
            );
            circular_buf_put(
                    thread_stats_array[tor_id]->num_rotor[link_id].value_queue,
                    thread_stats_array[tor_id]->num_rotor[link_id].temp_value
            );
            thread_stats_array[tor_id]->cycles_rotor[link_id].temp_value = 0;
            thread_stats_array[tor_id]->num_rotor[link_id].temp_value = 0;

            for (i = 0; i < NUM_JUMP_LOCATIONS; i++) {
                circular_buf_put(
                        thread_stats_array[tor_id]->locations_rotor[i + NUM_JUMP_LOCATIONS * link_id].value_queue,
                        thread_stats_array[tor_id]->locations_rotor[i + NUM_JUMP_LOCATIONS * link_id].temp_value
                );
                thread_stats_array[tor_id]->locations_rotor[i + NUM_JUMP_LOCATIONS * link_id].temp_value = 0;
                circular_buf_put(
                        thread_stats_array[tor_id]->cycle_locations_rotor[i + NUM_JUMP_LOCATIONS * link_id].value_queue,
                        thread_stats_array[tor_id]->cycle_locations_rotor[i + NUM_JUMP_LOCATIONS * link_id].temp_value
                );
                thread_stats_array[tor_id]->cycle_locations_rotor[i + NUM_JUMP_LOCATIONS * link_id].temp_value = 0;
            }
        }
    } else {
        thread_stats_array[tor_id]->cycles_cache[link_id].temp_value += cycle_value;
        thread_stats_array[tor_id]->num_cache[link_id].temp_value += 1;
        thread_stats_array[tor_id]->cycle_locations_cache[location +
                                                          NUM_JUMP_LOCATIONS * link_id].temp_value += cycle_value;
        thread_stats_array[tor_id]->locations_cache[location + NUM_JUMP_LOCATIONS * link_id].temp_value += 1;
        if (thread_stats_array[tor_id]->num_cache[link_id].temp_value > NUM_TEMP_CYCLE_SAMPLES) {
            circular_buf_put(
                    thread_stats_array[tor_id]->cycles_cache[link_id].value_queue,
                    thread_stats_array[tor_id]->cycles_cache[link_id].temp_value
            );
            circular_buf_put(
                    thread_stats_array[tor_id]->num_cache[link_id].value_queue,
                    thread_stats_array[tor_id]->num_cache[link_id].temp_value
            );
            thread_stats_array[tor_id]->cycles_cache[link_id].temp_value = 0;
            thread_stats_array[tor_id]->num_cache[link_id].temp_value = 0;

            for (i = 0; i < NUM_JUMP_LOCATIONS; i++) {
                circular_buf_put(
                        thread_stats_array[tor_id]->locations_cache[i + NUM_JUMP_LOCATIONS * link_id].value_queue,
                        thread_stats_array[tor_id]->locations_cache[i + NUM_JUMP_LOCATIONS * link_id].temp_value
                );
                thread_stats_array[tor_id]->locations_cache[i + NUM_JUMP_LOCATIONS * link_id].temp_value = 0;
                circular_buf_put(
                        thread_stats_array[tor_id]->cycle_locations_cache[i + NUM_JUMP_LOCATIONS * link_id].value_queue,
                        thread_stats_array[tor_id]->cycle_locations_cache[i + NUM_JUMP_LOCATIONS * link_id].temp_value
                );
                thread_stats_array[tor_id]->cycle_locations_cache[i + NUM_JUMP_LOCATIONS * link_id].temp_value = 0;
            }
        }
    }
}


void tag_packet(struct rte_mbuf *pkt, uint16_t vlan_id, uint64_t loop_cycle_start, unsigned tor_id,
                unsigned rotor_id, uint64_t cycles_per_period) {
    struct rte_ether_hdr *oh, *nh;
    struct vlan_hdr vlan_tag = {
            .eth_type = BE_RTE_ETHER_TYPE_VLAN,
            .vlan_id = rte_cpu_to_be_16(vlan_id)
    };

    /* oh = old header, nh = new header */
    oh = rte_pktmbuf_mtod(pkt,
    struct rte_ether_hdr *);
    /* Make space in front */
    nh = (struct rte_ether_hdr *) rte_pktmbuf_prepend(pkt, sizeof(struct rte_vlan_hdr));
    if (nh == NULL) {
        RTE_LOG(ERR, APP, "Error extending header - not enough space.\n");
        /* Collect sample here with location id 4 */
        collect_cycle_sample(loop_cycle_start, tor_id, rotor_id, false, 4);
        if (cycles_per_period) {
            decrease_shaper(
                    loop_cycle_start,
                    &remaining_cycles[tor_id * MAX_NUM_LINKS + rotor_id]);
        }
        return;
    }

    /* Copy the (first part of) the Ethernet header at its new place (oh->nh) */
    memmove(nh, oh, 2 * RTE_ETHER_ADDR_LEN);

    /* Copy list of tags after source and destination MAC */
    rte_memcpy(&(nh->ether_type), &vlan_tag, sizeof(struct vlan_hdr));

    RTE_LOG(DEBUG, APP, "Current tag %u\n", vlan_id);
    /* Update packet length */
    pkt->ol_flags &= ~(PKT_RX_VLAN_STRIPPED | PKT_TX_VLAN);
    if (pkt->ol_flags & PKT_TX_TUNNEL_MASK)
        pkt->outer_l2_len += sizeof(struct rte_vlan_hdr);
    else
        pkt->l2_len += sizeof(struct rte_vlan_hdr);
    RTE_LOG(DEBUG, APP, "Extended header\n");
}

int32_t drain_queue_by_budget(uint32_t port_id, struct rte_ring *queue, int32_t budget, int16_t act_matching,
                              uint64_t *stats_tx_queue, uint64_t *stats_tx_rotor, uint64_t *stats_dropped_rotor,
                              uint64_t loop_cycle_start,
                              unsigned tor_id, unsigned rotor_id, struct tor_params *p) {
    unsigned nb_tx, num, pkt_idx, total_sent = 0;
    struct rte_mbuf *pkts_burst[PKT_BURST_SZ];
    unsigned burst = PKT_BURST_SZ;
    if (budget < PKT_BURST_SZ) {
        burst = budget;
    }
    if (budget == 0) {
        collect_cycle_sample(loop_cycle_start, tor_id, rotor_id, false, 3);
        if (p->cycles_per_period) {
            decrease_shaper(
                    loop_cycle_start,
                    &remaining_cycles[tor_id * MAX_NUM_LINKS + rotor_id]);
        }
        return 0;
    }

    /* Read burst from queue */
    num = rte_ring_sc_dequeue_burst(queue, (void **) pkts_burst,
                                    burst, NULL);
    if (unlikely(num > burst)) {
        /* Location id 3 -- Rotor queue drain failed */
        collect_cycle_sample(loop_cycle_start, tor_id, rotor_id, false, 3);
        RTE_LOG(ERR, APP, "Error dequeuing\n");
        if (p->cycles_per_period) {
            decrease_shaper(
                    loop_cycle_start,
                    &remaining_cycles[tor_id * MAX_NUM_LINKS + rotor_id]);
        }
        return -1;
    }

    for (pkt_idx = 0; pkt_idx < num; pkt_idx++) {
        tag_packet(pkts_burst[pkt_idx], act_matching, loop_cycle_start, tor_id, rotor_id, p->cycles_per_period);
    }
    if (num)
        RTE_LOG(DEBUG, APP, "Tagged packets\n");
    /* Burst tx to eth */
    nb_tx = send_burst_all_packets(port_id, 0, pkts_burst, (uint16_t) num);
    RTE_LOG(DEBUG, APP, "Sent %u packets on port %u\n", nb_tx, port_id);

    if (nb_tx) {
        // RTE_LOG(INFO, APP, "Sent %u packets on port %u\n", nb_tx, port_id);
        (*stats_tx_queue) += nb_tx;
        (*stats_tx_rotor) += nb_tx;
        total_sent += nb_tx;
    }
    if (unlikely(nb_tx < num)) {
        /* Free mbufs not tx to NIC */
        free_pkts(&pkts_burst[nb_tx], num - nb_tx);
        (*stats_dropped_rotor) += num - nb_tx;
    }
    /* Collect sample here with location id 5 */
    collect_cycle_sample(loop_cycle_start, tor_id, rotor_id, false, 5);
    if (p->cycles_per_period) {
        decrease_shaper(
                loop_cycle_start,
                &remaining_cycles[tor_id * MAX_NUM_LINKS + rotor_id]);
    }
    return total_sent;
}


/**
 * Reads from the egress queue and sends to eth dev
 */
static void
egress_stage2(struct tor_params *p, int16_t act_matching, uint16_t tor_id,
              uint32_t *local_budget, uint32_t *nonlocal_budget) {

    unsigned nb_tx, num, rotor_id, port_id, dst_tor_idx, qid, cache_id, current_budget, drained_pkts;
    struct rte_mbuf *pkts_burst[PKT_BURST_SZ];
    uint64_t loop_cycle_start;

    if (p == NULL)
        return;

    /* (1) transmit burst via cache link */
    for (cache_id = 0; cache_id < p->num_caches; cache_id++) {
        loop_cycle_start = rte_rdtsc();
        // Check if cycles are available
        if (p->cycles_per_period && remaining_cycles[tor_id * MAX_NUM_LINKS + MAX_NUM_ROTORS + cache_id] == 0) {
            // No cycles left for this link
            continue;
        }

        port_id = p->cache[cache_id]->port_id;
        qid = p->nb_dst_racks + cache_id;
        num = rte_ring_sc_dequeue_burst(p->queues[qid], (void **) pkts_burst,
                                        PKT_BURST_SZ, NULL);
        if (unlikely(num > PKT_BURST_SZ)) {
            /* Collect sample here with location id 0 */
            collect_cycle_sample(loop_cycle_start, tor_id, cache_id, true, 0);
            RTE_LOG(ERR, APP, "Error dequeuing for cache\n");
            return;
        }
        nb_tx = send_burst_all_packets(port_id, 0, pkts_burst, (uint16_t) num);

        if (nb_tx) {
            kni_stats[tor_id]->tx_packets_cache[cache_id] += nb_tx;
        }
        if (unlikely(nb_tx < num)) {
            /* Free mbufs not tx to NIC */
            free_pkts(&pkts_burst[nb_tx], num - nb_tx);
            kni_stats[tor_id]->tx_dropped_cache[cache_id] += num - nb_tx;
        }
        /* Collect sample here with location id 1 */
        collect_cycle_sample(loop_cycle_start, tor_id, cache_id, true, 1);
        if (p->cycles_per_period) {
            decrease_shaper(loop_cycle_start,
                            &remaining_cycles[tor_id * MAX_NUM_LINKS + MAX_NUM_ROTORS + cache_id]);
        }
    }

    num = 0;
    nb_tx = 0;

    /* (2) transmit burst via rotor links */
    for (rotor_id = 0; rotor_id < p->num_rotors; rotor_id++) {
        loop_cycle_start = rte_rdtsc();
        // Check if cycles are available
        if (p->cycles_per_period && remaining_cycles[tor_id * MAX_NUM_LINKS + rotor_id] == 0) {
            // No cycles left for this link
            continue;
        }
        port_id = p->rotors[rotor_id]->port_id;

        if (act_matching <= MIN_MATCHING_VLAN_ID) {
            // Empty matching, i.e., open circuit
            /* Collect sample here with location id 2 -- Empty machting */
            collect_cycle_sample(loop_cycle_start, tor_id, rotor_id, false, 2);
            if (p->cycles_per_period) {
                decrease_shaper(
                        loop_cycle_start,
                        &remaining_cycles[tor_id * MAX_NUM_LINKS + rotor_id]
                );
            }
            continue;
        }

        /* (2.1) Sent traffic from nonlocal queues */
        for (dst_tor_idx = 0; dst_tor_idx < p->nb_dst_racks; dst_tor_idx++) {
            current_budget = nonlocal_budget[rotor_id * MAX_NUM_NONLOCAL_QUEUES + dst_tor_idx];
            if (current_budget) {
                RTE_LOG(DEBUG, APP, "ToR %u Budget for nonlocal queue %u is %u\n", tor_id, dst_tor_idx, current_budget);
                RTE_LOG(DEBUG, APP, "Pkts in queue: %u\n", rte_ring_count(p->nonlocal_queues[dst_tor_idx]));
            }
            drained_pkts = drain_queue_by_budget(
                    port_id,
                    p->nonlocal_queues[dst_tor_idx],
                    (int32_t) current_budget,
                    act_matching,
                    &kni_stats[tor_id]->tx_packets_nonlocal[dst_tor_idx],
                    &kni_stats[tor_id]->tx_packets_rotors[rotor_id],
                    &kni_stats[tor_id]->tx_dropped_rotors[rotor_id],
                    loop_cycle_start,
                    tor_id, rotor_id, p
            );
            nonlocal_budget[rotor_id * MAX_NUM_NONLOCAL_QUEUES + dst_tor_idx] = current_budget - drained_pkts;
            if (drained_pkts)
                RTE_LOG(INFO, APP, "Sent %u non-local packets\n", drained_pkts);
        }

        /* (2.2) Handle traffic from local queues (direct) */
        for (dst_tor_idx = 0; dst_tor_idx < p->nb_dst_racks; dst_tor_idx++) {
            current_budget = local_budget[rotor_id * MAX_NUM_LOCAL_QUEUES + dst_tor_idx];
            // RTE_LOG(INFO, APP, "Local: Rotor: %u Qid: %u Budget: %u\n", rotor_id, dst_tor_idx, current_budget);
            drained_pkts = drain_queue_by_budget(
                    port_id,
                    p->queues[dst_tor_idx],
                    (int32_t) current_budget,
                    act_matching,
                    &kni_stats[tor_id]->tx_packets[dst_tor_idx],
                    &kni_stats[tor_id]->tx_packets_rotors[rotor_id],
                    &kni_stats[tor_id]->tx_dropped_rotors[rotor_id],
                    loop_cycle_start, tor_id, rotor_id, p
            );
            local_budget[rotor_id * MAX_NUM_LOCAL_QUEUES + dst_tor_idx] = current_budget - drained_pkts;
            if (drained_pkts) {
                RTE_LOG(INFO, APP, "Queue %u drained with %u packets\n", dst_tor_idx, drained_pkts);
                RTE_LOG(INFO, APP, "Remaining budget %u\n",
                        local_budget[rotor_id * MAX_NUM_LOCAL_QUEUES + dst_tor_idx]);
            }
        }
    }
}

static void assign_remaining_budget_longest_queue(int16_t act_matching) {
    int16_t lock_budget;
    uint32_t tor_idx, rotor_id, qid, this_nonlocal_count, this_local_count, max_local_count, max_dst_id, dst_id;
    int32_t remaining_budget;
    if (act_matching <= MIN_MATCHING_VLAN_ID) {
        return;
    }

    for (tor_idx = 0; tor_idx < num_tors; tor_idx++) {
        // Reset all budgets to 0.
        memset(tor_params_array[tor_idx]->local_budgets, 0, sizeof(tor_params_array[tor_idx]->local_budgets));
        memset(tor_params_array[tor_idx]->nonlocal_budgets, 0, sizeof(tor_params_array[tor_idx]->nonlocal_budgets));
        for (rotor_id = 0; rotor_id < tor_params_array[tor_idx]->num_rotors; rotor_id++) {
            remaining_budget = (int32_t) budget_per_slot;
            // RTE_LOG(INFO, APP, "Remaining budget %i\n", remaining_budget);
            qid = tor_params_array[tor_idx]->rotors[rotor_id]->vid_to_qid_mapping[act_matching];
            // RTE_LOG(INFO, APP, "QID: %u\n", qid);

            // Subtract nonlocal (2nd hop)
            this_nonlocal_count = rte_ring_count(
                    tor_params_array[tor_idx]->nonlocal_queues[qid]
            );
            remaining_budget -= this_nonlocal_count;
            // RTE_LOG(INFO, APP, "Remaining budget after nonlocal R%u %i\n", rotor_id, remaining_budget);
            if (remaining_budget <= 0) {
                tor_params_array[tor_idx]->nonlocal_budgets[rotor_id * MAX_NUM_NONLOCAL_QUEUES + qid] =
                        ((int32_t) this_nonlocal_count) + remaining_budget;
                // All budget used, continue with next rotor
                continue;
            } else {
                tor_params_array[tor_idx]->nonlocal_budgets[rotor_id * MAX_NUM_NONLOCAL_QUEUES + qid] =
                        this_nonlocal_count;
            }

            // Subtract local
            this_local_count = rte_ring_count(tor_params_array[tor_idx]->queues[qid]);
            remaining_budget -= this_local_count;
            // RTE_LOG(INFO, APP, "Remaining budget after local R%u %i\n", rotor_id, remaining_budget);
            if (remaining_budget <= 0) {
                tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + qid] =
                        ((int32_t) this_local_count) + remaining_budget;
                // All budget used, continue with next rotor
                continue;
            } else {
                tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + qid] = this_local_count;
            }

            // We have budget left. Give it to next higher qid -- this might lead to congestion on
            // the intermediate ToR.
            max_dst_id = 0;
            max_local_count = rte_ring_count(tor_params_array[tor_idx]->queues[max_dst_id]);
            // Search for longest queue
            for (dst_id = 1; dst_id < tor_params_array[tor_idx]->nb_dst_racks; dst_id++) {
                if (dst_id == qid) {
                    continue;
                }
                this_local_count = rte_ring_count(tor_params_array[tor_idx]->queues[dst_id]);
                if (this_local_count > max_local_count) {
                    max_dst_id = dst_id;
                    max_local_count = this_local_count;
                }
            }
            tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + max_dst_id] =
                    (uint32_t) remaining_budget * 0.2;

        }
        lock_budget = rte_atomic16_read(&tor_params_array[tor_idx]->lock_budget);
        rte_atomic16_set(&tor_params_array[tor_idx]->lock_budget, 1 - lock_budget);
    }
}

static void assign_fixed_budget(int16_t act_matching) {
    int16_t lock_budget;
    uint32_t tor_idx, rotor_id, dst_tor_idx, qid;
    if (act_matching <= MIN_MATCHING_VLAN_ID) {
        return;
    }
    /* For now, assign all budget to the direct matching */
    for (tor_idx = 0; tor_idx < num_tors; tor_idx++) {
        // Reset all budgets to 0.
        memset(tor_params_array[tor_idx]->local_budgets, 0, sizeof(tor_params_array[tor_idx]->local_budgets));
        memset(tor_params_array[tor_idx]->nonlocal_budgets, 0, sizeof(tor_params_array[tor_idx]->nonlocal_budgets));
        for (rotor_id = 0; rotor_id < tor_params_array[tor_idx]->num_rotors; rotor_id++) {
            qid = tor_params_array[tor_idx]->rotors[rotor_id]->vid_to_qid_mapping[act_matching];
            for (dst_tor_idx = 0; dst_tor_idx < tor_params_array[tor_idx]->nb_dst_racks; dst_tor_idx++) {
                if (dst_tor_idx == qid) {
                    // Use only 80% of budget
                    tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + dst_tor_idx] =
                            (uint32_t) budget_per_slot * 0.8;
                    // And another 10% evenly among non local traffic
                    tor_params_array[tor_idx]->nonlocal_budgets[rotor_id * MAX_NUM_NONLOCAL_QUEUES + dst_tor_idx] =
                            (uint32_t) budget_per_slot * 0.1;
                } else {
                    // Distribute remaining 10% among evenly among others
                    tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + dst_tor_idx] =
                            (uint32_t) budget_per_slot * 0.1 / (tor_params_array[tor_idx]->nb_dst_racks - 1);
                    tor_params_array[tor_idx]->nonlocal_budgets[rotor_id * MAX_NUM_NONLOCAL_QUEUES + dst_tor_idx] = 0;
                }
            }
        }
        lock_budget = rte_atomic16_read(&tor_params_array[tor_idx]->lock_budget);
        rte_atomic16_set(&tor_params_array[tor_idx]->lock_budget, 1 - lock_budget);
    }
}

static void assign_only_direct(int16_t act_matching) {
    int16_t lock_budget;
    uint32_t tor_idx, rotor_id, qid;
    /* For now, assign all budget to the direct matching */
    for (tor_idx = 0; tor_idx < num_tors; tor_idx++) {
        // Reset all budgets to 0.
        memset(tor_params_array[tor_idx]->local_budgets, 0, sizeof(tor_params_array[tor_idx]->local_budgets));
        memset(tor_params_array[tor_idx]->nonlocal_budgets, 0, sizeof(tor_params_array[tor_idx]->nonlocal_budgets));
        if (act_matching <= MIN_MATCHING_VLAN_ID) {
            continue;
        }
        for (rotor_id = 0; rotor_id < tor_params_array[tor_idx]->num_rotors; rotor_id++) {
            qid = tor_params_array[tor_idx]->rotors[rotor_id]->vid_to_qid_mapping[act_matching];
            tor_params_array[tor_idx]->local_budgets[rotor_id * MAX_NUM_LOCAL_QUEUES + qid] =
                    (uint32_t) budget_per_slot;
        }
        lock_budget = rte_atomic16_read(&tor_params_array[tor_idx]->lock_budget);
        rte_atomic16_set(&tor_params_array[tor_idx]->lock_budget, 1 - lock_budget);
    }
}


/*
 * Process frame from clock generator. Update the active matching and the budgets
 */
static void process_sync_frame(struct rte_mbuf *pkt) {
    char *payload;

    payload = rte_pktmbuf_mtod_offset(
            pkt, char *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr)
    );
    if ((payload[1] & 0x000000FF) != 0xAA) {
        RTE_LOG(INFO, APP, "Received not a sync frame, %x\n", payload[0]);
        RTE_LOG(INFO, APP, "Received not a sync frame, %x\n", payload[1] & 0x000000FF);
        RTE_LOG(INFO, APP, "Received not a sync frame, %x\n", payload[2]);
        return;
    }
    RTE_LOG(DEBUG, APP, "Received sync frame\n");
    rte_atomic16_set(&active_matching, (int16_t) payload[2]);

    // Update budgets for indirect
    switch (indirect_routing) {
        case BUDGET_ALLOCATION_ONLY_DIRECT:
            assign_only_direct((int16_t) payload[2]);
            break;
        case BUDGET_ALLOCATION_INDIRECT_MAX:
            assign_remaining_budget_longest_queue((int16_t) payload[2]);
            break;
        case BUDGET_ALLOCATION_INDIRECT_FIXED:
            assign_fixed_budget((int16_t) payload[2]);
            break;
        default:
            RTE_LOG(WARNING, APP, "No budget allocation specified");
            break;
    }

}

/*
 * Processes a cplane frame. Example
 * Bytes: XX YY WW ZZ ZZ
 * XX -> Action, YY -> ToR id, WW -> cache links id, ZZZZ -> dst port
 */
static void process_cplane_frame(struct rte_mbuf *pkt) {
    uint8_t action, tor_id, cache_id, dst_tor_id;
    uint16_t dst_port;
    unsigned idx;
    char *payload;
    payload = rte_pktmbuf_mtod_offset(pkt,
    char *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));
    action = payload[0];
    tor_id = payload[1];
    for (idx = 0; idx < num_tors; idx++) {
        if (tor_params_array[idx]->id == tor_id) {
            tor_id = idx;
            goto tor_match;
        }
    }
    return;

    tor_match:
    cache_id = payload[2];
    if (unlikely(cache_id >= tor_params_array[tor_id]->num_caches)) {
        RTE_LOG(INFO, APP, "Invalid cache id (too high)\n");
        return;
    }
    dst_tor_id = payload[3];
    if (unlikely(dst_tor_id >= tor_params_array[tor_id]->nb_dst_racks)) {
        RTE_LOG(INFO, APP, "Invalid dst tor id (too high)\n");
        return;
    }
    dst_port = (payload[4] << 8) + payload[5];
    RTE_LOG(INFO, APP, "Action: %u, ToR: %u, Cache: %u, dst_tor_id: %u, dst_port: %u\n", action, tor_id, cache_id,
            dst_tor_id, dst_port);
    switch (action) {
        case ADD_FLOW:
            RTE_LOG(INFO, APP, "Adding flow to ToR %u on  port %u on ToR %u to cache %u\n", dst_tor_id, dst_port,
                    tor_id, cache_id);
            if (dst_port == 0) {
                tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->all_flows_to_cache = 1;
            } else {
                tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->flows_to_cache[dst_port] = 1;
            }
            break;
        case REMOVE_FLOW:
            RTE_LOG(INFO, APP, "Removing flow to ToR %u on port %u on ToR %u to cache %u\n", dst_tor_id, dst_port,
                    tor_id, cache_id);
            if (dst_port == 0) {
                tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->all_flows_to_cache = 0;
            } else {
                tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->flows_to_cache[dst_port] = 0;
            }
            break;
        case CLEAR_CACHE:
            RTE_LOG(INFO, APP, "Clearing cache %u on ToR %u for traffic to ToR %u\n", tor_id, cache_id, dst_tor_id);
            memset(tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->flows_to_cache, 0,
                   sizeof(tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->flows_to_cache));
            tor_params_array[tor_id]->cache[cache_id]->mappings[dst_tor_id]->all_flows_to_cache = 0;
            break;
        default:
            RTE_LOG(INFO, APP, "Unknown action\n");
    }
}

/**
 * Reads from sync port and updates active matching, process DA control plane
 */
static void
sync_thread(struct tor_params *p) {
    unsigned nb_rx, pkt_id;
    struct rte_mbuf *pkt[PKT_BURST_SZ];

    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_udp_hdr *tp_hdr;

    nb_rx = rte_eth_rx_burst(p->sync_port, 0, pkt, PKT_BURST_SZ);

    for (pkt_id = 0; pkt_id < nb_rx; pkt_id++) {
        RTE_LOG(DEBUG, APP, "Received packet\n");
        /* We assume always Ethernet. */
        eth_hdr = rte_pktmbuf_mtod(pkt[pkt_id],
        struct rte_ether_hdr *);

        /* Only IPv4: that means VLAN packets are not allowed. */
        if (eth_hdr->ether_type != BE_RTE_ETHER_TYPE_IPV4) {
            RTE_LOG(INFO, APP, "Not an IPv4 packet, header is %x\n", rte_be_to_cpu_16(eth_hdr->ether_type));
            return;
        }
        ipv4_hdr = (struct rte_ipv4_hdr *) (eth_hdr + 1);
        /* Only UDP. */
        if (unlikely(ipv4_hdr->next_proto_id != IPPROTO_UDP)) {
            RTE_LOG(INFO, APP, "Not an UDP packet\n");
            return;
        }
        tp_hdr = (struct rte_udp_hdr *) ((unsigned char *) ipv4_hdr + sizeof(struct rte_ipv4_hdr));

        if (likely(rte_be_to_cpu_16(tp_hdr->dst_port) == SYNC_UDP_PORT)) {
            process_sync_frame(pkt[pkt_id]);
            return;
        }

        if (rte_be_to_cpu_16(tp_hdr->dst_port) == CPLANE_UDP_PORT) {
            process_cplane_frame(pkt[pkt_id]);
            return;
        }

        RTE_LOG(INFO, APP, "Wrong dst port %u\n", rte_be_to_cpu_16(tp_hdr->dst_port));
    }
}

/*
 * The main loop for the threads. Different branches for the threads, which might also call sub-functions.
 */
static int main_loop(__rte_unused void *arg) {
    uint16_t i;
    int16_t act_matching = 0;
    int32_t f_stop;
    const unsigned lcore_id = rte_lcore_id();

    uint64_t loop_cycle_start, shaper_cycle_start;

    enum lcore_rxtx {
        LCORE_NONE,
        LCORE_RX,
        LCORE_TX,
        LCORE_TX_2,
        LCORE_SYNC,
        LCORE_MAX
    };
    enum lcore_rxtx flag = LCORE_NONE;

    // First, identify what this thread shall do.
    for (i = 0; i < MAX_NUM_TORS; i++) {
        RTE_LOG(INFO, APP, "main loop %u %u\n", lcore_id, i);
        if (!tor_params_array[i])
            continue;
        if (tor_params_array[i]->lcore_rx == (uint8_t) lcore_id) {
            flag = LCORE_RX;
            break;
        } else if (tor_params_array[i]->lcore_tx ==
                   (uint8_t) lcore_id) {
            flag = LCORE_TX;
            break;
        } else if (tor_params_array[i]->lcore_tx2 ==
                   (uint8_t) lcore_id) {
            flag = LCORE_TX_2;
            break;
        } else if (tor_params_array[i]->lcore_sync ==
                   (uint8_t) lcore_id) {
            flag = LCORE_SYNC;
            break;
        }
    }

    if (flag == LCORE_RX) {
        RTE_LOG(INFO, APP, "Lcore %u is reading for ToR %u\n",
                tor_params_array[i]->lcore_rx, i);

        while (1) {
            f_stop = rte_atomic32_read(&kni_stop);
            if (f_stop)
                break;

            kni_ingress(tor_params_array[i], 0, i);
        }
    } else if (flag == LCORE_TX) {
        RTE_LOG(INFO, APP, "Lcore %u is writing to queue for ToR %u\n",
                tor_params_array[i]->lcore_tx, i);
        while (1) {
            f_stop = rte_atomic32_read(&kni_stop);
            if (f_stop)
                break;

            /*
		    * Inform the configuration core that we have exited the
		    * linked list and that no devices are in use if requested.
		    */
            if (rx_thread_params_array[i]->dev_removal_flag == REQUEST_DEV_REMOVAL)
                rx_thread_params_array[i]->dev_removal_flag = ACK_DEV_REMOVAL;

            egress_stage1(rx_thread_params_array[i]);
        }
    } else if (flag == LCORE_TX_2) {
        RTE_LOG(INFO, APP, "Lcore %u is writing to ports for ToR %u \n",
                tor_params_array[i]->lcore_tx2, i);
        int16_t lock_budgets, old_lock_budgets;
        uint32_t nonlocal_budgets[MAX_NUM_ROTORS * MAX_NUM_NONLOCAL_QUEUES],
                local_budgets[MAX_NUM_ROTORS * MAX_NUM_LOCAL_QUEUES];
        // Initial read of old lock value
        old_lock_budgets = rte_atomic16_read(&tor_params_array[i]->lock_budget);
        // Start shaping period...
        shaper_cycle_start = rte_rdtsc();
        while (1) {
            f_stop = rte_atomic32_read(&kni_stop);
            act_matching = rte_atomic16_read(&active_matching);

            if (f_stop)
                break;

            lock_budgets = rte_atomic16_read(&tor_params_array[i]->lock_budget);
            // If old is different then current, there was an update in the budgets.
            if (lock_budgets != old_lock_budgets) {
                /* Read ROTORNET budgets into a thread local copy. So that control plane can update them independently */
                rte_memcpy(nonlocal_budgets, tor_params_array[i]->nonlocal_budgets, sizeof(nonlocal_budgets));
                rte_memcpy(local_budgets, tor_params_array[i]->local_budgets, sizeof(local_budgets));
                old_lock_budgets = lock_budgets;
            }

            loop_cycle_start = rte_rdtsc();
            egress_stage2(tor_params_array[i], act_matching, i, local_budgets, nonlocal_budgets);

            thread_stats_array[i]->cycles_total.temp_value += rte_rdtsc() - loop_cycle_start;
            thread_stats_array[i]->num_total.temp_value += 1;
            if (thread_stats_array[i]->num_total.temp_value > NUM_TEMP_CYCLE_SAMPLES) {
                circular_buf_put(
                        thread_stats_array[i]->cycles_total.value_queue,
                        thread_stats_array[i]->cycles_total.temp_value
                );
                circular_buf_put(
                        thread_stats_array[i]->num_total.value_queue,
                        thread_stats_array[i]->num_total.temp_value
                );
                thread_stats_array[i]->cycles_total.temp_value = 0;
                thread_stats_array[i]->num_total.temp_value = 0;
            }

            // Check if shaping period is over and we need to reset shaping budgets...
            if (rte_rdtsc() - shaper_cycle_start > timer_frequency * SHAPING_PERIOD) {
                RTE_LOG(INFO, APP, "ToR %u: resetting shaper %"
                PRIu64
                " %"
                PRIu64
                "\n", i,
                        rte_rdtsc() - shaper_cycle_start, timer_frequency * SHAPING_PERIOD);
                reset_shaper(tor_params_array[i],
                             &remaining_cycles[i * (MAX_NUM_ROTORS + MAX_NUM_CACHE_LINKS)]);
                shaper_cycle_start = rte_rdtsc();
            }
        }

    } else if (flag == LCORE_SYNC) {
        if (i > 0) {
            RTE_LOG(INFO, APP, "Lcore %u (ToR %u) has nothing to do as sync is done via ToR 0\n",
                    tor_params_array[i]->lcore_sync, i);
            return 0;
        }

        RTE_LOG(INFO, APP, "Lcore %u is syncing via port %u for ToR %u\n",
                tor_params_array[i]->lcore_sync, tor_params_array[i]->sync_port, i);
        RTE_LOG(INFO, APP, "Budget per slot: %u\n", budget_per_slot);

        while (1) {
            f_stop = rte_atomic32_read(&kni_stop);
            if (f_stop)
                break;
            sync_thread(tor_params_array[i]);
        }
    } else
        RTE_LOG(INFO, APP, "Lcore %u has nothing to do\n", lcore_id);

    return 0;
}

/*
 * Initialize queues for traffic from local sources/host/VMs...
 */
static void init_local_queues(void) {
    unsigned int i, tor_id;
    struct tor_params *params;

    for (tor_id = 0; tor_id < MAX_NUM_TORS; tor_id++) {
        /* Number of KNIs is fixed to 1 */
        if (!tor_params_array[tor_id]) {
            continue;
        }
        params = tor_params_array[tor_id];
        RTE_LOG(INFO, APP, "ToR %u: Initialize (1) ring buffer as queue\n", tor_id);
        /* Init ring buffer */
        char ring_name[MAX_NAME_LEN];
        uint32_t socket = rte_lcore_to_socket_id(params->lcore_rx);
        struct rte_ring *ring;

        for (i = 0; i < params->nb_dst_racks; i++) {
            snprintf(ring_name, MAX_NAME_LEN, "ring-%u-%u", tor_id, i);
            ring = rte_ring_lookup(ring_name);

            // RTE_LOG(INFO, APP, "Initialize (2) ring buffer as queue\n");
            if (ring == NULL)
                params->queues[i] = rte_ring_create(ring_name, RING_SIZE, socket, RING_F_SP_ENQ | RING_F_SC_DEQ);
            else
                params->queues[i] = ring;
            RTE_LOG(INFO, APP, "Initialize (2) ring buffer as queue for dst %u\n", i);
        }

        for (i = 0; i < params->num_caches; i++) {
            snprintf(ring_name, MAX_NAME_LEN, "cache-%u-%u", tor_id, i);
            ring = rte_ring_lookup(ring_name);

            // RTE_LOG(INFO, APP, "Initialize (2) ring buffer as queue\n");
            if (ring == NULL)
                params->queues[params->nb_dst_racks + i] = rte_ring_create(
                        ring_name,
                        RING_SIZE,
                        socket,
                        RING_F_SP_ENQ | RING_F_SC_DEQ
                );
            else
                params->queues[params->nb_dst_racks + i] = ring;
            RTE_LOG(INFO, APP, "Initialize (3) ring buffer as queue for cache %u\n", i);
        }
    }
}

/*
 * Initialize queues for non-local traffic that was received and has to be forwarded on the second hop
 */
static void init_nonlocal_queues(void) {
    unsigned int tor_idx, dst_tor_id, qidx;
    struct tor_params *params;

    for (tor_idx = 0; tor_idx < MAX_NUM_TORS; tor_idx++) {
        if (!tor_params_array[tor_idx]) {
            /* ToR does not exist */
            continue;
        }
        params = tor_params_array[tor_idx];
        RTE_LOG(INFO, APP, "ToR %u: Initialize (1) ring buffers as non-local queue\n", tor_idx);
        /* Init ring buffer */
        char ring_name[MAX_NAME_LEN];
        uint32_t socket = rte_lcore_to_socket_id(params->lcore_rx);
        struct rte_ring *ring;

        /* For every src and dst tor pair create a queue */
        for (dst_tor_id = 0; dst_tor_id < params->nb_dst_racks; dst_tor_id++) {
            snprintf(ring_name, MAX_NAME_LEN, "nlring-%u-%u", tor_idx, dst_tor_id);
            ring = rte_ring_lookup(ring_name);

            if (ring == NULL)
                params->nonlocal_queues[dst_tor_id] = rte_ring_create(ring_name, RING_SIZE,
                                                                      socket, RING_F_SP_ENQ | RING_F_SC_DEQ);
            else
                params->nonlocal_queues[dst_tor_id] = ring;
            RTE_LOG(INFO, APP, "Initialize (2) ring buffer as queue for dst %u at idx %u\n", dst_tor_id, qidx);
        }
    }
}

/*
 * Initialize the queue budgets as atomic variables with initial value = 0
 */
static void init_queue_budgets(void) {
    unsigned int tor_idx;
    struct tor_params *current_tor;

    for (tor_idx = 0; tor_idx < MAX_NUM_TORS; tor_idx++) {
        if (!tor_params_array[tor_idx]) {
            /* ToR does not exist */
            continue;
        }
        RTE_LOG(INFO, APP, "ToR %u: Initialize queue budgets\n", tor_idx);
        current_tor = tor_params_array[tor_idx];

        memset(current_tor->local_budgets, 0, sizeof(current_tor->local_budgets));
        memset(current_tor->nonlocal_budgets, 0, sizeof(current_tor->nonlocal_budgets));
        rte_atomic16_init(&current_tor->lock_budget);

    }
}

/*
 * Initialize data structures for thread statistics. It is mostly based on ring-buffers.
 */
static void init_thread_statistics_queues(void) {
    unsigned int i, loc_id, tor_id;
    uint64_t *buffer;

    for (tor_id = 0; tor_id < MAX_NUM_TORS; tor_id++) {
        if (tor_params_array[tor_id]) {
            RTE_LOG(INFO, APP, "ToR %u: Initialize (1) ring for thread statistics as queue\n", tor_id);
            /* Init ring buffer */

            for (i = 0; i < tor_params_array[tor_id]->num_rotors; i++) {
                buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                thread_stats_array[tor_id]->cycles_rotor[i].value_queue = circular_buf_init(
                        buffer, RING_SIZE_STATS
                );
                buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                thread_stats_array[tor_id]->num_rotor[i].value_queue = circular_buf_init(
                        buffer, RING_SIZE_STATS
                );
                for (loc_id = 0; loc_id < NUM_JUMP_LOCATIONS; loc_id++) {
                    buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                    thread_stats_array[tor_id]->locations_rotor[i * NUM_JUMP_LOCATIONS +
                                                                loc_id].value_queue = circular_buf_init(
                            buffer, RING_SIZE_STATS
                    );
                    buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                    thread_stats_array[tor_id]->cycle_locations_rotor[i * NUM_JUMP_LOCATIONS +
                                                                      loc_id].value_queue = circular_buf_init(
                            buffer, RING_SIZE_STATS
                    );
                }
            }
            for (i = 0; i < tor_params_array[tor_id]->num_caches; i++) {
                buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                thread_stats_array[tor_id]->cycles_cache[i].value_queue = circular_buf_init(
                        buffer, RING_SIZE_STATS
                );
                buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                thread_stats_array[tor_id]->num_cache[i].value_queue = circular_buf_init(
                        buffer, RING_SIZE_STATS
                );
                for (loc_id = 0; loc_id < NUM_JUMP_LOCATIONS; loc_id++) {
                    buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                    thread_stats_array[tor_id]->locations_cache[i * NUM_JUMP_LOCATIONS +
                                                                loc_id].value_queue = circular_buf_init(
                            buffer, RING_SIZE_STATS
                    );
                    buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
                    thread_stats_array[tor_id]->cycle_locations_cache[i * NUM_JUMP_LOCATIONS +
                                                                      loc_id].value_queue = circular_buf_init(
                            buffer, RING_SIZE_STATS
                    );
                }
            }
            buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
            thread_stats_array[tor_id]->cycles_total.value_queue = circular_buf_init(
                    buffer, RING_SIZE_STATS
            );
            buffer = calloc(RING_SIZE_STATS, sizeof(uint64_t));
            thread_stats_array[tor_id]->num_total.value_queue = circular_buf_init(
                    buffer, RING_SIZE_STATS
            );
        }
    }
}

/* Initialise a single port on an Ethernet device */
static void
init_port(uint16_t port) {
    int ret;
    uint16_t nb_rxd = NB_RXD;
    uint16_t nb_txd = NB_TXD;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_rxconf rxq_conf;
    struct rte_eth_txconf txq_conf;
    struct rte_eth_conf local_port_conf = port_conf;

    /* Initialise device and RX/TX queues */
    RTE_LOG(INFO, APP, "Initialising port %u ...\n", (unsigned) port);
    fflush(stdout);
    rte_eth_dev_info_get(port, &dev_info);
    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
        local_port_conf.txmode.offloads |=
                DEV_TX_OFFLOAD_MBUF_FAST_FREE;
    ret = rte_eth_dev_configure(port, 1, 1, &local_port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not configure port%u (%d)\n",
                 (unsigned) port, ret);

    ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not adjust number of descriptors "
                               "for port%u (%d)\n", (unsigned) port, ret);

    RTE_LOG(INFO, APP, "TX offloads capa: %"
    PRIu64
    "\n", dev_info.tx_offload_capa);
    RTE_LOG(INFO, APP, "TX offloads: %"
    PRIu64
    "\n", local_port_conf.txmode.offloads);
    RTE_LOG(INFO, APP, "RX offloads capa: %"
    PRIu64
    "\n", dev_info.rx_offload_capa);
    RTE_LOG(INFO, APP, "RX offloads: %"
    PRIu64
    "\n", local_port_conf.rxmode.offloads);

    rxq_conf = dev_info.default_rxconf;
    rxq_conf.offloads = local_port_conf.rxmode.offloads;
    ret = rte_eth_rx_queue_setup(port, 0, nb_rxd,
                                 rte_eth_dev_socket_id(port), &rxq_conf, pktmbuf_pool);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not setup up RX queue for "
                               "port%u (%d)\n", (unsigned) port, ret);

    txq_conf = dev_info.default_txconf;
    txq_conf.offloads = local_port_conf.txmode.offloads;
    ret = rte_eth_tx_queue_setup(port, 0, nb_txd,
                                 rte_eth_dev_socket_id(port), &txq_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not setup up TX queue for "
                               "port%u (%d)\n", (unsigned) port, ret);

    ret = rte_eth_dev_start(port);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not start port%u (%d)\n",
                 (unsigned) port, ret);

    if (PROMISCUOUS_MODE)
        rte_eth_promiscuous_enable(port);
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void check_all_ports_link_status(void) {
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
    uint16_t portid;
    uint8_t count, all_ports_up, print_flag = 0;
    struct rte_eth_link link;

    printf("\nChecking link status\n");
    fflush(stdout);
    for (count = 0; count <= MAX_CHECK_TIME; count++) {
        all_ports_up = 1;
        RTE_ETH_FOREACH_DEV(portid)
        {
            memset(&link, 0, sizeof(link));
            rte_eth_link_get_nowait(portid, &link);
            /* print link status if flag set */
            if (print_flag == 1) {
                if (link.link_status)
                    printf(
                            "Port%d Link Up - speed %uMbps - %s\n",
                            portid, link.link_speed,
                            (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
                            ("full-duplex") : ("half-duplex\n"));
                else
                    printf("Port %d Link Down\n", portid);
                continue;
            }
            /* clear all_ports_up flag if any link down */
            if (link.link_status == ETH_LINK_DOWN) {
                all_ports_up = 0;
                break;
            }
        }
        /* after finally printing all link status, get out */
        if (print_flag == 1)
            break;

        if (all_ports_up == 0) {
            printf(".");
            fflush(stdout);
            rte_delay_ms(CHECK_INTERVAL);
        }

        /* set the print_flag if all ports up or timeout */
        if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
            print_flag = 1;
            printf("done\n");
        }
    }
}

/*
 * Remove a device from the specific data core linked list and from the
 * main linked list. Synchonization  occurs through the use of the
 * lcore dev_removal_flag. Device is made volatile here to avoid re-ordering
 * of dev->remove=1 which can cause an infinite loop in the rte_pause loop.
 */
static void
destroy_device(int vid) {
    struct vhost_dev *vdev = NULL;
    unsigned tor_id;

    TAILQ_FOREACH(vdev, &vhost_dev_list, global_vdev_entry)
    {
        if (vdev->vid == vid)
            break;
    }
    if (!vdev)
        return;
    /*set the remove flag. */
    vdev->remove = 1;
    while (vdev->ready != DEVICE_SAFE_REMOVE) {
        rte_pause();
    }

    if (builtin_net_driver)
        vs_vhost_net_remove(vdev);

    TAILQ_REMOVE(&rx_thread_params_array[vdev->rx_thread_id]->vdev_list, vdev,
                 lcore_vdev_entry);
    TAILQ_REMOVE(&vhost_dev_list, vdev, global_vdev_entry);


    /* Set the dev_removal_flag on each lcore. */
    for (tor_id = 0; tor_id < num_tors; tor_id++) {
        rx_thread_params_array[tor_id]->dev_removal_flag = REQUEST_DEV_REMOVAL;
    }

    /*
     * Once each core has set the dev_removal_flag to ACK_DEV_REMOVAL
     * we can be sure that they can no longer access the device removed
     * from the linked lists and that the devices are no longer in use.
     */
    for (tor_id = 0; tor_id < num_tors; tor_id++) {
        while (rx_thread_params_array[tor_id]->dev_removal_flag != ACK_DEV_REMOVAL)
            rte_pause();
    }

    rx_thread_params_array[vdev->rx_thread_id]->device_num--;

    RTE_LOG(INFO, VHOST_DATA,
            "(%d) device has been removed from data core\n",
            vdev->vid);

    rte_free(vdev);
}

/*
 * A new device is added to a data core. First the device is added to the main linked list
 * and then allocated to a specific data core.
 */
static int
new_device(int vid) {
    int core_add = 0;
    unsigned lcore;
    uint32_t device_num_min = num_devices;
    struct vhost_dev *vdev;

    vdev = rte_zmalloc("vhost device", sizeof(*vdev), RTE_CACHE_LINE_SIZE);
    if (vdev == NULL) {
        RTE_LOG(INFO, VHOST_DATA,
                "(%d) couldn't allocate memory for vhost dev\n",
                vid);
        return -1;
    }
    vdev->vid = vid;

    if (builtin_net_driver)
        vs_vhost_net_setup(vdev);

    TAILQ_INSERT_TAIL(&vhost_dev_list, vdev, global_vdev_entry);
    vdev->vmdq_rx_q = vid * queues_per_pool + vmdq_queue_base;

    /*reset ready flag*/
    vdev->ready = DEVICE_MAC_LEARNING;
    vdev->remove = 0;

    /* Find a suitable thread to add the device. */
    for (lcore = 0; lcore < num_tors; lcore++) {
        if (rx_thread_params_array[lcore]->device_num < device_num_min) {
            device_num_min = rx_thread_params_array[lcore]->device_num;
            core_add = lcore;
        }
    }

    RTE_LOG(INFO, APP, "Device num: %u, core add: %i\n", device_num_min, core_add);
    vdev->rx_thread_id = core_add;

    TAILQ_INSERT_TAIL(&rx_thread_params_array[vdev->rx_thread_id]->vdev_list, vdev,
                      lcore_vdev_entry);
    rx_thread_params_array[vdev->rx_thread_id]->device_num++;
    RTE_LOG(INFO, APP, "%u devices registered with ToR %i\n", rx_thread_params_array[vdev->rx_thread_id]->device_num,
            vdev->rx_thread_id);

    /* Disable notifications. */
    rte_vhost_enable_guest_notification(vid, VIRTIO_RXQ, 0);
    rte_vhost_enable_guest_notification(vid, VIRTIO_TXQ, 0);

    RTE_LOG(INFO, VHOST_DATA,
            "(%d) device has been added to data core %d\n",
            vid, vdev->rx_thread_id);

    TAILQ_FOREACH(vdev, &rx_thread_params_array[core_add]->vdev_list, global_vdev_entry)
    {
        RTE_LOG(INFO, VHOST_DATA, "ToR: %i, %d\n", core_add, vdev->vid);
    }
    return 0;
}

/*
 * These callback allow devices to be added to the data core when configuration
 * has been fully complete.
 */
static const struct vhost_device_ops virtio_net_device_ops =
        {
                .new_device =  new_device,
                .destroy_device = destroy_device,
        };

/*
 * While creating an mbuf pool, one key thing is to figure out how
 * many mbuf entries is enough for our use. FYI, here are some
 * guidelines:
 *
 * - Each rx queue would reserve @nr_rx_desc mbufs at queue setup stage
 *
 * - For each switch core (A CPU core does the packet switch), we need
 *   also make some reservation for receiving the packets from virtio
 *   Tx queue. How many is enough depends on the usage. It's normally
 *   a simple calculation like following:
 *
 *       MAX_PKT_BURST * max packet size / mbuf size
 *
 *   So, we definitely need allocate more mbufs when TSO is enabled.
 *
 * - Similarly, for each switching core, we should serve @nr_rx_desc
 *   mbufs for receiving the packets from physical NIC device.
 *
 * - We also need make sure, for each switch core, we have allocated
 *   enough mbufs to fill up the mbuf cache.
 */
static void
create_mbuf_pool(uint16_t nr_port, uint32_t nr_switch_core, uint32_t mbuf_size,
                 uint32_t nr_queues, uint32_t nr_rx_desc, uint32_t nr_mbuf_cache) {
    uint32_t nr_mbufs;
    uint32_t nr_mbufs_per_core;
    uint32_t mtu = 1500;

    if (mergeable)
        mtu = 9000;
    if (enable_tso)
        mtu = 64 * 1024;

    nr_mbufs_per_core = (mtu + mbuf_size) * PKT_BURST_SZ /
                        (mbuf_size - RTE_PKTMBUF_HEADROOM);
    nr_mbufs_per_core += nr_rx_desc;
    nr_mbufs_per_core = RTE_MAX(nr_mbufs_per_core, nr_mbuf_cache);

    nr_mbufs = nr_queues * nr_rx_desc;
    nr_mbufs += nr_mbufs_per_core * nr_switch_core;
    nr_mbufs *= nr_port;

    RTE_LOG(INFO, APP, "Creating MBUF pool for %u MBUFs of max size %u.\n", nr_mbufs, mbuf_size);

    pktmbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", nr_mbufs,
                                           nr_mbuf_cache, 0, mbuf_size,
                                           rte_socket_id());
    if (pktmbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
}

static void *
print_stats_thread(__rte_unused void *arg) {
    unsigned idx_queues;
    int idx_tors;
    // const char clr[] = {27, '[', '2', 'J', '\0'};
    // const char top_left[] = {27, '[', '1', ';', '1', 'H', '\0'};

    while (1) {
        sleep(1);

        /* Clear screen and move to top left */
        print_stats();
        print_thread_stats();
        print_rotor_budgets();

        for (idx_tors = 0; idx_tors < num_tors; idx_tors++) {
            for (idx_queues = 0; idx_queues < tor_params_array[idx_tors]->nb_dst_racks +
                                              tor_params_array[idx_tors]->num_caches; idx_queues++) {
                printf("ToR %u Queue %u : %u\n", idx_tors, idx_queues,
                       rte_ring_count(tor_params_array[idx_tors]->queues[idx_queues]));
            }

        }
        printf("===================================================\n");
    }
    return NULL;
}


/* Initialise ports/queues etc. and start main loop on each core */
int
main(int argc, char **argv) {
    int ret, lcore_id;
    int rte_argc;
    char **rte_argv;
    uint16_t nb_sys_ports, port;
    unsigned i, tor_id;
    void *retval;
    static pthread_t tid;
    int pid;
    uint64_t flags = 0;

    /* Associate signal_hanlder function with USR signals */
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGRTMIN, signal_handler);
    signal(SIGINT, signal_handler);

    /* Initialise EAL ----------- DPDK stuff starts here */
    /* Initialize eal options with zeros */
    rte_argc = 0;
    rte_argv = calloc(8, sizeof(char *));  // program name, -l, cpu ids, -n, 4, --
    RTE_LOG(INFO, APP, "argc before eal parsing: %i, %i\n", rte_argc, argc);
    parse_args_eal(argc, argv, &rte_argc, rte_argv);

    ret = rte_eal_init(rte_argc, rte_argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    rte_argc -= ret;
    rte_argv += ret;
    RTE_LOG(INFO, APP, "RTE argc after init of EAL: %i, %i\n", rte_argc, argc);

    /* Reset tor_params_array */
    memset(&tor_params_array, 0, sizeof(tor_params_array));
    memset(&kni_stats, 0, sizeof(kni_stats));
    memset(&thread_stats_array, 0, sizeof(thread_stats_array));
    RTE_LOG(INFO, APP, "Initialized kni port params array ptr %p\n", (void *) tor_params_array);

    /* Parse application arguments (after the EAL ones) */
    optind = 1;
    ret = parse_args(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not parse input parameters\n");

    for (tor_id = 0; tor_id < MAX_NUM_TORS; tor_id++) {
        if (tor_params_array[tor_id] && rx_thread_params_array[tor_id]) {
            TAILQ_INIT(&rx_thread_params_array[tor_id]->vdev_list);
            kni_stats[tor_id] = calloc(1, sizeof(struct kni_interface_stats));
            thread_stats_array[tor_id] = calloc(1, sizeof(struct thread_stats));
        }
    }

    /* Create the mbuf pool */
    pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
                                           MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id());
    if (pktmbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
        return -1;
    }

    /* Get number of ports found in scan */
    nb_sys_ports = rte_eth_dev_count_avail();
    if (nb_sys_ports == 0)
        rte_exit(EXIT_FAILURE, "No supported Ethernet device found\n");

    /* Check if the configured port ID is valid */
    for (tor_id = 0; tor_id < MAX_NUM_TORS; tor_id++) {
        if (!tor_params_array[tor_id])
            continue;
        for (i = 0; i < tor_params_array[tor_id]->num_rotors; i++)
            if (tor_params_array[tor_id] &&
                !rte_eth_dev_is_valid_port(tor_params_array[tor_id]->rotors[i]->port_id))
                rte_exit(EXIT_FAILURE, "Configured invalid "
                                       "port ID %u\n", i);
    }
    RTE_LOG(INFO, APP, "Start initializing local queues\n");
    init_local_queues();

    RTE_LOG(INFO, APP, "Start initializing non-local queues\n");
    init_nonlocal_queues();

    RTE_LOG(INFO, APP, "Initializing queue budgets\n");
    init_queue_budgets();

    init_thread_statistics_queues();

    RTE_LOG(INFO, APP, "Prepare shaping\n");
    prepare_shaping(tor_params_array);

    RTE_LOG(INFO, APP, "Start initializing ports\n");
    if (mergeable) {
        port_conf.rxmode.offloads |=
                DEV_RX_OFFLOAD_JUMBO_FRAME;
        port_conf.rxmode.max_rx_pkt_len
                = JUMBO_FRAME_MAX_SIZE;
    }

    /* Initialise each port */
    RTE_ETH_FOREACH_DEV(port)
    {
        init_port(port);

        if (port >= RTE_MAX_ETHPORTS)
            rte_exit(EXIT_FAILURE, "Can not use more than "
                                   "%d ports for kni\n", RTE_MAX_ETHPORTS);
    }
    check_all_ports_link_status();

    ret = rte_ctrl_thread_create(&tid, "print-stats", NULL,
                                 print_stats_thread, NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE,
                 "Cannot create print-stats thread\n");

    /* Initialize the matchings */
    RTE_LOG(INFO, APP, "Initial matching: %i\n", active_matching);

    pid = getpid();
    RTE_LOG(INFO, APP, "========================\n");
    RTE_LOG(INFO, APP, "KNI Running\n");
    RTE_LOG(INFO, APP, "kill -SIGUSR1 %d\n", pid);
    RTE_LOG(INFO, APP, "    Show KNI Statistics.\n");
    RTE_LOG(INFO, APP, "kill -SIGUSR2 %d\n", pid);
    RTE_LOG(INFO, APP, "    Zero KNI Statistics.\n");
    RTE_LOG(INFO, APP, "========================\n");
    fflush(stdout);

    if (ret < 0)
        rte_exit(EXIT_FAILURE,
                 "Could not create link status thread!\n");

    /* Launch per-lcore function on every lcore */
    RTE_LCORE_FOREACH_SLAVE(lcore_id)
    {
        RTE_LOG(INFO, APP, "%i\n", lcore_id);
        rte_eal_remote_launch(main_loop, NULL, lcore_id);
    }

    RTE_LOG(INFO, APP, "Started workers...\n");
    if (client_mode)
        flags |= RTE_VHOST_USER_CLIENT;

    if (dequeue_zero_copy)
        flags |= RTE_VHOST_USER_DEQUEUE_ZERO_COPY;

    /* Register vhost user driver to handle vhost messages. */
    for (i = 0; i < (unsigned) nb_sockets; i++) {
        char *file = socket_files + i * PATH_MAX;
        RTE_LOG(INFO, APP, "Registering driver at %s\n", file);
        ret = rte_vhost_driver_register(file, flags);
        if (ret != 0) {
            unregister_drivers(i);
            rte_exit(EXIT_FAILURE,
                     "vhost driver register failure.\n");
        }

        if (builtin_net_driver)
            rte_vhost_driver_set_features(file, VIRTIO_NET_FEATURES);

        if (mergeable == 0) {
            rte_vhost_driver_disable_features(file,
                                              1ULL << VIRTIO_NET_F_MRG_RXBUF);
        }

        if (enable_tx_csum == 0) {
            rte_vhost_driver_disable_features(file,
                                              1ULL << VIRTIO_NET_F_CSUM);
        }

        if (enable_tso == 0) {
            rte_vhost_driver_disable_features(file,
                                              1ULL << VIRTIO_NET_F_HOST_TSO4);
            rte_vhost_driver_disable_features(file,
                                              1ULL << VIRTIO_NET_F_HOST_TSO6);
            rte_vhost_driver_disable_features(file,
                                              1ULL << VIRTIO_NET_F_GUEST_TSO4);
            rte_vhost_driver_disable_features(file,
                                              1ULL << VIRTIO_NET_F_GUEST_TSO6);
        }

        if (PROMISCUOUS_MODE) {
            rte_vhost_driver_enable_features(file,
                                             1ULL << VIRTIO_NET_F_CTRL_RX);
        }

        ret = rte_vhost_driver_callback_register(file,
                                                 &virtio_net_device_ops);
        if (ret != 0) {
            rte_exit(EXIT_FAILURE,
                     "failed to register vhost driver callbacks.\n");
        }

        if (rte_vhost_driver_start(file) < 0) {
            rte_exit(EXIT_FAILURE,
                     "failed to start vhost driver.\n");
        }
    }

    /* Wait for other threads to finish */
    RTE_LCORE_FOREACH_SLAVE(i)
    {
        if (rte_eal_wait_lcore(i) < 0)
            return -1;
    }

    for (tor_id = 0; tor_id < MAX_NUM_TORS; tor_id++) {
        if (tor_params_array[tor_id]) {
            rte_free(tor_params_array[tor_id]);
            tor_params_array[tor_id] = NULL;
        }
    }

    return 0;
}
