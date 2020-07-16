/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/energest.h"
#include "net/nbr-table.h"
#include "net/routing/rpl-lite/rpl-neighbor.h"
#include "net/link-stats.h"
#include "random.h"

#include "myrpl_conf.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define UDP_RPL_CLIENT_PORT 4321
#define UDP_RPL_SERVER_PORT 1234

static struct ctimer timer_transmit;

static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_tx_rpl_conn;
static struct simple_udp_connection udp_rx_rpl_conn;

NBR_TABLE_GLOBAL(struct nbr_stats, transmit_rates_table);

/* static unsigned long to_seconds(uint64_t time) { */
/*     return (unsigned long)(time / ENERGEST_SECOND); */
/* } */

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
void transmit_rate_callback(void *ptr){
    ctimer_reset(&timer_transmit);
    static char str[] = "transmit";
    struct nbr_stats* stats;
    /* uint16_t tmp = 0; */

    rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);
    while (nbr != NULL){
        stats = nbr_table_get_from_lladdr(transmit_rates_table, rpl_neighbor_get_lladdr(nbr));
        if(stats == NULL){
            stats = nbr_table_add_lladdr(transmit_rates_table, rpl_neighbor_get_lladdr(nbr), NBR_TABLE_REASON_UNDEFINED, NULL);
        }
        /* transmit_rate = &tmp; */
        uip_ipaddr_t *receiver_ipaddr =  rpl_neighbor_get_ipaddr(nbr);
        /* LOG_INFO("Transmitrate: %ds from ", stats->tx_time); */
        /* LOG_INFO_6ADDR(receiver_ipaddr); */
        /* LOG_INFO_("\n"); */
        
        /* const struct link_stats *link; */
        /* link = link_stats_from_lladdr(rpl_neighbor_get_lladdr(nbr)); */
        /* LOG_INFO("etx: %d, rssi: %d from ", link->etx, link->rssi); */
        /* LOG_INFO_6ADDR(receiver_ipaddr); */
        /* LOG_INFO_("\n"); */

        simple_udp_sendto(&udp_tx_rpl_conn, str, strlen(str), receiver_ipaddr);
        nbr = nbr_table_next(rpl_neighbors, nbr);

    }
    
}
static void udp_rpl_tx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {

    /* LOG_INFO("Received trasmitrate '%.*s' from ", datalen, (char *)data); */
    /* LOG_INFO_6ADDR(sender_addr); */
    /* LOG_INFO_("\n"); */
    unsigned int data_num = 0;
    unsigned int base = 1;
    int i;
    for(i = 0; i < datalen; i++){
        data_num *= base;
        data_num += (*data - 48);
        base *= 10;
        data++;
    }

    struct nbr_stats* stats;
    rpl_nbr_t *nbr = rpl_neighbor_get_from_ipaddr((uip_ipaddr_t*)sender_addr);
    stats = nbr_table_get_from_lladdr(transmit_rates_table,rpl_neighbor_get_lladdr(nbr));
    stats->tx_time = data_num;
    /* LOG_INFO("Transmitrate: %d from ", stats->tx_time); */
    /* LOG_INFO_6ADDR(sender_addr); */
    /* LOG_INFO_("\n"); */
}

static void udp_rpl_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {

    static unsigned int transmit_rate = 0;
    static char str[8];
    /* LOG_INFO("Transmitrate request from: "); */
    /* LOG_INFO_6ADDR(sender_addr); */
    /* LOG_INFO_("\n"); */

    /* energest_flush(); */
    /* unsigned int tranmit_time = (unsigned int)to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)); */
    /* unsigned long total_time = to_seconds(ENERGEST_GET_TOTAL_TIME()); */

    /* transmit_rate = tranmit_time; */ 
    snprintf(str, sizeof(str), "%d", transmit_rate);
    simple_udp_sendto(&udp_rx_rpl_conn, str, strlen(str), sender_addr);

}

/* ---------------------------------------------------------------------------- */
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  /* LOG_INFO("Received request '%.*s' from ", datalen, (char *) data); */
  /* LOG_INFO_6ADDR(sender_addr); */
  /* LOG_INFO_("\n"); */
#if WITH_SERVER_REPLY
  /* send back the same string to the client as an echo reply */
  /* LOG_INFO("Sending response.\n"); */
  simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_tx_rpl_conn, UDP_RPL_CLIENT_PORT, NULL, UDP_RPL_SERVER_PORT,
                      udp_rpl_tx_callback);
  simple_udp_register(&udp_rx_rpl_conn, UDP_RPL_SERVER_PORT, NULL, UDP_RPL_CLIENT_PORT,
                      udp_rpl_rx_callback);
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);
    
  nbr_table_register(transmit_rates_table, NULL);
  
  ctimer_set(&timer_transmit, 5 * CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)), transmit_rate_callback, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
