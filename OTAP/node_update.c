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
static  uip_ipaddr_t parent_addr ;
static uip_ipaddr_t rt_adress[MAX_NEIGHBOR];
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
static int amount_page_received = 0;
static char retransmit_enable = '0';
// timer to wait for events
#define SEND_INTERVAL		(100 * CLOCK_SECOND)
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
  parent_is_null[0] = '1';
  parent_required = '0';
  mode = '0';
  required[0] = '0';
  currrent_neigh_pos = 0;
  next = 0;
  count = 0;
  size = 0;
  count_received = 0;
  page_count = 0;
  memset(neighbor_addr, 0 , sizeof (neighbor_addr));
  currrent_neigh_pos = 0;
  memset(file, 0 , sizeof file);
  fp = 0;
  memset(&parent_addr, 0 , sizeof parent_addr);

}
void fixArray(int pos) {
  int j;
  j = pos;
  for( j = pos ; j < currrent_neigh_pos; j++) {
    LOG_INFO("value of j is :  %d  \n", j);
    LOG_INFO("value of MAX_NEIGHBOR is :  %d  \n", currrent_neigh_pos);
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
void handleAdvertisementPackage(char* src, const uip_ipaddr_t send_addr, int * j  ) {
    // check if sender is is child do nothing if it is
    *j = 0;
    char buffer[30] = {0};
    LOG_INFO("Recived advertisement with value %s", src);
    int i = 0;

    while(i < MAX_NEIGHBOR) {
      if(uip_ip6addr_cmp(&neighbor_addr[i],&send_addr) ) {
		    LOG_INFO("Sender is child \n");
		      return;
      }
      i++;
    }
    LOG_INFO("Sender was not a child\n");

    char update_softwareid[9];
    char update_softwareversion[13];
    char update_required[1];
    //copy values from src  to check them
    snprintf(update_softwareid, sizeof(update_softwareid), "%s", src + 1);
    snprintf(update_softwareversion, 13, "%s", src + 9);
    update_required[0] = *(src + 21);

    LOG_INFO("Value software_id : %s  version : %s  required %s  \n" , update_softwareid, update_softwareversion, update_required);

    // store parent_addr if is null
    if(parent_is_null[0] == '1') {
      LOG_INFO("Sender was first one \n");
      parent_is_null[0] = '0';
      // parent needs update and was first one
      if(update_required [0] == '1') {
        LOG_INFO("Parent needs update \n");
        parent_required = '1';

      }
      //copy sender addr to dummy
      uip_ip6addr_copy(&parent_addr,&send_addr);
      LOG_INFO("Sender addr  was :");
      LOG_INFO_6ADDR(&send_addr);
      LOG_INFO("\n");
      LOG_INFO("Sending IP packet \n");
      simple_udp_sendto(&connection, "IP", sizeof("IP"), &parent_addr);
    }
    // if not null check if parent needs update
    else {
      if(parent_required == '0' && update_required[0] == '1') {
        uip_ip6addr_copy(&parent_addr,&send_addr);
        //LOG_INFO("In Check Sending IP packet with value %s \n", buffer);
        //LOG_INFO("Sender addr  was :")
        //LOG_INFO_6ADDR(&dummy);
        //LOG_INFO("\n")
        simple_udp_sendto(&connection, "IP", sizeof("IP"), &send_addr);
        return;
      }
      else {
        /* it was planned to send a deny package so that the parent can  remove THIS
        // child from its neighborlist but since we need to send the IP package we can
        // ignore this step
        */
        return;
      }

    }

     //check if update on node is needed,
     if(strncmp(SOFTWARE_ID,update_softwareid,8) == 0) {
       LOG_INFO("SOFTWARE_ID is same \n");
       if(strncmp(SOFTWARE_VERSION,update_softwareversion,12) != 0) {
          LOG_INFO("Update required \n");
          required[0] = '1';
	     }
     }
     else {
      LOG_INFO("update not needed \n");
      required[0] = '0';
     }
    //multicast  packet to neighbor adress will be stored when reciving IP packet
   //we manipulate buffer so that the process uses its value to send the data
    snprintf(buffer, 22, "%s" , src );
    *(buffer + 21) = required[0];
    LOG_INFO("Buffer value is %s \n" ,buffer);
    LOG_INFO("\n");
    uip_ipaddr_t addr;
    //multicast adress
    uip_create_linklocal_allnodes_mcast(&addr);
    LOG_INFO("BROADCASTING TO NEIGHBORS \n");
    simple_udp_sendto(&broadcast_connection, buffer, sizeof buffer, &addr);
    memset(buffer,0, sizeof buffer);
    mode = '1';
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

  // upong reciving IP package store neighbor in list if it fits
  //TODO adjust  array so that after deleting element another one can be stored
  if(data[0] == 'I' && data[1] == 'P') {
    if(currrent_neigh_pos < MAX_NEIGHBOR)
    {
      int i = 0;
      while(i < MAX_NEIGHBOR) {
        LOG_INFO("Checking if sender is chill \n");
        if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
          LOG_INFO("Sender is child");
            return;
        }
        i++;
      }
      uip_ip6addr_copy(&(neighbor_addr[currrent_neigh_pos]) ,sender_addr);
       LOG_INFO("Recived IP packet ADDR is :");
       LOG_INFO_6ADDR(&neighbor_addr[currrent_neigh_pos]);
       LOG_INFO("\n");
       currrent_neigh_pos++;

    }
  }

  // upon reciving advertisement call handler function
  if(data[0] == 'A') {
    char dst[100];
    strncpy(dst, (char*) data, datalen);
    handleAdvertisementPackage(dst,*sender_addr, &j);
  }

  // denyResponse -> deletee neighbor from list
  // TODO adjust so that after deleting another neighbor can be stored, maybe use a list?
  if(data[0] == 'R' && data[1] == '0') {
    int i = 0;
    //delete adress
    while(i < MAX_NEIGHBOR) {
      if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
        LOG_INFO("Found the adress to delete");
        fixArray(i);
        break;
      }
    i++;
    }
    //check if neighborlist is empty now
    if((currrent_neigh_pos) != 0) {
      LOG_INFO("Has neighbors \n");
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
      LOG_INFO("Reviced R1 \n");
      if(currrent_neigh_pos < MAX_NEIGHBOR)
      {
        int i = 0;
        while(i < MAX_NEIGHBOR) {
          LOG_INFO("Checking if sender is chill \n");
          if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
            simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
              return;
          }
          i++;
        }
        uip_ip6addr_copy(&(neighbor_addr[currrent_neigh_pos]) ,sender_addr);
         LOG_INFO("Recived IP packet ADDR is :");
         LOG_INFO_6ADDR(&neighbor_addr[currrent_neigh_pos]);
         LOG_INFO("\n");
         currrent_neigh_pos++;

      }
      if(size !=0 && count != 0) {
          unsigned char update[9] = {"U"};
          memcpy(update + sizeof(char), &size, sizeof(size));
          memcpy(update + sizeof(size) + sizeof(char), &count, sizeof(count));
          LOG_INFO(" value of dst is  %d and cnt  %d  \n" , size , count);
          simple_udp_sendto(&connection, update, sizeof(update), sender_addr);
      } else {
      simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
      }
    }

    //update_package -> if needed apply if not retransmit to neighbors
    if(data[0] == 'U') {
      mode = '0';
      memcpy(&size, data + 1 , sizeof(size));
      memcpy(&count, data + 1 + sizeof(count) , sizeof(size));
       LOG_INFO("Applying update  with size: %d  and count %d \n",size,count);
      //if has neighbors send to them
      if(currrent_neigh_pos != 0) {
        LOG_INFO("Sending update to neighbors \n");
        int i = 0;
        while(i < currrent_neigh_pos) {
          simple_udp_sendto(&connection, data, datalen, &neighbor_addr[i]);
          i++;
        }

      }
      retransmit_enable = '1';
    }


    if(data[0] == 'S') {
      retransmit_enable  = '1';
      //retransmit to every possible child
      LOG_INFO("S received \n");
      memcpy(&sequence_num, data + 1 , sizeof(sequence_num));
      int amount = PAGESIZE / size;
      count_received++;
      LOG_INFO(" Sequence num is %d \n" , sequence_num);

      //write offset of seq number to Pagebuffer
      snprintf(file + (sequence_num * size), size + 1 , "%s" , data + 1 + sizeof(sequence_num));
      // every sequence_num received
      if(count_received == amount) {
        if(currrent_neigh_pos == 0) {
          retransmit_enable = '1';
        } else {
            retransmit_enable = '0';
        }
        int amount;
        int sequence_num = 0;
        int dummy_fp = 0;
        for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
          char response[sizeof(int) + 1 + 20] = {"S"};
          memcpy(response + 1 ,&sequence_num, sizeof(sequence_num));
          snprintf(response + 1 + sizeof(sequence_num), size + 1, "%s", file + dummy_fp);
          int i = 0;
          while(i < currrent_neigh_pos) {
            LOG_INFO("Sending update to neighbors \n");
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
          memset(file, 0 , sizeof(file));
          LOG_INFO("Cleared file buffer %s \n", file);
          //request next page if no neighbors present
          if(currrent_neigh_pos == 0 && amount_page_received != count) {
            char response[1 + sizeof(int)] = {"N"};
            int page = amount_page_received + 1;
            memcpy(response + 1 ,&page, sizeof(page));
            simple_udp_sendto(&connection, response, sizeof(response), &parent_addr);
          }
          LOG_INFO("Value of ROM %s \n", ROM);
          if(count == amount_page_received) {
            snprintf(SOFTWARE_ID, 9, "%s", ROM);
            snprintf(SOFTWARE_VERSION, 13, "%s", ROM + 8);
            retransmit_enable = '0';
            reset();
            amount_page_received = 0;
            LOG_INFO("reset every variable hehe \n");
          }
        }

    }
    //retransmit N to parent since we should have completed the page already
    if(data[0] == 'N') {
      simple_udp_sendto(&connection, data, datalen, &parent_addr);
    }


    if(data[0] == 'R'  && data[1] == 'T') {
      rt_mode = 1;
      uip_ip6addr_copy(&rt_adress[current_rt] ,sender_addr);
      current_rt++;
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
    LOG_INFO("SOFTWARE_VERSION is %s \n" , SOFTWARE_VERSION );
    LOG_INFO("SOFTWARE_ID is %s \n" , SOFTWARE_ID);
    // sending denyResponse if neighborlist is empty after specific time period
    if(rt_mode == 1) {
      LOG_INFO("IN RT \n");
      static int amount;
      static int sequence_num;
      sequence_num = 0;

      int page = 1;
      static int dummy_rom_p;
      dummy_rom_p = (page - 1) * PAGESIZE;
      for(amount = PAGESIZE / size ; amount > 0 ; amount--) {
        LOG_INFO("value of dummy_rom_p %d and size %d \n", dummy_rom_p, size);
        static char response[sizeof(int) + 1 + 20] = {"S"};
        memcpy(response + 1 ,&sequence_num, sizeof(sequence_num));
        snprintf(response + 1 + sizeof(sequence_num), size + 1, "%s", ROM + dummy_rom_p);
        LOG_INFO("response is %s \n", response);
        etimer_set(&send_timer, SEND_TIME);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
        int i = 0;
        while(i < current_rt) {
            simple_udp_sendto(&connection, response ,sizeof(response), &rt_adress[i]);
        }

        sequence_num++;
        dummy_rom_p += size;

      }
      memset(rt_adress, 0, sizeof(rt_adress));
      current_rt = 0;
    }

    if(j % 100 == 0 && mode == '1') {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      etimer_set(&send_timer, SEND_TIME);

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
      if((currrent_neigh_pos) == 0) {
        LOG_INFO("Sending RESPONSE \n");
        if(required[0] == '1') {
            simple_udp_sendto(&connection, "R1", sizeof("R1"), &parent_addr);
        }
        else {
          simple_udp_sendto(&connection, "R0", sizeof("R0"), &parent_addr);
        }
      }
    }
    LOG_INFO("Value of retransmit_enable %c \n", retransmit_enable);
    if(j % 20 == 0 && retransmit_enable == '1') {
      LOG_INFO("IM in retransmit_enable \n");
      count_received = 0;
      int page = amount_page_received + 1;
      char retransmit_request[2 + sizeof(int)] = {"RT"};
      memcpy(retransmit_request + 2 ,&page, sizeof(page));
      simple_udp_sendto(&connection, retransmit_request, sizeof(retransmit_request), &parent_addr);
    }

    j++;
    }
  PROCESS_END();
}




/*---------------------------------------------------------------------------*/
