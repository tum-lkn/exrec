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

#include "config.h"

/*
 * Set socket file path.
 */
int us_vhost_parse_socket_path(const char *q_arg) {
    char *old;

    /* parse number string */
    if (strnlen(q_arg, PATH_MAX) == PATH_MAX)
        return -1;

    old = socket_files;
    socket_files = realloc(socket_files, PATH_MAX * (nb_sockets + 1));
    if (socket_files == NULL) {
        free(old);
        return -1;
    }

    strncpy(socket_files + nb_sockets * PATH_MAX, q_arg, PATH_MAX);
    nb_sockets++;

    return 0;
}


int extract_matchings_of_rotor(json_value *matching_array, int tor_id, int rotor_id, int *num_racks) {
    unsigned int x;
    RTE_LOG(DEBUG, APP, "Start extracting matchings\n");
    for (x = 0; x < matching_array->u.array.length; x++) {
        RTE_LOG(INFO, APP, "VID %i maps to QID %i\n", x + 2, (int) matching_array->u.array.values[x]->u.integer);
        tor_params_array[tor_id]->rotors[rotor_id]->vid_to_qid_mapping[x +
                                                                            VLAN_ID_OFFSET] = matching_array->u.array.values[x]->u.integer;
        (*num_racks) += 1;
    }
    return 0;

}


int extract_rotors(json_value *array, int tor_id) {
    json_char *key;
    unsigned int x, y, num_matchings_per_rotor;
    int ret;
    json_value *array_elem, *value;
    RTE_LOG(DEBUG, APP, "Start extracting rotors\n");
    memset(&tor_params_array[tor_id]->rotors, 0, sizeof(tor_params_array[tor_id]->rotors));

    for (x = 0; x < array->u.array.length; x++) {
        array_elem = array->u.array.values[x];
        tor_params_array[tor_id]->rotors[x] = malloc(sizeof(struct rotor_switch));

        for (y = 0; y < array_elem->u.object.length; y++) {
            key = array_elem->u.object.values[y].name;
            value = array_elem->u.object.values[y].value;
            if (!strncmp(key, OPTION_PORT, sizeof(OPTION_PORT))) {
                ret = rte_eth_dev_get_port_by_name(
                        value->u.string.ptr,
                        &tor_params_array[tor_id]->rotors[x]->port_id
                );
                RTE_LOG(INFO, APP, "Port of Rotor %i is %u\n", x, tor_params_array[tor_id]->rotors[x]->port_id);
            } else if (!strncmp(key,
                                OPTION_MATCHINGS,
                                sizeof(OPTION_MATCHINGS))) {
                num_matchings_per_rotor = 1;
                ret = extract_matchings_of_rotor(
                        value,
                        tor_id,
                        x,
                        &num_matchings_per_rotor
                );
                if (num_matchings_per_rotor > tor_params_array[tor_id]->nb_dst_racks) {
                    tor_params_array[tor_id]->nb_dst_racks = num_matchings_per_rotor;
                }
            }
        }
        tor_params_array[tor_id]->num_rotors++;

    }
    return 0;
}

int extract_caches(json_value *jvalue, int tor_id) {
    unsigned int x, y;
    json_char *key;
    json_value *value;

    memset(&tor_params_array[tor_id]->cache, 0, sizeof(tor_params_array[tor_id]->cache));

    for (x = 0; x < jvalue->u.object.length; x++) {
        key = jvalue->u.object.values[x].name;
        value = jvalue->u.object.values[x].value;
        tor_params_array[tor_id]->cache[x] = malloc(sizeof(struct cache_switch));
        memset(&tor_params_array[tor_id]->cache[x]->mappings, 0, sizeof(tor_params_array[tor_id]->cache[x]->mappings));

        rte_eth_dev_get_port_by_name(
                key,
                &tor_params_array[tor_id]->cache[x]->port_id
        );
        tor_params_array[tor_id]->num_caches++;

        for (y=0; y < tor_params_array[tor_id]->nb_dst_racks; y++) {
            tor_params_array[tor_id]->cache[x]->mappings[y] = malloc(sizeof(struct flow_mapping));
        }
    }

    return 0;
}


int extract_cores(json_value *array, int tor_id) {
    unsigned int x;
    json_value *array_elem;
    RTE_LOG(DEBUG, APP, "Start extracting cores\n");
    if (array->u.array.length < 4) {
        RTE_LOG(WARNING, APP, "not enough cores");
        return -1;
    }

    for (x = 0; x < array->u.array.length; x++) {
        array_elem = array->u.array.values[x];
        RTE_LOG(DEBUG, APP, "Using core id %i\n", (int) array_elem->u.integer);
    }
    tor_params_array[tor_id]->lcore_sync = array->u.array.values[1]->u.integer;
    tor_params_array[tor_id]->lcore_rx = array->u.array.values[2]->u.integer;
    tor_params_array[tor_id]->lcore_tx = array->u.array.values[3]->u.integer;
    tor_params_array[tor_id]->lcore_tx2 = array->u.array.values[4]->u.integer;

    return 0;
}

int process_object(json_value *jvalue, int tor_id) {
    json_char *key;
    json_value *value;
    int length, x, ret;
    if (jvalue == NULL) {
        return -1;
    }
    length = jvalue->u.object.length;
    for (x = 0; x < length; x++) {
        key = jvalue->u.object.values[x].name;
        value = jvalue->u.object.values[x].value;
        RTE_LOG(DEBUG, APP, "object[%d].name = %s\n", x, key);

        if (!strncmp(key,
                     OPTION_SYNC_PORT, sizeof(OPTION_SYNC_PORT))) {
            ret = rte_eth_dev_get_port_by_name(
                    value->u.string.ptr,
                    &tor_params_array[tor_id]->sync_port
            );
            RTE_LOG(INFO, APP, "Sync port set to %u\n", tor_params_array[tor_id]->sync_port);
            if (ret) {
                RTE_LOG(WARNING, APP, "Could not get port id from PCI address\n");
                return -1;
            }
        } else if (!strncmp(key,
                            OPTION_ROTORS, sizeof(OPTION_ROTORS))) {
            ret = extract_rotors(jvalue->u.object.values[x].value, tor_id);
            if (ret) {
                RTE_LOG(WARNING, APP, "Could not extract rotors\n");
                return -1;
            }
        } else if (!strncmp(key,
                            OPTION_ID, sizeof(OPTION_ID))) {
            tor_params_array[tor_id]->id = value->u.integer;
        } else if (!strncmp(key,
                            OPTION_NUM_RACKS, sizeof(OPTION_NUM_RACKS))) {
            tor_params_array[tor_id]->nb_dst_racks = value->u.integer;
        } else if (!strncmp(key,
                            OPTION_CORES, sizeof(OPTION_CORES))) {
            ret = extract_cores(jvalue->u.object.values[x].value, tor_id);
            if (ret) {
                RTE_LOG(WARNING, APP, "Could not extract cores\n");
                return -1;
            }

        } else if (!strncmp(key,
                            OPTION_CACHE, sizeof(OPTION_CACHE))) {
            ret = extract_caches(jvalue->u.object.values[x].value, tor_id);
            if (ret) {
                RTE_LOG(WARNING, APP, "Could not extract caches\n");
                return -1;
            }

        } else if (!strncmp(key,
                            OPTION_IP_ADDR, sizeof(OPTION_IP_ADDR))) {
            RTE_LOG(INFO, APP, "IP addr is %s\n", value->u.string.ptr);
            tor_params_array[tor_id]->my_address = calloc(18, sizeof(char));
            strncpy(tor_params_array[tor_id]->my_address, value->u.string.ptr, 18);
            RTE_LOG(INFO, APP, "IP addr is %s\n", tor_params_array[tor_id]->my_address);

        } else if (!strncmp(key, OPTION_SOCKET_FILE, sizeof(OPTION_SOCKET_FILE))) {
            us_vhost_parse_socket_path(value->u.string.ptr);
            RTE_LOG(INFO, APP, "Socket file %s\n", value->u.string.ptr);
        } else if (!strncmp(key, OPTION_PRINT_STATS, sizeof(OPTION_PRINT_STATS))) {
            tor_params_array[tor_id]->print_stats = value->u.integer;
            RTE_LOG(INFO, APP, "Print statistics %i\n", value->u.integer);
        } else if (!strncmp(key, OPTION_PRINT_CYCLES, sizeof(OPTION_PRINT_CYCLES))) {
            tor_params_array[tor_id]->print_cycles = value->u.integer;
            RTE_LOG(INFO, APP, "Print cycles %i\n", value->u.integer);
        } else if (!strncmp(key, OPTION_SHAPING, sizeof(OPTION_SHAPING))) {
            tor_params_array[tor_id]->shaping = value->u.integer;
            RTE_LOG(INFO, APP, "Shaping %i\n", value->u.integer);
        } else if (!strncmp(key, OPTION_INDIRECT, sizeof(OPTION_INDIRECT))) {
            // Write to global variable. Last ToR will dominate but values should anyways be the same.
            indirect_routing = value->u.integer;
            RTE_LOG(INFO, APP, "Indirect %i\n", value->u.integer);
        }
    }
    return 0;
}

int process_array(json_value *jvalue) {
    int ret;
    unsigned int x;

    for (x = 0; x < jvalue->u.array.length; x++) {
        RTE_LOG(INFO, APP, "Processing config for ToR %i\n", x);
        //tor_params_array[x] = rte_zmalloc("KNI_port_params",
        //                                       sizeof(struct tor_params), RTE_CACHE_LINE_SIZE);
        tor_params_array[x] = calloc(1, sizeof(struct tor_params));
        tor_params_array[x]->nb_dst_racks = 1;
        rx_thread_params_array[x] = calloc(1, sizeof(struct rx_thread_params));
	    memset(tor_params_array[x]->local_budgets, 0, sizeof(tor_params_array[x]->local_budgets));
	    memset(tor_params_array[x]->nonlocal_budgets, 0, sizeof(tor_params_array[x]->nonlocal_budgets));
        ret = process_object(jvalue->u.array.values[x], x);
        if (ret) {
            RTE_LOG(WARNING, APP, "Processing of ToR config failed.\n");
            return -1;
        }
        num_tors++;

    }
    return 0;
}


int parse_config(const char *arg) {
    char *filename;
    FILE *fp;
    struct stat filestatus;
    int file_size;
    char *file_contents;
    json_char *json;
    json_value *value;

    filename = arg;

    if (stat(filename, &filestatus) != 0) {
        RTE_LOG(WARNING, APP, "File %s not found\n", filename);
        return 1;
    }
    file_size = filestatus.st_size;
    file_contents = (char *) malloc(filestatus.st_size);
    if (file_contents == NULL) {
        RTE_LOG(WARNING, APP, "Memory error: unable to allocate %d bytes\n", file_size);
        return 1;
    }

    fp = fopen(filename, "rt");
    if (fp == NULL) {
        RTE_LOG(WARNING, APP, "Unable to open %s\n", filename);
        fclose(fp);
        free(file_contents);
        return 1;
    }
    if (fread(file_contents, file_size, 1, fp) != 1) {
        RTE_LOG(WARNING, APP, "Unable t read content of %s\n", filename);
        fclose(fp);
        free(file_contents);
        return 1;
    }
    fclose(fp);

    json = (json_char *) file_contents;
    value = json_parse(json, file_size);

    if (value == NULL) {
        RTE_LOG(WARNING, APP, "Unable to parse data\n");
        free(file_contents);
        return 1;
    }

    process_array(value);

    json_value_free(value);
    free(file_contents);
    return 0;
}

/* Display usage instructions */
void
print_usage(const char *prgname) {
    RTE_LOG(INFO, APP, "\nUsage: %s [EAL options] -- "
                       "[--config path/to/config.json]\n",
            prgname);
}

void print_config(void) {
    uint32_t i, rotor_id, cache_id;
    int tor_id;
    struct tor_params *p;
    RTE_LOG(INFO, APP, "Config has %i ToRs\n.", num_tors);

    for (tor_id = 0; tor_id < num_tors; tor_id++) {
        p = tor_params_array[tor_id];
        RTE_LOG(INFO, APP, "------ Config for ToR %i ------\n", tor_id);
        RTE_LOG(INFO, APP,
                " IP address: %s \n"
                " Num. Rotor Switches: %i\n"
                " Num. Cache Links: %i\n"
                " Core IDs: RX->%u, TX->%u, TX2->%u, Sync->%u\n"
                " Num. Racks in setup %u\n",
                p->my_address, (int) p->num_rotors, (int) p->num_caches,
                p->lcore_rx, p->lcore_tx, p->lcore_tx2, p->lcore_sync,
                p->nb_dst_racks
        );
        for (rotor_id = 0; rotor_id < p->num_rotors; rotor_id++) {
            RTE_LOG(INFO, APP,
                    " \t Phys. Port ID %u\n"
                    " \tMatchings: \n", p->rotors[rotor_id]->port_id
            );
            for (i = 0; i < MAX_NUM_DESTINATION_RACKS + VLAN_ID_OFFSET; i++) {
                RTE_LOG(INFO, APP,
                        " \t\t VID %u -> QID %i \n", i, p->rotors[rotor_id]->vid_to_qid_mapping[i]
                );
            }
        }
        for (cache_id = 0; cache_id < p->num_caches; cache_id++) {
            RTE_LOG(INFO, APP,
                    " \t Phys. Port ID %u\n", p->cache[cache_id]->port_id
            );
        }

    }
    RTE_LOG(INFO, APP, "Sockets: \n");
    for (i = 0; i < nb_sockets; i++) {
        RTE_LOG(INFO, APP, "%s\n", socket_files + i * PATH_MAX);
    }
}

#define CMDLINE_OPT_CONFIG  "config"

/* Parse the arguments given in the command line of the application */
int parse_args(int argc, char **argv) {
    int opt, longindex, ret = 0;
    const char *prgname = argv[0];
    static struct option longopts[] = {
            {CMDLINE_OPT_CONFIG, required_argument, NULL, 0},
            {NULL,               0,                 NULL, 0}
    };
    RTE_LOG(WARNING, APP, "Start parsing command line parameters\n");
    /* Disable printing messages within getopt() */
    opterr = 1;

    /* Parse command line */
    while ((opt = getopt_long(argc, argv, "", longopts,
                              &longindex)) != EOF) {
        switch (opt) {
            case 0:
                RTE_LOG(WARNING, APP, "Parameter is config\n");
                if (!strncmp(longopts[longindex].name,
                             CMDLINE_OPT_CONFIG,
                             sizeof(CMDLINE_OPT_CONFIG))) {
                    /* Open and read json config file */
                    ret = parse_config(optarg);
                    if (ret) {
                        printf("Invalid config\n");
                        print_usage(prgname);
                        return -1;
                    }
                    print_config();
                }
                break;
            default:
                print_usage(prgname);
                rte_exit(EXIT_FAILURE, "Invalid option specified\n");
        }
    }

    return ret;
}

int parse_args_eal(int argc, char **argv, int *rte_argc, char **rte_argv) {
    int opt, longindex, ret = 0;
    const char *prgname = argv[0];
    static struct option longopts[] = {
            {CMDLINE_OPT_CONFIG, required_argument, NULL, 0},
            {NULL,               0,                 NULL, 0}
    };

    /* Disable printing messages within getopt() */
    opterr = 1;

    /* Parse command line */
    while ((opt = getopt_long(argc, argv, "", longopts,
                              &longindex)) != EOF) {
        switch (opt) {
            case 0:
                RTE_LOG(WARNING, APP, "Parameter is config\n");
                if (!strncmp(longopts[longindex].name,
                             CMDLINE_OPT_CONFIG,
                             sizeof(CMDLINE_OPT_CONFIG))) {
                    /* Open and read json config file */
                    ret = parse_config_rte(optarg, rte_argc, rte_argv);
                    if (ret) {
                        printf("Invalid config\n");
                        print_usage(prgname);
                        return -1;
                    }
                }
                break;
            default:
                print_usage(prgname);
                rte_exit(EXIT_FAILURE, "Invalid option specified\n");
        }
    }
    return 0;
}


int parse_config_rte(const char *arg, int *rte_argc, char **rte_argv) {
    char *filename;
    FILE *fp;
    struct stat filestatus;
    int file_size;
    char *file_contents;
    json_char *json;
    json_value *value;

    filename = arg;

    if (stat(filename, &filestatus) != 0) {
        // RTE_LOG(WARNING, APP, "File %s not found\n", filename);
        return 1;
    }
    file_size = filestatus.st_size;
    file_contents = (char *) malloc(filestatus.st_size);
    if (file_contents == NULL) {
        // RTE_LOG(WARNING, APP, "Memory error: unable to allocate %d bytes\n", file_size);
        return 1;
    }

    fp = fopen(filename, "rt");
    if (fp == NULL) {
        // RTE_LOG(WARNING, APP, "Unable to open %s\n", filename);
        fclose(fp);
        free(file_contents);
        return 1;
    }
    if (fread(file_contents, file_size, 1, fp) != 1) {
        // RTE_LOG(WARNING, APP, "Unable t read content of %s\n", filename);
        fclose(fp);
        free(file_contents);
        return 1;
    }
    fclose(fp);

    json = (json_char *) file_contents;
    value = json_parse(json, file_size);

    if (value == NULL) {
        RTE_LOG(WARNING, APP, "Unable to parse data\n");
        free(file_contents);
        return 1;
    }

    process_array_rte(value, rte_argc, rte_argv);
    json_value_free(value);
    free(file_contents);
    return 0;
}

int process_array_rte(json_value *jvalue, int *rte_argc, char **rte_argv) {
    unsigned int x;

    rte_argv[0] = "main";
    rte_argv[1] = "-l";
    rte_argv[2] = calloc(14 * jvalue->u.array.length, sizeof(char));
    rte_argv[3] = "-n";
    rte_argv[4] = "4";
    rte_argv[5] = "--socket-mem";
    rte_argv[6] = "2048";
    rte_argv[7] = "--";
    (*rte_argc) = 8;

    for (x = 0; x < jvalue->u.array.length; x++) {
        RTE_LOG(INFO, APP, "EAL Processing config for ToR %i\n", x);
        process_object_rte(jvalue->u.array.values[x], rte_argc, rte_argv);
    }

    // remove last comma
    rte_argv[2][strlen(rte_argv[2]) - 1] = 0;
    RTE_LOG(INFO, APP, "Core ids argv: %s\n", rte_argv[2]);
    return 0;
}


int process_object_rte(json_value *jvalue, int *rte_argc, char **rte_argv) {
    json_char *key;
    int length, x, ret;
    if (jvalue == NULL) {
        return -1;
    }
    length = jvalue->u.object.length;
    for (x = 0; x < length; x++) {
        key = jvalue->u.object.values[x].name;

        if (!strncmp(key, OPTION_CORES, sizeof(OPTION_CORES))) {
            ret = extract_cores_rte(jvalue->u.object.values[x].value, rte_argc, rte_argv);
            if (ret) {
                RTE_LOG(WARNING, APP, "Could not extract cores\n");
                return -1;
            }
        }
    }
    return 0;
}

int extract_cores_rte(json_value *array, int *rte_argc, char **rte_argv) {
    unsigned int x;
    char buf[4];
    json_value *array_elem;
    if (array->u.array.length < 4) {
        return -1;
    }

    for (x = 0; x < array->u.array.length; x++) {
        array_elem = array->u.array.values[x];
        RTE_LOG(INFO, APP, "Using core id %i\n", (int) array_elem->u.integer);
        snprintf(buf, 4, "%i,", (int) array_elem->u.integer);
        strncat(rte_argv[2], &buf, 3);
    }
    RTE_LOG(INFO, APP, "Core ids argv: %s\n", rte_argv[2]);

    return 0;
}
