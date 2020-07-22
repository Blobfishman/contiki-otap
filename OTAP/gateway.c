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


//////LOG_INFO
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//udp related connections
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define UDP_PORT 1234
#define UDP_PORT_N 2222
static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection connection;
static struct simple_udp_connection udp_conn;


// max amount of neighbors stored in lists
#define MAX_NEIGHBOR 15

// page and update size
#define PAGESIZE 200
#define UPDATE_PACKAGE_SIZE 50

//amount of pages to be sent
static int count = 10;

#define SEND_INTERVAL		(2 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))


//stores neighbor_addr upron receiving r1
static uip_ipaddr_t neighbor_addr[MAX_NEIGHBOR];
static int currrent_neigh_pos = 0;
static char neigh_is_null = '1';

static char buffer[100];
//mesage to be send
static char file[520] = {"1.001.081.001.000.12QCMoULnHfu9Mj3Af3pivt6yWnZjT8RJuFucXw6LhMZyoipt2WCfLrEEK1gMMUk4TgyEE3tfnSkBwVS6uIHBFiqf6oLw50atiWuwJ2uRXJ5ROg2Q7IbYzY5tDmzjROkGSSjAgeBikQWWNivj8RBi6raQGngPiqJwW3SoZvOiUDgO1YJQNclXgXH96liE7RhNKc1P9zkrcYTWYCCJ4MyEdXWYDmE6njIOKBVU0dPonpquxza67l9e38X1Uf9DBgui7n8mABt0xGsw5Xye8y8GmCTcmhplDc515cLyXsxQi1GoZNUMMI2kosVBwqqroMbHrnSxX7LKpIy2CXlDmu8MgzUAc1mVJP6NNKplmSo17THlfGOEfFCkBGTfPluyzAp81iYItdyVU4Ssm5oLXnzssUcNht0CXOQNQfhswcyrFgXlvGwVdZyMdMJmUM63xUasYUAdkYfiKbiwufrSorAN21Qpcvhd1v8VNiPB5HIP2wTct8jlSpTsm"};


static int fp = 0;
static int pg_cnt = 0;
static int nbr_cnt = 0;


//called after update_process
void reset() {
  memset(neighbor_addr, 0 , sizeof neighbor_addr);
  currrent_neigh_pos = 0;
  fp = 0;
  pg_cnt = 0;
  nbr_cnt = 0;
  count = 10;
  neigh_is_null = '1';

}

//sets pointer to offset position
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

//deletes neighbor from array upon receiving r0
void fixArray(int pos) {
  int j;
  j = pos;
  for( j = pos ; j < currrent_neigh_pos; j++) {
    neighbor_addr[j] = neighbor_addr[j + 1];


  }
  currrent_neigh_pos--;
  if(currrent_neigh_pos == 0) {
    neigh_is_null = '1';
  }



  //memset(&neighbor_addr[currrent_neigh_pos], 0 , sizeof(uip_ipaddr_t));
}
PROCESS(udp_server_process, "UDP server");
PROCESS(broadcast, "UDP broadcast example process");
PROCESS(test, "TEST");

//PROCESS(gateway_process, "Gateway");
AUTOSTART_PROCESSES(&udp_server_process,&test);


//rpl callback function called when recicing rpl data
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback_rpl(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
LOG_INFO("Received Data '%.*s' from ", datalen, (char *) data);
 LOG_INFO_6ADDR(sender_addr);
 LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
// dag  root
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback_rpl);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/



//callback function for own ota-protocoll
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

  ////LOG_INFO("Received request '%.*s' from ", datalen, (char *) data);
  ////LOG_INFO_6ADDR(sender_addr);
  ////LOG_INFO_("\n");




 //Handle reponsePackage by sending the update_package to sender
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
  // r1 message ->  store neighbor
  if(data[0] == 'R' && data[1] == '1') {
    LOG_INFO("Sending update\n");


    neigh_is_null = '0';

    unsigned char update[40];
    update[0] = 'U';
    int s = 50;
    int c = 2;
    update[0] = 'U';

    //snprintf(update + 1  ,4 * sizeof(int), "%d", size);
    //snprintf(update + sizeof(int) + 2 ,4 * sizeof(int), "%d", count);
    memcpy(update + 1, &s, sizeof(s));
    memcpy(update + sizeof(c) + 1 , &c, sizeof(c));
    int dst = 0;
    int cnt = 0;
    memcpy(&dst, update + 1,  sizeof(int));
    memcpy(&cnt, update + sizeof(int) + 1,  sizeof(int));
    LOG_INFO(" value of dst is  %d and cnt  %d  \n" , dst , cnt);
    simple_udp_sendto(&connection, update, sizeof(update), sender_addr);

  }

  // r0 message -> remove from neighborlist
  if(data[0] == 'R' && data[1] == '0') {
   int i = 0;
   //delete adress
   while(i < MAX_NEIGHBOR) {
     if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
       ////LOG_INFO("Found the adress to delete \n");
       fixArray(i);
       return;
     }
     if(currrent_neigh_pos < 0) {

     }
   i++;
   }
  }
  // Next package with page number -> transmit to sender
  if(data[0] == 'N') {
    int num;
    memcpy(&num, data + 1,  sizeof(int));
    LOG_INFO("N received num is %d \n", num);
    uip_ipaddr_t dummy;
    uip_ip6addr_copy(&dummy, sender_addr);

    static int  sequence_num;
    sequence_num = 0;
    static int dummy_fp;
    int amount;
    dummy_fp = (num - 1)* PAGESIZE;
    for(amount = PAGESIZE /   UPDATE_PACKAGE_SIZE ; amount > 0 ; amount--) {
      //("IN LOOP \n");
      char response[sizeof(int) + 1 + 60] = {"S"};
      memcpy(response + 1 ,&num, sizeof(int));
      memcpy(response + 1 + sizeof(int) ,&sequence_num, sizeof(sequence_num));
      snprintf(response + 1 +  2* sizeof(int), UPDATE_PACKAGE_SIZE + 1, "%s", file + dummy_fp);
      simple_udp_sendto(&connection, response ,sizeof(response), &dummy);

      sequence_num++;
      dummy_fp += UPDATE_PACKAGE_SIZE;

    }
  }
  // retransmit package -> send page to sender
  if(data[0] == 'R' && data [1] == 'T' ) {
    int num;
    memcpy(&num, data + 2,  sizeof(int));
    LOG_INFO("RT reveiced num is %d \n", num);
    uip_ipaddr_t dummy;
    uip_ip6addr_copy(&dummy, sender_addr);

    static int  sequence_num;
    sequence_num = 0;
    static int dummy_fp;
    int amount;
    dummy_fp = (num - 1)* PAGESIZE;
    for(amount = PAGESIZE /   UPDATE_PACKAGE_SIZE ; amount > 0 ; amount--) {
      ////LOG_INFO("IN LOOP \n");
      char response[sizeof(int) + 1 + 60] = {"S"};
      memcpy(response + 1 ,&num, sizeof(int));
      memcpy(response + 1 + sizeof(int) ,&sequence_num, sizeof(sequence_num));
      snprintf(response + 1 +  2* sizeof(int), UPDATE_PACKAGE_SIZE + 1, "%s", file + dummy_fp);
      simple_udp_sendto(&connection, response ,sizeof(response), &dummy);

      sequence_num++;
      dummy_fp += UPDATE_PACKAGE_SIZE;

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
 //printf("DST value is %s' \n" ,dst);
 //building the package
 buildAdvertisementPackage(buffer, dst);
 ////LOG_INFO("STARTING BROADCAST\n");
 //start broadcast with manpiulated buffer


 process_start(&broadcast, NULL);

 PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(broadcast, ev, data)
{

  static struct etimer periodic_timer;

  static uip_ipaddr_t addr;

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      udp_rx_callback);
                      simple_udp_register(&connection, UDP_PORT_N,


                      NULL, UDP_PORT_N,
                      udp_rx_callback);




    uip_create_linklocal_allnodes_mcast(&addr);



    while(1) {

      //rebroadcast every 40 seconds
      if(neigh_is_null == '1') {

        etimer_set(&periodic_timer, SEND_INTERVAL * 20);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));



        // 3 minute till rebroadcast 16
        reset();

        char dst[40];
        static char SOFTWARE_ID[8] = {"1.001.08"};
        static char SOFTWARE_VERSION[12] = {"1.001.000.12"};
        static char parent_required[1] = {'1'};
        //fill buffer with values
         dst[0] = 'A';
         snprintf(dst + 1, 9, "%s",  SOFTWARE_ID);
         snprintf(dst + 9, 14 , "%s",  SOFTWARE_VERSION);
         snprintf(dst + 23, 1, "%s",  parent_required);
         //printf("DST value is %s' \n" ,dst);
         //building the package
        LOG_INFO("RESENDING BROADCAST \n");
        printf("Buffer value is %s' \n" ,dst);

        simple_udp_sendto(&broadcast_connection, dst, strlen(dst), &addr);

      }
       // int i = 0;
       // while(i <= currrent_neigh_pos) {
       //   //LOG_INFO("Neighbors of this node are :");
       //   //LOG_INFO_6ADDR(&neighbor_addr[i]);
       //   //LOG_INFO("\n");
       //   i++;
       // }
      //
      // ////LOG_INFO("\n");


    }
  PROCESS_END();
}
