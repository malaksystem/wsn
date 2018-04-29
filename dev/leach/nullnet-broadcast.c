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
#include "net/packetbuf.h"
#include <string.h>
#include "random.h"
#include <stdbool.h>
#include <stdio.h> /* For printf() */
#include "dev/leds.h"
#include "sys/energest.h"

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


#define DATA_SIZE             (32)
#define CH_P                  (0.2)
#define ROUND_TIME            (200* CLOCK_SECOND)
#define SEND_TIME             (10* CLOCK_SECOND)
#define HEADER_SEND_TIMES      5

#define NETWORK_BUILD_TIME    (10* CLOCK_SECOND)

#define ADVERTISE_ADD_HEADER      (0x77)
#define ADVERTISE_RM_HEADER       (0x5B)
#define DATA_INFO                 (0xAA)
#define HEADER_LIST_NUM           8


#define TOTAL_ENERGY              2000000 




static bool pre_role = false;
static bool is_headed = false;


typedef struct header_info
{
  linkaddr_t addr;
  int rssi;
  bool available;
}header_info;

typedef struct header_list
{
  uint8_t head_num;
  uint8_t ptr;
  header_info headers[HEADER_LIST_NUM];
}header_list_type;

header_list_type h_list;
header_info *last_header;
header_info *current_header;

header_info *last_next_header;
header_info *next_header;

uint32_t receive_pkt_num = 0;

uint32_t total_send_num = 0;

uint64_t cpu, lpm, transmit, listen, sensor;
float energy_cost = 0;


void header_list_add(header_info info);
void header_advertise(uint8_t type);
void send_data(linkaddr_t *dest_addr);
bool header_decide(uint16_t round);
header_info * next_header_select(void);
header_info *header_select(void);
void header_list_rm(header_info info);


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  header_info info;
  if(*(uint8_t *)data == ADVERTISE_ADD_HEADER){
    info.addr = *src;
    info.rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    info.available = true;
    printf("receive HA from ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    header_list_add(info);
  } else if(*(uint8_t *)data == ADVERTISE_RM_HEADER){
    info.addr = *src;
    printf("receive HR from ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    header_list_rm(info);
  }else {
    receive_pkt_num++;
    LOG_INFO("Received number %ld pkt from ", receive_pkt_num);
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer, send_timer;
  static uint16_t r = 1;
  static uint16_t i;
  static uint8_t send_times = 0;

  static bool is_header = false;

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */


  h_list.ptr = 0;
  h_list.head_num = 0;

  LOG_INFO("Link-layer address ");
  LOG_INFO_LLADDR(&linkaddr_node_addr);
  LOG_INFO_("\n");

  /* Initialize NullNet */
  
  nullnet_set_input_callback(input_callback);

  etimer_set(&periodic_timer, CLOCK_SECOND);
  etimer_set(&send_timer, SEND_TIME);
  while(1) {

    PROCESS_WAIT_EVENT();

    if(data == &periodic_timer) {
      cpu = energest_type_time(ENERGEST_TYPE_CPU);
      lpm = energest_type_time(ENERGEST_TYPE_LPM);
      transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      listen = energest_type_time(ENERGEST_TYPE_LISTEN); 

      energy_cost = (transmit + listen) * 0.02 + cpu * 0.002;
      printf("total energy cost is %ld \n", (int32_t)energy_cost);
      if(energy_cost > TOTAL_ENERGY && linkaddr_node_addr.u8[0] != 0x01){
        NETSTACK_MAC.off();
        etimer_stop(&periodic_timer);
        etimer_stop(&send_timer);
        printf("I'm die!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      }

      is_header = header_decide(r++);

      if(is_header) {
        printf("I'm a header now\n");
        header_advertise(ADVERTISE_ADD_HEADER);
        leds_on(LEDS_GREEN);

      } else if(pre_role && !is_header){
        header_advertise(ADVERTISE_RM_HEADER);
        printf("I became a member now\n");
        leds_off(LEDS_GREEN);
      }else{
        printf("I'm a member\n");
        leds_off(LEDS_GREEN);
      }

      pre_role = is_header;
      etimer_reset_with_new_interval(&periodic_timer, ROUND_TIME);
      //etimer_reset_with_new_interval(&send_timer, NETWORK_BUILD_TIME);
    } else if (data == &send_timer){
      if(is_header){

      	if(linkaddr_node_addr.u8[0] == 0x01){
	        printf("!!!p %ld\n", (int32_t)receive_pkt_num);
	      }
        if(++send_times > HEADER_SEND_TIMES){
          send_times = 0;
          next_header = next_header_select();
          
          if(next_header != NULL){
            for(i = 0; i < receive_pkt_num + HEADER_SEND_TIMES; i++){ //add data produce by itself
              send_data(&next_header->addr);
              PROCESS_PAUSE();
            }
            LOG_ANNOTATE("#L %u 0;red\n", last_next_header->addr.u8[0]);
            LOG_ANNOTATE("#L %u 1;red\n", next_header->addr.u8[0]);

            last_next_header = next_header;
            printf("send to next header ");
            LOG_INFO_LLADDR(&next_header->addr);
            printf("\n");
            receive_pkt_num = 0;
          } else {
            printf("waiting next header ready\n");
          }
        }
      } else {
 
  		printf("!!!e %ld\n", (int32_t)energy_cost);
    	printf("!!!p %ld\n", (int32_t)total_send_num);

      	

        current_header = header_select();


        if(current_header != NULL){
          if(current_header->addr.u8[0] != last_header->addr.u8[0]){
            LOG_ANNOTATE("#L %u 0;blue\n", last_header->addr.u8[0]);
          }
          LOG_ANNOTATE("#L %u 1;blue\n", current_header->addr.u8[0]);
          last_header = current_header;

          send_data(&current_header->addr);
          printf("send number %ld data to ", total_send_num);
          LOG_INFO_LLADDR(&current_header->addr);
          printf("\n");
        }else {
          printf("don't have header\n");
        }
      }
      etimer_reset_with_new_interval(&send_timer, SEND_TIME + random_rand() % SEND_TIME);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
bool header_decide(uint16_t round)
{
  float T, rand;
  static int last_as_header = -1 * 1/CH_P;

  if(linkaddr_node_addr.u8[0] == 0x01){
    return true;
  }else{
    if(round - last_as_header < (1/CH_P)){
      return false;
    } else {
      T = CH_P/(1.0-CH_P * (round % (uint16_t)(1.0 /CH_P)));
      rand = (float)random_rand() / (float)RANDOM_RAND_MAX;
      if(rand < T){
        is_headed = true;
        last_as_header = round;
        return true;
      } else {
        is_headed = false;
        return false;
      }
    }
  }
}

void header_advertise(uint8_t type)
{
  uint8_t data = type;

  nullnet_buf = (uint8_t *)&data;
  nullnet_len = sizeof(data);

  NETSTACK_NETWORK.output(NULL);
}

static uint8_t data_buf[DATA_SIZE] = {DATA_INFO};
void send_data(linkaddr_t *dest_addr)
{
  total_send_num++;
  nullnet_buf = data_buf;
  NETSTACK_NETWORK.output(dest_addr);
}


void header_list_add(header_info info)
{
  uint8_t i;
  bool has_free = false;
  if(h_list.head_num < HEADER_LIST_NUM){
    h_list.head_num++;
  }

  for(i = 0; i < h_list.head_num; i++){
    if(!h_list.headers[i].available){
      
      h_list.headers[i] = info;

      has_free = true;
    }
  }
  if(!has_free){
    h_list.headers[h_list.ptr] = info;
    if(++h_list.ptr >= HEADER_LIST_NUM){
        h_list.ptr = 0;
      }
  }

  printf("add ");
  LOG_INFO_LLADDR(&info.addr);
  printf(" to header list, rssi: %d\n", info.rssi);

/*  printf("******************list************************: \n");

  for(i = 0; i < h_list.head_num; i++){
    if(h_list.headers[i].available){
      printf("ip: ");
      LOG_INFO_LLADDR(&h_list.headers[i].addr);
      printf(" rssi: %d\n", h_list.headers[i].rssi);
    }
  }
  printf("******************list************************: \n");*/
}

void header_list_rm(header_info info)
{
  uint8_t i;
  for(i = 0; i < h_list.head_num; i++){
    if(h_list.headers[i].addr.u8[0] == info.addr.u8[0]){
      h_list.headers[i].available = false;
      printf("remove node : ");
      LOG_INFO_LLADDR(&h_list.headers[i].addr);
      printf(" from list\n");
    }
  }
}

header_info *header_select(void)
{
  uint8_t i;
  int max_rssi = -100, max_ptr = -1;

  for(i = 0; i < h_list.head_num; i++){
    if(h_list.headers[i].rssi > max_rssi && h_list.headers[i].available){
      max_rssi = h_list.headers[i].rssi;
      max_ptr = i;
    }
  }
  if(max_ptr >= 0){
    printf("choose ");
    LOG_INFO_LLADDR(&h_list.headers[max_ptr].addr);
    printf(" as header, rssi: %d\n", h_list.headers[max_ptr].rssi);
    return &h_list.headers[max_ptr];
  } else{
    return NULL;
  }
}

header_info * next_header_select(void)
{
  uint8_t i;

  if(linkaddr_node_addr.u8[0] == 0x01){
    return NULL;
  } else{
    uint8_t min_addr_h = 255;
    int8_t min_ptr = -1;

    for(i = 0; i < h_list.head_num; i++)
    {
      if(h_list.headers[i].addr.u8[0] < min_addr_h  && h_list.headers[i].available){
        min_addr_h = h_list.headers[i].addr.u8[0];
        min_ptr = i;
      }
    }
    if(min_ptr >= 0){
      printf("choose ");
      LOG_INFO_LLADDR(&h_list.headers[min_ptr].addr);
      printf(" as next header\n");
      return &h_list.headers[min_ptr];
    } else {
      return NULL;
    }
  }
}
