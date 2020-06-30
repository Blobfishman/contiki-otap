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



#define UDP_PORT 1234
#define UDP_PORT_N 2222
static struct simple_udp_connection broadcast_connection;
static struct simple_udp_connection connection;

//lists for storing adresses where data is sent to , maybe cut for real lists
#define MAX_NEIGHBOR 8
static  uip_ipaddr_t parent_addr ;
static  uip_ipaddr_t neighbor_addr[MAX_NEIGHBOR];
static int currrent_neigh_pos = 0;
static int deleted = 0;
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

static char SOFTWARE_ID[8] = {"1.001.08"};
static char SOFTWARE_VERSION[12] = {"1.001.000.11"};

static char mode = '0';
//mode 0 = no update , mode 1 = update
//static char mode = 0


/**
* @brief handles advertisement, stores parent  and boradcasts advertisement package to neighbors
* @param [in] @src datapackage we Reviced via udp
* @param [in] @send_addr sender of this package
* @return void
*/
void handleAdvertisementPackage(char* src, const uip_ipaddr_t send_addr ) {
    // check if sender is is child do nothing if it is
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
      uip_ipaddr_t  dummy;
      uip_ip6addr_copy(&dummy,&send_addr);
      uip_ip6addr_copy(&parent_addr,&dummy);
      LOG_INFO("Sending IP packet with value %s \n", buffer);
      LOG_INFO("Sender addr  was :");
      LOG_INFO_6ADDR(&dummy);
      LOG_INFO("\n");
      simple_udp_sendto(&connection, "IP", 2, &dummy);
    }
    // if not null check if parent needs update
    else {
      if(parent_required == '0' && update_required[0] == '1') {
        uip_ipaddr_t dummy;
        uip_ip6addr_copy(&dummy,&send_addr);
        uip_ip6addr_copy(&parent_addr,&dummy);
        //LOG_INFO("In Check Sending IP packet with value %s \n", buffer);
        //LOG_INFO("Sender addr  was :")
        //LOG_INFO_6ADDR(&dummy);
        //LOG_INFO("\n")
        simple_udp_sendto(&connection, "IP", 2, &dummy);
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
    simple_udp_sendto(&broadcast_connection, buffer, sizeof buffer, &addr);
    memset(buffer,0, sizeof buffer);
    mode = '1';
  }

/*---------------------------------------------------------------------------*/





/*---------------------------------------------------------------------------*/
PROCESS(broadcast_example_process, "UDP broadcast example process");
AUTOSTART_PROCESSES(&broadcast_example_process);
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
      while(i <= MAX_NEIGHBOR) {
        LOG_INFO("Checking if sender is chil \n");
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
    handleAdvertisementPackage(dst,*sender_addr);
  }

  // denyResponse -> deletee neighbor from list
  // TODO adjust so that after deleting another neighbor can be stored, maybe use a list?
  if(data[0] == 'R' && data[1] == '0') {
    int i = 0;
    //delete adress
    while(i <= MAX_NEIGHBOR) {
      if(uip_ip6addr_cmp(&neighbor_addr[i],&(*sender_addr)) ) {
        LOG_INFO("Found the adress to delete");
          memset(&neighbor_addr[i], 0, sizeof(uip_ipaddr_t) );
          deleted++;
      }
    i++;
    }
    //check if neighborlist is empty now
    if((currrent_neigh_pos - deleted ) != 0) {
      LOG_INFO("Has neighbors");
      return;
    }
    // if empty send response depending on whtether update is needed or not
    if(required[0] == '1') {
        simple_udp_sendto(&connection, "R1", 2, &parent_addr);
        return;
    } else {
        simple_udp_sendto(&connection, "R0", 2, &parent_addr);
      }
    }

    // requestResponse -> retransmit request to parent
    if(data[0] == 'R' && data[1] == '1') {
      LOG_INFO("Reviced R1 \n");
      simple_udp_sendto(&connection, "R1", 2, &parent_addr);
    }

    //update_package -> if needed apply if not retransmit to neighbors
    if(data[0] == 'U') {
      if(required[0] == '1') {
        LOG_INFO("Applying update  with data :%s  \n", data);
      }
      //if has neighbors send to them
      if(currrent_neigh_pos - deleted != 0) {
        LOG_INFO("Sending update to neighbors \n");
        int i = 0;
        while(i <= currrent_neigh_pos) {
          simple_udp_sendto(&connection, data, datalen, &neighbor_addr[i]);
          i++;
        }
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
  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);
 //specific ip-adress
 simple_udp_register(&connection, UDP_PORT_N,
                    NULL, UDP_PORT_N,
                    receiver);

  //timer for delay
  etimer_set(&periodic_timer, SEND_INTERVAL);
    static int j = 0;
    while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    //printf("Sending broadcast\n");
    //uip_create_linklocal_allnodes_mcast(&addr);
    //simple_udp_sendto(&broadcast_connection, "Test", 4, &addr

    // Periodically log neighbor and parent list
    int i = 0;
    while(i <= currrent_neigh_pos) {
      LOG_INFO("Neighbors of this node are :");
      LOG_INFO_6ADDR(&neighbor_addr[i]);
      LOG_INFO("\n");
      i++;
    }
    LOG_INFO("Parent of this node is :");
    LOG_INFO_6ADDR(&parent_addr);
    LOG_INFO("\n");
    // sending denyResponse if neighborlist is empty after specific time period
    if(j % 10 == 0 && mode == '1') {
      if((currrent_neigh_pos - deleted) == 0) {
        LOG_INFO("Sending RESPONSE \n");
        simple_udp_sendto(&connection, "R0", 2, &parent_addr);
      }
    }
    j++;
    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
