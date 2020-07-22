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

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static struct ctimer timer_energy;
static struct ctimer timer_app;
static struct ctimer timer_transmit;

static struct simple_udp_connection udp_conn;

uip_ipaddr_t dest_ipaddr;
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
    static int rain = 0;
    /* rearm the ctimer */
    ctimer_reset(&timer_app);
    if(rain == 101){
        rain = 0;
    }
    rain += 1;
    if (NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        /* Send to DAG root */
        LOG_INFO("Sending rain: %d %% request to ", rain);
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");
      snprintf(str, sizeof(str), "Rain: %d %%", rain);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
    } else {
        LOG_INFO("Not reachable yet\n");
    }
}
void transmit_rate_callback(void *ptr){
    ctimer_reset(&timer_transmit);

    rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);
    while (nbr != NULL){
        uip_ipaddr_t *receiver_ipaddr =  rpl_neighbor_get_ipaddr(nbr);
        
        const struct link_stats *link;
        link = link_stats_from_lladdr(rpl_neighbor_get_lladdr(nbr));
        LOG_INFO("etx: %d, rssi: %d from ", link->etx, link->rssi);
        LOG_INFO_6ADDR(receiver_ipaddr);
        LOG_INFO_("\n");

        nbr = nbr_table_next(rpl_neighbors, nbr);

    }
    
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
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT,
                        udp_rx_callback);
    

    ctimer_set(&timer_energy, 20 * CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)), energy_callback, NULL);
    ctimer_set(&timer_app, 4 * CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)), app_callback, NULL);
    ctimer_set(&timer_transmit, 10 * CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)), transmit_rate_callback, NULL);
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/