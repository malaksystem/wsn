/*
 * Copyright (c) 2017, RISE SICS.
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

/**
 * \file
 *         NullNet broadcast example
 * \author
*         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include "random.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#endif /* MAC_CONF_WITH_TSCH */



#define DATA_SIZE             (128)
#define CH_P		              (0.05)
#define ROUND_TIME            (30* CLOCK_SECOND)
#define SEND_TIME            (5* CLOCK_SECOND)


#define ADVERTISE_HEADER       (0x77)
#define HEADER_LIST_NUM       8



static bool is_headed = false;

static linkaddr_t header_addr;


typedef struct header_info
{
  linkaddr_t addr;
  int rssi;
}header_info;

typedef struct header_list
{
  bool have_header;
  uint8_t ptr;
  header_info headers[HEADER_LIST_NUM];
}header_list_type;

header_list_type h_list;
header_info current_header;


void header_list_add(header_info info);
header_info header_select(void);
void header_advertise(void);
void send_data(linkaddr_t *dest_addr);
bool header_decide(uint16_t round)
void send_next_header(linkaddr_t *addr);

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{

  if(*data == ADVERTISE_HEADER){
    header_info info;
    info.addr = *dest;
    info.rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    header_list_add(info);
    header_select();
  } else {
    LOG_INFO("Received data from ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer, send_timer;
  static uint16_t r = 0;

  static bool is_header = false;

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */


  h_list.ptr = 0;
  h_list.have_header = false;

  /* Initialize NullNet */
  
  nullnet_set_input_callback(input_callback);

  etimer_set(&periodic_timer, ROUND_TIME);
  etimer_set(&send_timer, SEND_TIME);
  while(1) {

    PROCESS_WAIT_EVENT();

    if(data == &period_timer) {
      is_header = header_decide();
      if(is_header) {
        printf("I'm a header now\n");
        header_advertise();
      }
      etimer_reset(&periodic_timer);
    } else if (data == &send_timer){
      if(is_header){
        send_next_header(&current_header.addr);
      } else {
        if(h_list.have_header){
          send_data();
        }else {
          printf("don't have header\n");
        }
      }
      etimer_reset(&send_timer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
bool header_decide(uint16_t round)
{
  float T, rand;
  if(round % (1 / CH_P) == 0){
    is_headed = false;
  }
  if(is_headed){
    return false;
  } else {
    T = CH_P/(1.0-CH_P*(round % (1.0 /CH_P)));
    rand = (float)random_rand() / (float)RANDOM_RAND_MAX;
    if(rand < T){
      is_headed = true;
      return true;
    } else {
      return false;
    }
  }
}


void header_advertise(void)
{
  uint8_t data = ADVERTISE_HEADER;

  nullnet_buf = (uint8_t *)&data;
  nullnet_len = sizeof(data);


  memcpy(nullnet_buf, &data, sizeof(data));
  nullnet_len = sizeof(data);

  NETSTACK_NETWORK.output(NULL);
}

static uint8_t data_buf[DATA_SIZE] = {0XAA};
void send_data(linkaddr_t *dest_addr)
{
  memcpy(nullnet_buf, data_buf, sizeof(data_buf));
  NETSTACK_NETWORK.output(dest_addr);
}


void header_list_add(header_info info)
{
  h_list.have_header = true;
  if(++h_list.ptr >= HEADER_LIST_NUM)
  {
    h_list.ptr = 0;
  } 
  h_list.headers[h_list.ptr] = info;
}

header_info header_select(void)
{
  int max_rssi = h_list.headers[0].rssi, max_ptr = 0;
  for(uint8_t i = 1; i < HEADER_LIST_NUM; i++)
  {
    if(h_list.headers[i].rssi > max_rssi){
      max_ptr = i;
    }
  }
  return h_list.headers[max_ptr];
}

void send_next_header(linkaddr_t *addr)
{

}