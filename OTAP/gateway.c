#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"

//LOG_INFO
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define UDP_PORT 1234
#define UDP_PORT_N 2222


#define SEND_INTERVAL		(5 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection connection;
static char buffer[100];


/**
* @brief builds the advertisement
* @param [in] dst destination of the package
* @param [in] src source of the package
* @return void
* @details manipulates the dst pointer
*/
void buildAdvertisementPackage(char * dst,char * src) {
  memset(dst, 'A', 1);
  snprintf(dst + 1, 9, "%s\n",  (src + 1));
  snprintf(dst + 9, 14, "%s\n",  (src + 9));
  snprintf(dst + 23 , 1, "%s\n",  (src + 23));

}

PROCESS(broadcast, "UDP broadcast example process");
PROCESS(test, "TEST");
//PROCESS(gateway_process, "Gateway");
AUTOSTART_PROCESSES(&test);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  // Here we handle incoming messages:

  LOG_INFO("Received request '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

 //Handle reponsePackage by sending the update_package to sender
 //TODO send whole package with some delay
  if(data[0] == 'R' && data[1] == '1') {
    LOG_INFO("Sending update to :");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO("\n");
    char update[20] = "UPDATE";
    simple_udp_sendto(&connection, update, sizeof(update), sender_addr);
  }



}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test, ev, data)
{
  PROCESS_BEGIN();

//Placeholder until we recive a real file
char dst[120];
static char SOFTWARE_ID[8] = {"1.001.08"};
static char SOFTWARE_VERSION[12] = {"1.001.000.12"};
static char parent_required[1] = {'1'};

//fill buffer with values
 memset(dst, 'A', 1);
 snprintf(dst + 1, 9, "%s",  SOFTWARE_ID);
 snprintf(dst + 9, 14 , "%s",  SOFTWARE_VERSION);
 snprintf(dst + 23, 1, "%s",  parent_required);
 printf("DST value is %s' \n" ,dst);
 //building the package
 buildAdvertisementPackage(buffer, dst);
 LOG_INFO("STARTING BROADCAST\n");
 //start broadcast with manpiulated buffer
 process_start(&broadcast, NULL);

 PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(broadcast, ev, data)
{

  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      udp_rx_callback);


 simple_udp_register(&connection, UDP_PORT_N,
                      NULL, UDP_PORT_N,
                      udp_rx_callback);

  etimer_set(&periodic_timer, SEND_INTERVAL);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    printf("Sending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    printf("Buffer value is ''%s' \n" ,buffer);
    simple_udp_sendto(&broadcast_connection, buffer, strlen(buffer), &addr);
  PROCESS_END();
}
