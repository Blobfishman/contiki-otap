/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
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
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "simple-udp.h"
#include "sys/log.h"
#include <stdio.h>

#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"

#define PAGESIZE 250
#define UDP_PORT_BROADCAST 1234
#define UDP_PORT_UNICAST 2222
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
static struct simple_udp_connection udp_conn;
static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection connection;

//lists for storing adresses where data is sent to , maybe cut for real lists
#define MAX_NEIGHBOR 8
static int page = 0;
static  uip_ipaddr_t parent_addr ;
static char received_n_from[MAX_NEIGHBOR];
static char rt_mode = '0';
static  uip_ipaddr_t neighbor_addr[MAX_NEIGHBOR];
static int currrent_neigh_pos = 0;
static int count = 0;
static int size = 0;
static int j = 0;
static int rt_val = 0;
static int count_received = 0;
static int count_n_received_from = 0;
static int amount_page_received = 0;
static char retransmit_enable = '0';
static char send_n = '0';
static int s;
static char seq_receieved[PAGESIZE]  = {"0"};
// timer to wait for events
#define SEND_INTERVAL		(16 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

// debuggginginfo
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

static char parent_is_null =  '1';
// store if parent needs update, why is this important tho?
static char parent_required = 0;
static char required[1];

static char SOFTWARE_ID[10] = {"1.001.08\0"};
static char SOFTWARE_VERSION[14] = {"1.001.000.11\0"};
static char ROM[500];
static int rom_p = 0;
static char mode = '0';
//mode 0 = no update , mode 1 = update
//static char mode = 0
PROCESS(broadcast_example_process, "UDP broadcast example process");
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&broadcast_example_process, &udp_client_process);
static char file[PAGESIZE];
static int fp = 0;

void reset() {
  //LOG_INFO("reset called \n");
  retransmit_enable = '0';
  amount_page_received = 0;
  rt_mode = '0';
  currrent_neigh_pos = 0;
  parent_is_null = '1';
  parent_required = '0';
  mode = '0';
  required[0] = '0';
  count = 0;
  size = 0;
  count_n_received_from = 0;
  count_received = 0;
  memset(neighbor_addr, 0 , sizeof (neighbor_addr));
  memset(file, 0 , sizeof file);
  fp = 0;
  memset(&parent_addr, 0 , sizeof parent_addr);
  retransmit_enable = '0';
  amount_page_received = 0;
  count_n_received_from = 0;
}
void fixArray(int pos) {
  int j;
  j = pos;
  for( j = pos ; j < currrent_neigh_pos; j++) {
    neighbor_addr[j] = neighbor_addr[j + 1];


  }
  currrent_neigh_pos--;
}

/**
* @brief handles advertisement, stores parent  and boradcasts advertisement package to neighbors
* @param [in] @src datapackage we Reviced via udp
* @param [in] @send_addr sender of this package
* @return void
*/
void handleAdvertisementPackage(char* src, const uip_ipaddr_t send_addr) {
  int i;
  i = 0;
  //if it is neighbor dont do anything
  while(i < MAX_NEIGHBOR) {
     if(uip_ip6addr_cmp(&neighbor_addr[i],&send_addr) ) {
         return;
     }
     i++;
   }

  char update_softwareid[9];
  char update_softwareversion[13];
  char update_required[1];
  //copy values from src  to check them
  snprintf(update_softwareid, sizeof(update_softwareid), "%s", src + 1);
  snprintf(update_softwareversion, 13, "%s", src + 9);
  update_required[0] = *(src + 21);

  // store parent_addr if is null
  if(parent_is_null == '1') {
    parent_is_null = '0';
    // parent needs update and was first one
    if(update_required [0] == '1') {
      parent_required = '1';

    }
    //copy sender addr to parent
    uip_ip6addr_copy(&parent_addr,&send_addr);
    simple_udp_sendto(&connection, "IP", sizeof("IP"), &parent_addr);

      //check if update on node is needed,
         if(strncmp(SOFTWARE_ID,update_softwareid,8) == 0) {
           if(strncmp(SOFTWARE_VERSION,update_softwareversion,12) != 0) {
              required[0] = '1';
              LOG_INFO("Update required\n");
    	     }
         }
         else {
          required[0] = '0';
         }
        //multicast  packet to neighbor adress will be stored when reciving IP packet
       //we manipulate buffer so that the process uses its value to send the data
        char buffer[40];
        snprintf(buffer, 22, "%s" , src );
        *(buffer + 21) = required[0];
        uip_ipaddr_t addr;
        //multicast adress
        uip_create_linklocal_allnodes_mcast(&addr);
        simple_udp_sendto(&broadcast_connection, buffer, sizeof buffer, &addr);
        memset(buffer,0, sizeof buffer);
        mode = '1';
  }
   else {
     return;
   }



  }

/*---------------------------------------------------------------------------*/





/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  //printf("Data received on port %d  with length %d and value %s \n",
    //     receiver_port, datalen, data);
  ////////LOG_INFO("Sender addr :");
  ////////LOG_INFO_6ADDR(sender_addr);
  ////////LOG_INFO("\n");
  ////////LOG_INFO("Receiver  addr :");
  ////////LOG_INFO_6ADDR(receiver_addr);
  ////////LOG_INFO("\n");


  // upong reciving IP package store neighbor in list if it fits
  //TODO adjust  array so that after deleting element another one can be stored
  if(data[0] == 'I' && data[1] == 'P') {
    if(uip_ip6addr_cmp(&parent_addr , sender_addr)) {
      return;
    }
    if(currrent_neigh_pos < MAX_NEIGHBOR)
    {
      int i = 0;
      while(i < MAX_NEIGHBOR) {
        if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
            return;
        }
        i++;
      }
      uip_ip6addr_copy(&(neighbor_addr[currrent_neigh_pos]) ,sender_addr);
       currrent_neigh_pos++;

    }
  }

  // upon reciving advertisement call handler function
  if(data[0] == 'A') {
    char dst[100];
    strncpy(dst, (char*) data, datalen);
    handleAdvertisementPackage(dst,*sender_addr);
  }

  // denyResponse -> deletee neighbor from list
  // TODO adjust so that after deleting another neighbor can be stored, maybe use a list?
  if(data[0] == 'R' && data[1] == '0') {
    int i = 0;
    //delete adress
    while(i < MAX_NEIGHBOR) {
      if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
        fixArray(i);
        break;
      }
    i++;
    }
    //check if neighborlist is empty now
    if((currrent_neigh_pos) != 0) {
      return;
    }
    if(amount_page_received == count) {
      int i;
      for (i = 0 ;i < 10 ; i++) {
        simple_udp_sendto(&connection, "R0" , sizeof("R0"), &parent_addr);
      }
      reset();
    }
    // if empty send response depending on whtether update is needed or not
    if(required[0] == '1') {
        simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
        return;
    } else {
        simple_udp_sendto(&connection, "R0", sizeof("R0"), &parent_addr);
      }
    }

    // requestResponse -> retransmit request to parent
    if(data[0] == 'R' && data[1] == '1') {
      if(size !=0 && count != 0) {
        if(!uip_ip6addr_cmp(&parent_addr , sender_addr)) {
          int i = 0;
          while(i < MAX_NEIGHBOR) {
            if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
                    break;

            }
            i++;
            }
            if(i > currrent_neigh_pos) {
              uip_ip6addr_copy(&(neighbor_addr[currrent_neigh_pos]) ,sender_addr);

               currrent_neigh_pos++;
            }
        }

          unsigned char update[9] = {"U"};
          memcpy(update + sizeof(char), &size, sizeof(size));
          memcpy(update + sizeof(size) + sizeof(char), &count, sizeof(count));

          simple_udp_sendto(&connection, update, sizeof(update), sender_addr);
          return;
      }
      if(uip_ip6addr_cmp(&parent_addr , sender_addr)) {
        return;
      }
      if(currrent_neigh_pos < MAX_NEIGHBOR)
      {

        int i = 0;
        while(i < MAX_NEIGHBOR) {

          if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
                  simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
                  return;
          }
          i++;
        }
        uip_ip6addr_copy(&(neighbor_addr[currrent_neigh_pos]) ,sender_addr);
         currrent_neigh_pos++;
         simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);

      }

    }

    //update_package -> if needed apply if not retransmit to neighbors
    if(data[0] == 'U') {
      if(size !=0 && count != 0) {
        mode = '0';
        return;
      }
      mode = '0';
      retransmit_enable = '1';
      memcpy(&size, data + 1 , sizeof(size));
      memcpy(&count, data + 1 + sizeof(count) , sizeof(size));

      //if has neighbors send to them
      if(currrent_neigh_pos != 0) {
        int i = 0;
        while(i < currrent_neigh_pos) {
          simple_udp_sendto(&connection, data, datalen, &neighbor_addr[i]);
          i++;
        }

    }
    }


    if(data[0] == 'S') {
      send_n = '0';
      retransmit_enable = '0';
      unsigned const  char * d = data;
      int pg;
      memcpy(&pg, d + 1, sizeof(int));
      if(pg != amount_page_received + 1) {
        return;
      }

      count_received = 0;

      rt_val = amount_page_received + 1;

      if(size == 0 && count == 0) {
        simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
        return;
      }
      //unsigned const char * d = data +1 + 2* sizeof(int);
      //LOG_INFO("Value of data is : %s \n", d + 1 + 2* sizeof(int));
      //retransmit to every possible child
      int sequence_num;
      memcpy(&sequence_num, d + 1 + sizeof(int) , sizeof(sequence_num));
      int amount = PAGESIZE / size;

      //write offset of seq number to Pagebuffer
      if(seq_receieved[sequence_num] != '1') {
          seq_receieved[sequence_num] = '1';
            memcpy(&file[sequence_num * size],  d + 1 + 2 * sizeof(int), size);
            //LOG_INFO("Value of file %s and seq_num %d \n" ,file,sequence_num);

      }
      int m;
      count_received = 0;
      for(m = PAGESIZE/size ; m >= 0 ; m --) {
        if(seq_receieved[m] == '1') {
          count_received++;
        }
      }
      //LOG_INFO("count_received %d amoun : %d and page %d and count %d and amount_page_received %d \n",count_received, amount, *(d + 1),count, amount_page_received);
      // every sequence_num received
      if(count_received == amount) {
        retransmit_enable = '0';
        memset(seq_receieved, '0', sizeof(seq_receieved));
        count_received = 0;

        int amount;
        int sequence_num = 0;
        int dummy_fp = 0;
        int pg;
        pg = amount_page_received + 1;
        for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
          char response[sizeof(int) + 1 + 60] = {"S"};
          memcpy(response + 1 ,&pg, sizeof(int));
          memcpy(response + 1 + sizeof(int) ,&sequence_num, sizeof(int));
          snprintf(response + 1 + 2 * sizeof(int), size + 1, "%s", file + dummy_fp);
          int i = 0;
          while(i < currrent_neigh_pos) {

            simple_udp_sendto(&connection, response , sizeof(response), &neighbor_addr[i]);
            i++;
          }
          sequence_num++;
          dummy_fp += size;
        }


          //reset
          count_received = 0;
          fp = 0;
          amount_page_received +=  1;
          //write to Rom , for now its a Buffer
          if(required[0] == '1') {
            memcpy(ROM + rom_p, file, PAGESIZE);
            rom_p += PAGESIZE;
          }

          //reset Pagebuffer
          if(count == amount_page_received) {
            send_n = '0';
            retransmit_enable = '0';
	           memset(received_n_from, '0', sizeof(received_n_from));
	            count_n_received_from = 0;


            snprintf(SOFTWARE_ID, 9, "%s", ROM);
            snprintf(SOFTWARE_VERSION, 13, "%s", ROM + 8);
            if(required[0] == '1') {


            if(strncmp(ROM,"1.001.081.001.000.12QCMoULnHfu9Mj3Af3pivt6yWnZjT8RJuFucXw6LhMZyoipt2WCfLrEEK1gMMUk4TgyEE3tfnSkBwVS6uIHBFiqf6oLw50atiWuwJ2uRXJ5ROg2Q7IbYzY5tDmzjROkGSSjAgeBikQWWNivj8RBi6raQGngPiqJwW3SoZvOiUDgO1YJQNclXgXH96liE7RhNKc1P9zkrcYTWYCCJ4MyEdXWYDmE6njIOKBVU0dPonpquxza67l9e38X1Uf9DBgui7n8mABt0xGsw5Xye8y8GmCTcmhplDc515cLyXsxQi1GoZNUMMI2kosVBwqqroMbHrnSxX7LKpIy2CXlDmu8MgzUAc1mVJP6NNKplmSo17THlfGOEfFCkBGTfPluyzAp81iYItdyVU4Ssm5oLXnzssUcNht0CXOQNQfhswcyrFgXlvGwVdZyMdMJmUM63xUasYUAdkYfiKbiwufrSorAN21Qpcvhd1v8VNiPB5HIP2wTct8jlSpTsm",200) == 0) {

              LOG_INFO("Update successfully received \n");
            }
            }
            if(currrent_neigh_pos == 0) {
              int i;
              for (i = 0 ;i < 10 ; i++) {
                simple_udp_sendto(&connection, "R0" , sizeof("R0"), &parent_addr);
              }

              reset();
              amount_page_received = 0;
              return;
            }
          }

          //request next page if no neighbors present
          if(currrent_neigh_pos == 0) {
            //////////LOG_INFO("Yoo im here \n");
            char response[1 + sizeof(int) + 30];
            response[0] = 'N';
            int pg = amount_page_received + 1;
            memcpy(response + 1 ,&pg, sizeof(pg));
            send_n = '1';
            simple_udp_sendto(&connection, response, sizeof(response), &parent_addr);
            return;
          }


        return;
        }
        retransmit_enable = '1';

    }
    //retransmit N to parent since we should have completed the page already
    if(data[0] == 'N') {
      //LOG_INFO("N received \n");
      int pg;
      memcpy(&pg, data + 1, sizeof(int));

      //LOG_INFO("value of N %d \n", pg);

      //LOG_INFO("In N value of count %d and amount %d and  page %d \n" , count, amount_page_received, d);

      if(pg > count) {
        return;
      }
      if(pg == amount_page_received) {
        s = 0;
        for(s = 0; s < currrent_neigh_pos ; s++) {
          if(uip_ip6addr_cmp(&neighbor_addr[s] , sender_addr)) {
            break;
          }
        }


           ////LOG_INFO("Set rt1");

           //LOG_INFO("page retransmitting %d \n", pg);

           int amount;
           static int  sequence_num;
           sequence_num = 0;
           int dummy_fp;
           dummy_fp = 0;

           for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
             ////LOG_INFO("IN LOOP \n");
             char response[sizeof(int) + 1 + 60] = {"S"};
             memcpy(response + 1 ,&pg, sizeof(int));
             memcpy(response + 1 + sizeof(int) ,&sequence_num, sizeof(sequence_num));
             snprintf(response + 1 + 2 * sizeof(int), size + 1, "%s", file + dummy_fp);

             simple_udp_sendto(&connection, response ,sizeof(response), &neighbor_addr[s]);

             sequence_num++;
             dummy_fp += size;
      }
      return;
}

      count_n_received_from = 0;
      int i;
      if(pg == amount_page_received + 1) {
      for(i = 0 ; i < currrent_neigh_pos ; i++) {
        if(uip_ip6addr_cmp(&(neighbor_addr[i]), sender_addr ) ) {
          received_n_from[i] = '1';

        }
        if(received_n_from[i] == '1') {
          ////////LOG_INFO("incremented n \n");
          count_n_received_from+= 1;
        }
      }
    } else {
      memset(received_n_from, '0' , sizeof(received_n_from));
      count_n_received_from = 0;
    }


      //LOG_INFO("Value count_n_received_from after loop %d\n" , count_n_received_from);
      //LOG_INFO("Value of currentneighpos %d \n", currrent_neigh_pos);
      if( (count_n_received_from ) == currrent_neigh_pos) {
         ////LOG_INFO("Sending to parent \n");
          memset(received_n_from, '0' , sizeof(received_n_from));
          count_n_received_from = 0;
          int page;
          memcpy(&page, data +1, sizeof(int));
          //////LOG_INFO("\n page is %d \n", page);
          simple_udp_sendto(&connection, data, datalen, &parent_addr);
          send_n = '1';
          count_n_received_from = 0;
          memset(received_n_from, 0 , sizeof(received_n_from));
      }
      count_n_received_from = 0;
    }


    if(data[0] == 'R'  && data[1] == 'T') {
      int p;
      memcpy(&p, data + 2, sizeof(p));
      // LOG_INFO("RT received ");
      // LOG_INFO("Addr is :");
      // LOG_INFO_6ADDR(sender_addr);
      // LOG_INFO("\n");

       s = 0;
       for(s = 0; s < currrent_neigh_pos ; s++) {
         if(uip_ip6addr_cmp(&neighbor_addr[s] , sender_addr)) {
           break;
         }
       }

      if(p == amount_page_received) {
          ////LOG_INFO("Set rt1");
          //LOG_INFO("page retransmitting %d \n", p);

          int amount;
          static int  sequence_num;
          sequence_num = 0;
          int dummy_fp;
          dummy_fp = 0;
          int pg = p;
          for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
            ////LOG_INFO("IN LOOP \n");
            char response[sizeof(int) + 1 + 60] = {"S"};
            memcpy(response + 1 ,&pg, sizeof(int));
            memcpy(response + 1 + sizeof(int) ,&sequence_num, sizeof(sequence_num));
            snprintf(response + 1 + 2 * sizeof(int), size + 1, "%s", file + dummy_fp);

            simple_udp_sendto(&connection, response ,sizeof(response), &neighbor_addr[s]);

            sequence_num++;
            dummy_fp += size;

          }
      } else {
        return;
        if(amount_page_received + 1 == p) {
        char response[sizeof(int) + 10] = {"RT"};
        memcpy(response + 2 ,&p, sizeof(int));
        simple_udp_sendto(&connection, response ,sizeof(response), &parent_addr);
        ////////LOG_INFO("Dont have this page yet \n");
        }
        return;
      }
    }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  //uip_ipaddr_t addr;

  PROCESS_BEGIN();
  //broadcast
  simple_udp_register(&broadcast_connection, UDP_PORT_BROADCAST,
                      NULL, UDP_PORT_BROADCAST,
                      receiver);
 //specific ip-adress
 simple_udp_register(&connection, UDP_PORT_UNICAST,
                    NULL, UDP_PORT_UNICAST,
                    receiver);

  //timer for delay
  etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    //printf("Sending broadcast\n");
    //uip_create_linklocal_allnodes_mcast(&addr);
    //simple_udp_sendto(&broadcast_connection, "Test", 4, &addr

    // Periodically log neighbor and parent list
    int i = 0;
    while(i < currrent_neigh_pos) {
      ////LOG_INFO("Neighbors of this node are :");
      ////LOG_INFO_6ADDR(&neighbor_addr[i]);
      i++;
    }
   //////LOG_INFO("Parent of this node is :");
   //  ////////LOG_INFO_6ADDR(&parent_addr);
   //  ////////LOG_INFO("\n");
    if(j % 20 == 0 ) {
      ////LOG_INFO("\n");
      ////////LOG_INFO("SOFTWARE_VERSION is %s \n" , SOFTWARE_VERSION );
      ////////LOG_INFO("SOFTWARE_ID is %s \n" , SOFTWARE_ID);
    }
    // sending denyResponse if neighborlist is empty after specific time period
    if(retransmit_enable == '1') {

      int pg = amount_page_received + 1;
      char retransmit_request[20 + sizeof(int)] = {"RT"};
      memcpy(retransmit_request + 2 ,&pg, sizeof(pg));
      if(retransmit_enable == '1') {

      }
        //LOG_INFO("Sending rt with page %d \n", pg);
      simple_udp_sendto(&connection, retransmit_request, sizeof(retransmit_request), &parent_addr);
    }

    if(rt_mode == '1') {
      ////LOG_INFO("IN RT \n");
      ////LOG_INFO(" page %d amount_received %d \n", page, amount_page_received)
      etimer_set(&periodic_timer, SEND_INTERVAL );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      etimer_set(&send_timer, SEND_TIME);
      if(page == (amount_page_received) && page != 0) {

        //wait 30 seconds till retransmit
        static int amount;


        static int  sequence_num;
        sequence_num = 0;
        static int dummy_fp;
        int pg = page;
        for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
          ////////LOG_INFO("IN LOOP \n");
          char response[sizeof(int) + 1 + 60] = {"S"};
          memcpy(response + 1 ,&pg, sizeof(int));
          memcpy(response + 1 + sizeof(int) ,&sequence_num, sizeof(sequence_num));
          snprintf(response + 1 +  2 * sizeof(sequence_num), size + 1, "%s", file + dummy_fp);
          simple_udp_sendto(&connection, response ,sizeof(response), &neighbor_addr[s]);

          sequence_num++;
          dummy_fp += size;

        }
        rt_mode = '0';
      }


}

	 if(send_n == '1') {
      retransmit_enable = '0';
      //LOG_INFO("Sending n \n");
      etimer_set(&periodic_timer, SEND_INTERVAL );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      etimer_set(&send_timer, SEND_TIME);
      char response[1 + sizeof(int) + 30] = {"N"};
      int pg = amount_page_received + 1;
      memcpy(response + 1 ,&pg, sizeof(pg));
      simple_udp_sendto(&connection, response, sizeof(response), &parent_addr);
    }

    if(mode == '1') {
      etimer_set(&periodic_timer, SEND_INTERVAL );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      etimer_set(&send_timer, SEND_TIME);
      if((currrent_neigh_pos) == 0) {

        if(required[0] == '1') {
            simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
        }
        else {
          int i = 0;
          for(i = 0 ; i < 10 ; i++) {
              simple_udp_sendto(&connection, "R0", sizeof("R0"), &parent_addr);
          }

          reset();

        }
      }
    }


    j++;
    if(j == 100) {
      j = 0;
    }
    }
  PROCESS_END();
}

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

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback_rpl);

  etimer_set(&periodic_timer, SEND_INTERVAL * 0.5);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;
    } else {
      //////////LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL / 5);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
