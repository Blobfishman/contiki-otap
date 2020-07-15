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
#define MAX_NEIGHBOR 8
#define PAGESIZE 10
#define SEND_INTERVAL		(10 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define UPDATE_PACKAGE_SIZE 5
static int page_count =  PAGESIZE / UPDATE_PACKAGE_SIZE ;
static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection connection;
static uip_ipaddr_t neighbor_addr[MAX_NEIGHBOR];
static int currrent_neigh_pos = 0;
static int count = 2;
static char buffer[100];

static int send_update_to[MAX_NEIGHBOR];
static char file[100] = {"1.001.081.001.000.12ABCDWDHQWDOQWDIQWDHQWID"};
static int fp = 0;
static int next = 0;
static char next_part = '0';
static int pg_cnt = 0;
static int nbr_cnt = 0;

void reset() {
  memset(neighbor_addr, 0 , sizeof neighbor_addr);
  currrent_neigh_pos = 0;
  fp = 0;
  next = 0;
  next_part = '0';
  pg_cnt = 0;
  nbr_cnt = 0;
  count = 2;
}

void preparePage(int page, int * fp, int page_size) {
  int offset = (page - 1) * page_size;
  *fp = offset;
}
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


void fixArray(int pos) {
  int j;
  j = pos;
  for( j = pos ; j < currrent_neigh_pos; j++) {
    neighbor_addr[j] = neighbor_addr[j + 1];


  }
  currrent_neigh_pos--;
  int i = 0;
  while(i < currrent_neigh_pos) {
    LOG_INFO("Neighbors of this node are :");
    LOG_INFO_6ADDR(&neighbor_addr[i]);
    LOG_INFO("\n");
    i++;
  }


  //memset(&neighbor_addr[currrent_neigh_pos], 0 , sizeof(uip_ipaddr_t));
}

PROCESS(broadcast, "UDP broadcast example process");
PROCESS(test, "TEST");
PROCESS(jitter , "jitter");
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
  int i = 0;
  while(i <= currrent_neigh_pos) {
    LOG_INFO("Neighbors of this node are :");
    LOG_INFO_6ADDR(&neighbor_addr[i]);
    LOG_INFO("\n");
    i++;
  }



 //Handle reponsePackage by sending the update_package to sender
 //TODO send whole package with some delay
 if(data[0] == 'I' && data[1] == 'P') {
   int i;
   for(i = 0 ; i < currrent_neigh_pos; i++) {
    if( uip_ip6addr_cmp(&neighbor_addr[i], sender_addr) ) {
       return;
     }
   }
   uip_ip6addr_copy(&neighbor_addr[currrent_neigh_pos], sender_addr);
   currrent_neigh_pos++;
 }

  if(data[0] == 'R' && data[1] == '1') {
    LOG_INFO("Sending update to :");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO("\n");
    int k;
    for(k = 0; k <= currrent_neigh_pos ; k++) {
      if(uip_ip6addr_cmp(&neighbor_addr[k],&(*sender_addr)) ) {
        send_update_to[k] = 1;
      }
    }

    unsigned char update[9] = {"U"};
    int size = UPDATE_PACKAGE_SIZE;
    int count = 2;
    //snprintf(update + 1  ,4 * sizeof(int), "%d", size);
    //snprintf(update + sizeof(int) + 2 ,4 * sizeof(int), "%d", count);
    memcpy(update + sizeof(char), &size, sizeof(size));
    memcpy(update + sizeof(size) + sizeof(char), &count, sizeof(count));
    int dst = 0;
    int cnt = 0;
    memcpy(&dst, update + sizeof(char),  sizeof(size));
    memcpy(&cnt, update + sizeof(size) + sizeof(char),  sizeof(size));
    LOG_INFO(" value of dst is  %d and cnt  %d  \n" , dst , cnt);
    simple_udp_sendto(&connection, update, sizeof(update), sender_addr);

  }


  if(data[0] == 'R' && data[1] == '0') {
   int i = 0;
   //delete adress
   while(i < MAX_NEIGHBOR) {
     if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
       LOG_INFO("Found the adress to delete \n");
       fixArray(i);
       return;
     }
     if(currrent_neigh_pos == 0) {
       reset();
     }
   i++;
   }
  }

  if(data[0] == 'N') {
    int num;
    memcpy(&num, data + sizeof(char),  sizeof(int));
    if(num > count) {
      return;
    }
    int k;
    for(k = 0; k < currrent_neigh_pos; k++) {
      if(uip_ip6addr_cmp(&neighbor_addr[k],&(*sender_addr)) ) {
        send_update_to[k] = num;
        LOG_INFO("NUM is %d \n", num);
      }
    }
  }

  if(data[0] == 'R' && data [1] == 'T' ) {
    int num;
    memcpy(&num, data + 2 * sizeof(char),  sizeof(int));
    static int d;
    LOG_INFO("num is %d \n", num);

    for(d = 0; d < currrent_neigh_pos; d++) {
      if(uip_ip6addr_cmp(&neighbor_addr[d],&(*sender_addr)) ) {
        LOG_INFO("set the send_updateto \n");
        send_update_to[d] = num;
      }
    }
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
  static uip_ipaddr_t addr;

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
    static int k = 0;
    while(1) {
      k++;
      //LOG_INFO("VALUE OF k %d \n" , k);
      if(k % 40 == 0 && currrent_neigh_pos == 0) {
        reset();
        LOG_INFO("RESENDING BROADCAST \n");
        printf("Buffer value is ''%s' \n" ,buffer);
        simple_udp_sendto(&broadcast_connection, buffer, strlen(buffer), &addr);
        k = 0;
      }
      // int i = 0;
      // while(i <= currrent_neigh_pos) {
      //   LOG_INFO("Neighbors of this node are :");
      //   LOG_INFO_6ADDR(&neighbor_addr[i]);
      //   LOG_INFO("\n");
      //   i++;
      // }
      //
      // LOG_INFO("\n");
      etimer_set(&periodic_timer, SEND_INTERVAL);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        etimer_reset(&periodic_timer);

       static int l ;

       for(l = 0 ; l <= currrent_neigh_pos ; l++) {
         if(send_update_to[l] != 0 ) {
            LOG_INFO("value of l is %d \n", l);
            static int sequence_num = 0;
            preparePage(send_update_to[l], &fp, PAGESIZE );
            while(page_count > 0) {
              static char dst[UPDATE_PACKAGE_SIZE + sizeof(int) + 20] = {"S"};
              memcpy(dst + sizeof(char) , &sequence_num, sizeof(sequence_num));
              snprintf(dst + 1 + sizeof(sequence_num)  , UPDATE_PACKAGE_SIZE + 1, "%s",  file + fp);
              fp+= UPDATE_PACKAGE_SIZE ;
              LOG_INFO("Sending update to : \n");
              LOG_INFO("IPADDR is");
              LOG_INFO_6ADDR(&neighbor_addr[l]);
              LOG_INFO(" VALUE OF BUFFER IS : %s \n" , dst);



              simple_udp_sendto(&connection, dst, sizeof(dst) , &neighbor_addr[l]);
              page_count--;
              sequence_num++;
              }
              sequence_num = 0;
              send_update_to[l] = 0;
              fp = 0;
              page_count =  PAGESIZE / UPDATE_PACKAGE_SIZE;

            }
         }
         l = 0;
    }
  PROCESS_END();
}


PROCESS_THREAD(jitter, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  PROCESS_BEGIN();
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  etimer_reset(&periodic_timer);
  etimer_set(&send_timer, SEND_TIME);

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
  PROCESS_EXIT();

  PROCESS_END();

}
