//
// Created by Johannes Zerwas <johannes.zerwas@tum.de>.
//

#include "shaping.h"

void prepare_shaping(struct tor_params **tor_array) {
    unsigned tor_id;
    timer_frequency = rte_get_timer_hz();
    printf("Timer frequency: %"PRIu64"\n", timer_frequency);
    for (tor_id = 0; tor_id < num_tors; tor_id++) {
        if (!tor_array[tor_id]->shaping) {
            tor_array[tor_id]->cycles_per_period = 0;
	    printf("Shaping disabled for ToR %u\n", tor_id);
            continue;
	}
        if (tor_array[tor_id]->shaping == 1) {
            printf("Shaping ToR %u: Using native mode.\n", tor_id);
            tor_array[tor_id]->cycles_per_period = SHAPING_MARGIN * timer_frequency / (tor_array[tor_id]->num_rotors + tor_array[tor_id]->num_caches);
        } else {
            tor_array[tor_id]->cycles_per_period = SHAPING_MARGIN * timer_frequency / tor_array[tor_id]->shaping;
            printf("Shaping ToR %u: Using factor %i\n", tor_id, tor_array[tor_id]->shaping);
        }
        printf("ToR %u: %"PRIu64" Cycles per link\n", tor_id, tor_array[tor_id]->cycles_per_period);
        reset_shaper(tor_array[tor_id], &remaining_cycles[tor_id*MAX_NUM_LINKS]);
    }
}

void reset_shaper(struct tor_params *p, uint64_t *remaining_cycles_of_tor) {
    unsigned link_id;
    for (link_id=0; link_id < p->num_rotors; link_id++) {
        remaining_cycles_of_tor[link_id] = p->cycles_per_period;
    }
    for (link_id=0; link_id < p->num_caches; link_id++) {
        remaining_cycles_of_tor[MAX_NUM_ROTORS+link_id] = p->cycles_per_period;
    }
}

void decrease_shaper(uint64_t loop_cycle_start, uint64_t *remaining_cycles_of_link) {
    uint64_t used_cycles;
    used_cycles = rte_rdtsc() - loop_cycle_start;
    if (used_cycles >= (*remaining_cycles_of_link)) {
        (*remaining_cycles_of_link) = 0;
    } else {
        (*remaining_cycles_of_link) -= used_cycles;
    }
}
