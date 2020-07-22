#ifndef MYRPL_CONF_H_
#define MYRPL_CONF_H_

#include "contiki.h"
#include "net/nbr-table.h"

struct nbr_stats{
    /* uint16_t cpu_time; */
    uint16_t tx_time;
};

NBR_TABLE_DECLARE(transmit_rates_table);

#endif