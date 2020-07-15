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
#include <string.h>


#define PAGESIZE 10
#define UDP_PORT_BROADCAST 1234
#define UDP_PORT_UNICAST 2222
static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection connection;

//lists for storing adresses where data is sent to , maybe cut for real lists
#define MAX_NEIGHBOR 8
static int page = 0;
static  uip_ipaddr_t parent_addr ;
static uip_ipaddr_t rt_adress[MAX_NEIGHBOR];
static char received_n_from[MAX_NEIGHBOR];
static int current_rt = 0;
static char rt_mode = '0';
static  uip_ipaddr_t neighbor_addr[MAX_NEIGHBOR];
static int currrent_neigh_pos = 0;
static int next = 0;
static int count = 0;

static int size = 0;
  static int j = 0;
static int count_received = 0;
static int page_count = 0;
static int sequence_num;
static int count_n_received_from = 0;
static int amount_page_received = 0;
static char retransmit_enable = '0';
static char send_n = '0';
static int s;
// timer to wait for events
#define SEND_INTERVAL		(10 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

// debuggginginfo
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

static char parent_is_null[1] = {"1"};
// store if parent needs update, why is this important tho?
static char parent_required = 0;
static char required[1];

static char SOFTWARE_ID[10] = {"1.001.08\0"};
static char SOFTWARE_VERSION[14] = {"1.001.000.11\0"};
static char ROM[100];
static int rom_p = 0;
static char mode = '0';
//mode 0 = no update , mode 1 = update
//static char mode = 0
PROCESS(broadcast_example_process, "UDP broadcast example process");

AUTOSTART_PROCESSES(&broadcast_example_process);
static char file[PAGESIZE];
static int fp = 0;

void reset() {
  currrent_neigh_pos = 0;
  sequence_num = 0;
  parent_is_null[0] = '1';
  parent_required = '0';
  mode = '0';
  required[0] = '0';
  currrent_neigh_pos = 0;
  next = 0;
  count = 0;
  size = 0;
  count_n_received_from = 0;
  count_received = 0;
  page_count = 0;
  memset(neighbor_addr, 0 , sizeof (neighbor_addr));
  currrent_neigh_pos = 0;
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
  if(parent_is_null[0] == '1') {
    parent_is_null[0] = '0';
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
        LOG_INFO("Sending broadcast \n");
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

  printf("Data received on port %d  with length %d and value %s \n",
         receiver_port, datalen, data);
  LOG_INFO("Sender addr :");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO("\n");
  LOG_INFO("Receiver  addr :");
  LOG_INFO_6ADDR(receiver_addr);
  LOG_INFO("\n");

  int i = 0;
  while(i <= currrent_neigh_pos) {
    LOG_INFO("Neighbors of this node are :");
    LOG_INFO_6ADDR(&neighbor_addr[i]);
    LOG_INFO("\n");
    i++;
  }

  LOG_INFO("\n");
  LOG_INFO("And parent is :");
  LOG_INFO_6ADDR(&parent_addr);
  LOG_INFO("\n");

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
      retransmit_enable = '1';
    }


    if(data[0] == 'S') {
      retransmit_enable = '1';
      if(size == 0 && count == 0) {
        simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
        return;
      }
      unsigned const char * d = data +1 + sizeof(int);
      LOG_INFO("Value of data is : %s \n", d);
      //retransmit to every possible child

      memcpy(&sequence_num, data + 1 , sizeof(sequence_num));
      int amount = PAGESIZE / size;
      count_received++;
      //write offset of seq number to Pagebuffer
      snprintf(file + (sequence_num * size), size + 1 , "%s" , data + 1 + sizeof(sequence_num));
      // every sequence_num received
      if(count_received == amount) {
        retransmit_enable = '1';
        int amount;
        int sequence_num = 0;
        int dummy_fp = 0;
        for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
          char response[sizeof(int) + 1 + 20] = {"S"};
          memcpy(response + 1 ,&sequence_num, sizeof(sequence_num));
          snprintf(response + 1 + sizeof(sequence_num), size + 1, "%s", file + dummy_fp);
          int i = 0;
          while(i < currrent_neigh_pos) {

            simple_udp_sendto(&connection, response , sizeof(response) + 1, &neighbor_addr[i]);
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
            snprintf(SOFTWARE_ID, 9, "%s", ROM);
            snprintf(SOFTWARE_VERSION, 13, "%s", ROM + 8);
            LOG_INFO("SOFTWARE_ID now : %s and SOFTWARE_VERSION now :%s \n", SOFTWARE_ID,SOFTWARE_VERSION);
            if(currrent_neigh_pos == 0) {
              send_n = '0';
              reset();
              amount_page_received = 0;
              return;
            }
          }

          //request next page if no neighbors present
          if(currrent_neigh_pos == 0 && amount_page_received != count) {
            char response[1 + sizeof(int)] = {"N"};
            int page = amount_page_received + 1;
            memcpy(response + 1 ,&page, sizeof(page));
            simple_udp_sendto(&connection, response, sizeof(response), &parent_addr);
            send_n = '1';
          }


        }

    }
    //retransmit N to parent since we should have completed the page already
    if(data[0] == 'N') {
      if(count == amount_page_received) {
        return;
      }
      retransmit_enable = '0';
      int page;
      memcpy(&page, data + 1, sizeof(page));
      if(amount_page_received == count) {
        return;
      }
      int i;
      for(i = 0 ; i < currrent_neigh_pos ; i++) {
        if(uip_ip6addr_cmp(&(neighbor_addr[i]), sender_addr ) ) {
          received_n_from[i] = '1';

        }
        if(received_n_from[i] == '1') {
          count_n_received_from+= 1;
        }
      }
      LOG_INFO("Value count_n_received_from %d\n" , count_n_received_from);
      if(count_n_received_from == currrent_neigh_pos) {
          retransmit_enable = '1';
          memset(received_n_from, 0 , sizeof(received_n_from));
          count_n_received_from = 0;
          int page;
          memcpy(&page, data +1, sizeof(int));
          simple_udp_sendto(&connection, data, datalen, &parent_addr);
          send_n = '1';
          count_n_received_from = 0;
          memset(received_n_from, 0 , sizeof(received_n_from));
      }
      count_n_received_from = 0;
    }


    if(data[0] == 'R'  && data[1] == 'T') {

      s = 0;
      for(s = 0; s < currrent_neigh_pos ; s++) {
        if(uip_ip6addr_cmp(&neighbor_addr[s] , sender_addr)) {
          break;
        }
      }
      memcpy(&page, data + 2, sizeof(page));
      if(page == amount_page_received) {
          rt_mode = '1';
      } else {
        LOG_INFO("Dont have this page yet \n");
        return;
      }
    }

    if(data[0] == 'R' && data[1] == 'E') {
      static int amount;

      static int  sequence_num;
      sequence_num = 0;
      static int dummy_fp;
      dummy_fp = 0;
      for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
        LOG_INFO("IN LOOP \n");
        char response[sizeof(int) + 1 + 10] = {"S"};
        memcpy(response + 1 ,&sequence_num, sizeof(sequence_num));
        snprintf(response + 1 + sizeof(sequence_num), size + 1, "%s", file + dummy_fp);
            simple_udp_sendto(&connection, response ,sizeof(response), &neighbor_addr[s]);


        sequence_num++;
        dummy_fp += size;

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
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    //printf("Sending broadcast\n");
    //uip_create_linklocal_allnodes_mcast(&addr);
    //simple_udp_sendto(&broadcast_connection, "Test", 4, &addr

    // Periodically log neighbor and parent list
    // int i = 0;
    // while(i < currrent_neigh_pos) {
    //   LOG_INFO("Neighbors of this node are :");
    //   LOG_INFO_6ADDR(&neighbor_addr[i]);
    //   LOG_INFO("\n");
    //   i++;
    // }
    // LOG_INFO("Parent of this node is :");
    // LOG_INFO_6ADDR(&parent_addr);
    // LOG_INFO("\n");
    //LOG_INFO("SOFTWARE_VERSION is %s \n" , SOFTWARE_VERSION );
  //  LOG_INFO("SOFTWARE_ID is %s \n" , SOFTWARE_ID);
    // sending denyResponse if neighborlist is empty after specific time period

    if(rt_mode == '1') {
      LOG_INFO("IN RT \n");

      send_n = '0';

      LOG_INFO(" page %d amount_received %d \n", page, amount_page_received);
      if(page == (amount_page_received) && page != 0) {
        static int amount;

        static int  sequence_num;
        sequence_num = 0;
        static int dummy_fp;
        dummy_fp = 0;
        for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
          LOG_INFO("IN LOOP \n");
          char response[sizeof(int) + 1 + 10] = {"S"};
          memcpy(response + 1 ,&sequence_num, sizeof(sequence_num));
          snprintf(response + 1 + sizeof(sequence_num), size + 1, "%s", file + dummy_fp);
          simple_udp_sendto(&connection, response ,sizeof(response), &neighbor_addr[s]);

          sequence_num++;
          dummy_fp += size;

        }
        memset(rt_adress, 0, sizeof(rt_adress));
        current_rt = 0;
        rt_mode = '0';


        current_rt++;

      }
    }
    if(send_n == '1') {
      if(amount_page_received == count) {
        send_n = '0';

      } else {
      etimer_set(&send_timer, SEND_TIME);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
      char response[1 + sizeof(int)] = {"N"};
      int page = amount_page_received + 1;
      memcpy(response + 1 ,&page, sizeof(page));
      simple_udp_sendto(&connection, response, sizeof(response), &parent_addr);
      if(j % 5 == 0) {
        send_n = 0;

      }
    }
    }


    if(j % 10 == 0 && mode == '1') {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      etimer_set(&send_timer, SEND_TIME);

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
      if((currrent_neigh_pos) == 0) {

        if(required[0] == '1') {
            simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
        }
        else {
          int i = 0;
          for(i = 0 ; i < 3 ; i++) {
              simple_udp_sendto(&connection, "R0", sizeof("R0"), &parent_addr);
          }
                      reset();

        }
      }
    }
    if(j % 2 == 0 && retransmit_enable == '1') {
      count_received = 0;
      memset(file, 0 , sizeof(file));
      int pg = amount_page_received + 1;
      char retransmit_request[2 + sizeof(int)] = {"RT"};
      memcpy(retransmit_request + 2 ,&pg, sizeof(pg));
      LOG_INFO("Sending rt with page %d \n", pg);
      simple_udp_sendto(&connection, retransmit_request, sizeof(retransmit_request), &parent_addr);
    }

    j++;
    }
  PROCESS_END();
}




/*---------------------------------------------------------------------------*/
