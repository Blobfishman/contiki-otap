#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "random.h"
#include "sys/energest.h"
#include "net/nbr-table.h"
#include "net/routing/rpl-lite/rpl-neighbor.h"
#include "net/link-stats.h"
#include "sys/log.h"

#include "myrpl_conf.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UDP_RPL_CLIENT_PORT 4321
#define UDP_RPL_SERVER_PORT 1234

static struct ctimer timer_energy;
static struct ctimer timer_app;
static struct ctimer timer_transmit;

static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_tx_rpl_conn;
static struct simple_udp_connection udp_rx_rpl_conn;

uip_ipaddr_t dest_ipaddr;

NBR_TABLE_GLOBAL(struct nbr_stats, transmit_rates_table);
/*---------------------------------------------------------------------------*/
static unsigned long to_seconds(uint64_t time) {
    return (unsigned long)(time / ENERGEST_SECOND);
}
/*---------------------------------------------------------------------------*/
void energy_callback(void *ptr) {
    static char str[128];
    /* rearm the ctimer */
    ctimer_reset(&timer_energy);

    if (NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        /* Send to DAG root */
        energest_flush();

        snprintf(str, sizeof(str),
                 "CPU=ON:%4lus LPM:%4lus, DEEP_LPM:%4lus; "
                 "RADIO=LISTEN:%4lus,TRANSMIT:%4lus,OFF:%4lus",
                 to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
                 to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
                 to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
                 to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
                 to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
                 to_seconds((ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT)) - energest_type_time(ENERGEST_TYPE_LISTEN)));

        simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
        LOG_INFO("Sending ");
        LOG_INFO_(str);
        LOG_INFO_(" to ");
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");

    } else {
        LOG_INFO("Not reachable yet\n");
    }
}

void app_callback(void *ptr) {
    static char str[32];
    static unsigned short temp = 0;
    /* rearm the ctimer */
    ctimer_reset(&timer_app);
    if(temp == 51){
        temp = 0;
    }
    temp += 1;
    if (NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        /* Send to DAG root */
        LOG_INFO("Sending temp: %d C request to ", temp);
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");
      snprintf(str, sizeof(str), "temp: %d C", temp);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
    } else {
        LOG_INFO("Not reachable yet\n");
    }
}

void transmit_rate_callback(void *ptr){
    ctimer_reset(&timer_transmit);
    static char str[] = "transmit";
    struct nbr_stats* stats;

    rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);
    while (nbr != NULL){
        stats = nbr_table_get_from_lladdr(transmit_rates_table, rpl_neighbor_get_lladdr(nbr));
        if(stats == NULL){
            stats = nbr_table_add_lladdr(transmit_rates_table, rpl_neighbor_get_lladdr(nbr), NBR_TABLE_REASON_UNDEFINED, NULL);
        }
        uip_ipaddr_t *receiver_ipaddr =  rpl_neighbor_get_ipaddr(nbr);
        /* LOG_INFO("Transransmitrate: %ds from ", stats->tx_time); */
        /* LOG_INFO_6ADDR(receiver_ipaddr); */
        /* LOG_INFO_("\n"); */
        
        const struct link_stats *link;
        link = link_stats_from_lladdr(rpl_neighbor_get_lladdr(nbr));
        LOG_INFO("etx: %d, rssi: %d from ", link->etx, link->rssi);
        LOG_INFO_6ADDR(receiver_ipaddr);
        LOG_INFO_("\n");

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

    LOG_INFO("Received trasmitrate '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    unsigned short data_num = 0;
    unsigned short base = 1;
    unsigned short i;
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
}

static void udp_rpl_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {

    unsigned short transmit_rate = 0;
    static char str[32];

    energest_flush();
    transmit_rate = (unsigned short)((to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT))*1000)/to_seconds(ENERGEST_GET_TOTAL_TIME()));
    snprintf(str, sizeof(str), "%d", transmit_rate);
    simple_udp_sendto(&udp_rx_rpl_conn, str, strlen(str), sender_addr);

}
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {
    LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
    LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
    LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data) {
    PROCESS_BEGIN();

    /* Initialize UDP connection */
    simple_udp_register(&udp_tx_rpl_conn, UDP_RPL_CLIENT_PORT, NULL, UDP_RPL_SERVER_PORT,
                        udp_rpl_tx_callback);
    simple_udp_register(&udp_rx_rpl_conn, UDP_RPL_SERVER_PORT, NULL, UDP_RPL_CLIENT_PORT,
                        udp_rpl_rx_callback);
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT,
                        udp_rx_callback);
    
    nbr_table_register(transmit_rates_table, NULL);

    ctimer_set(&timer_energy, 10 * CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)), energy_callback, NULL);
    ctimer_set(&timer_app, 1 * CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)), app_callback, NULL);
    ctimer_set(&timer_transmit, 5 * CLOCK_SECOND, transmit_rate_callback, NULL);
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/