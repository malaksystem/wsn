/*
 * Copyright (c) 201, RISE SICS
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
#include "rpl.h"
#include "rpl-dag-root.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "net/link-stats.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/tcp-socket.h"



#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ROOT"
#define LOG_LEVEL LOG_LEVEL_INFO


#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;

static FILE * fp;

/* Declare and auto-start this file's process */
PROCESS(contiki_ng_br, "Contiki-NG Border Router");
AUTOSTART_PROCESSES(&contiki_ng_br);
/*---------------------------------------------------------------------------*/
static void save_data(const uip_ipaddr_t *ip_addr, uint16_t *data, uint16_t size)
{
	uint16_t i;

  time_t receive_time;

  receive_time = time(NULL);

  fprintf(fp, "%ld,", receive_time);
	

  fprintf(fp, "%d,", *(uint16_t *)data);           //seqno
  fprintf(fp, "%d,", *((uint16_t *)data+1));       //type

  //LOG_INFO_("sensor data: ");
  for(i = 0;i < 8;i++)                            //ip
  {
    uint16_t tmp;
    tmp = ((uint16_t)ip_addr->u8[2*i] << 8) + ip_addr->u8[2 * i + 1];
    fprintf(fp, "%d,", tmp);
    //LOG_INFO_("%d,", tmp); 
  }

	for(i = 2;i < size / 2;i++){                   //data
		fprintf(fp, "%d,", *(data+i));
		//LOG_INFO_("%d,", *(data+i)); 
	}
	fprintf(fp, "\n");
	//LOG_INFO_("\n");
  fflush(fp);
}
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
  LOG_INFO("Received %u data, data type: %d data sequ: %d form: ", datalen,  *(data+1), *data);
  LOG_INFO_6ADDR(sender_addr);
  printf("\n");
  
	save_data(sender_addr, (uint16_t *)data, datalen);   
}

static volatile int keepRunning = 1;
void intHandler(int dummy) {
    fclose(fp);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();

  signal(SIGINT, intHandler);

  fp = fopen("data.csv", "a+");

  if(fp == NULL)
  {
  	LOG_ERR("can not open file!");
  }

  /* Initialize UDP connection */


  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  LOG_INFO("Contiki-NG Border Router started\n");

  PROCESS_END();
}
