#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define UDP_PORT 1234
#define SEND_INTERVAL		(20 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection broadcast_connection;



static struct simple_udp_connection udp_conn;
static char buffer[256];

// only called by gateway so it can boradcast message
void buildAdvertisementPackage(char * dst,char * src) {
  memset(dst, 'A', 1);
  memset(dst + 1, src + 1, 8 );
  memset(dst + 9, src + 9, 12);
  memset(dst + 23, src + 23 , 1);


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
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);
char[120] dst;
static char[8] SOFTWARE_ID = {"1.001.08"};
static char[12] SOFTWARE_VERSION = {"1.001.000.12"};
static char parent_required = '0';

 memset(dst, 'A', 1);
 memset(dst + 1, SOFTWARE_ID, 8 );
 memset(dst + 9, SOFTWARE_VERSION, 12);
 memset(dst + 23, parent_required, 1);
 buildAdvertisementPackage(buffer, dst)
 PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(broadcast_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

    printf("Sending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, "Test", 4, &addr);
    process_start(&broadcast, NULL);
  PROCESS_END();
}
