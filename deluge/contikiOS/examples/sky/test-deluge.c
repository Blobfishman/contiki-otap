#include "contiki.h"
#include "cfs/cfs.h"
#include "deluge.h"
#include "sys/node-id.h"


#include "net/rpl/rpl.h"
#include "simple-udp.h"


#include <stdio.h>
#include <string.h>

#ifndef SINK_ID
#define SINK_ID	1
#endif

#define UDP_PORT 1234

#define SEND_INTERVAL		(10 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

#ifndef FILE_SIZE
#define FILE_SIZE 500
#endif

//Message that is casted before the update
static char* update = "Temperature is 25 degrees.";

PROCESS(example_unicast_process, "Example unicast");
PROCESS(deluge_test_process, "Deluge test process");
AUTOSTART_PROCESSES(&deluge_test_process,&example_unicast_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(deluge_test_process, ev, data)
{
  char buf[500];
  static int xy = 0;

  //different Buffers for testing

  //char buf2[5] = "abcd";
  //char buf2[100] = "YzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lB";
  //char buf2[200] ="YzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lB";
  //char buf2[300] = "YzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lB";
  char buf2[500]="YzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lBYzJBHr5fFSog5u7SQnc1t9vAkwOJf3RagDmalsYHpdXd2mWJ4gxBGCwFDVTpGg7BqszZQ9Nx9hU6QHhBy8BxSjmR6iANbHkx8lB";
  
  int fd, r;
  static struct etimer et;

  PROCESS_BEGIN();

  memset(buf, 0, sizeof(buf));
  if(node_id == SINK_ID) {
    strcpy(buf,buf2);
  } 
  else {
    //Copy a different string to the Buffer
    strcpy(buf, "This is version 0 of the file");
  }


  //use file system to safe the data
  cfs_remove("test");
  fd = cfs_open("test", CFS_WRITE);
  if(fd < 0) {
    process_exit(NULL);
  }
  if(cfs_write(fd, buf, sizeof(buf)) != sizeof(buf)) {
    cfs_close(fd);
    process_exit(NULL);
  }

  if(cfs_seek(fd, FILE_SIZE, CFS_SEEK_SET) != FILE_SIZE) {
    printf("failed to seek to the end\n");
  }

  deluge_disseminate("test", node_id == SINK_ID);
  cfs_close(fd);

  //check each second if the update is available
  etimer_set(&et, CLOCK_SECOND );
  for(;;) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if(node_id != SINK_ID) {
      fd = cfs_open("test", CFS_READ);
      if(fd < 0) {
        printf("failed to open the test file\n");
      } 
      else {
        r = cfs_read(fd, buf, sizeof(buf));
	      buf[sizeof(buf) - 1] = '\0';
	      if(r <= 0) {
	        printf("failed to read data from the file\n");
	      } 
        else {
          //update the update message
	        if((strcmp(buf,buf2) == 0)  && (xy == 0)) {
            printf("Update successfully received \n");
            xy = 1;
            update = "Rain probability is 25%";
          }
          
	      }
	      cfs_close(fd);
      }
    }
    etimer_reset(&et);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
//Application that casts a message 
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  char * msg = packetbuf_dataptr();
  printf("received from %d.%d: %s\n",
	 from->u8[0], from->u8[1], msg);
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/

//process to send a message each ten seconds
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
    
  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
    static struct etimer et;
    linkaddr_t addr;
    
    
    etimer_set(&et, CLOCK_SECOND * 10);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom(update, strlen(update));
    addr.u8[0] = 1;
    addr.u8[1] = 0;
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      unicast_send(&uc, &addr);
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
