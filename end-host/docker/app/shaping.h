//
// Created by Johannes Zerwas <johannes.zerwas@tum.de>.
//

#ifndef ROTOR_EMULATION_SHAPING_H
#define ROTOR_EMULATION_SHAPING_H

#include "config.h"

#define SHAPING_PERIOD 1   // in seconds
#define SHAPING_MARGIN 0.9
uint64_t timer_frequency;
uint64_t remaining_cycles[MAX_NUM_TORS * (MAX_NUM_ROTORS + MAX_NUM_CACHE_LINKS)];

/*
 * Read the shaper configuration for the ToRs and initialize the data structures of the shaper
 */
void prepare_shaping(struct tor_params **tor_array);

/*
 * Reset the remaining cycle counts for a new period
 */
void reset_shaper(struct tor_params *p, uint64_t *remaining_cycles_of_tor);

/*
 * Reduce remaining cycles count for a link
 */
void decrease_shaper(uint64_t loop_cycle_start, uint64_t *remaining_cycles_of_link);

#endif //ROTOR_EMULATION_SHAPING_H
