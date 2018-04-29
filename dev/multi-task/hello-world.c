/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "random.h"
#include <stdio.h> /* For printf() */
#include "sys/node-id.h"
#include "sys/energest.h"

#define SENDSOR_TIME_UNIT			(2 * CLOCK_SECOND)

#define LOG_INTERVAL					5	

uint8_t last_on = ENERGEST_TYPE_LISTEN +1;

uint32_t sensor_1,sensor_2,sensor_3,sensor_4,sensor_5,sensor_6;
uint16_t counter = 0, reed = 0;
uint8_t log_num = 0;


void sennsor_schedule(energest_type_t type, uint8_t action);
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();

  static uint16_t i, j = 0;

  static struct etimer period_timer;

  etimer_set(&period_timer, SENDSOR_TIME_UNIT);

  for(i = 1; i < 7; i++){
			sennsor_schedule(ENERGEST_TYPE_LISTEN + i, 1);
	}

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER) {
      if(data == &period_timer) {
      	counter++;
      	reed = counter % node_id;

	  		j++;
	  		if(j > 6){
	  			j = 0;
	  		}

	  		for(i = 1; i < 7; i++){
					if(i != j)
						sennsor_schedule(ENERGEST_TYPE_LISTEN + i, 1);
					else{
		  			sennsor_schedule(ENERGEST_TYPE_LISTEN + i, 0);
					}
	  		}
      	
      	log_num++;
      	if(log_num > LOG_INTERVAL){
      		energest_flush();
	      	sensor_1 = energest_type_time(MY_SENSOR_1);
	      	sensor_2 = energest_type_time(MY_SENSOR_2);
	      	sensor_3 = energest_type_time(MY_SENSOR_3);
	      	sensor_4 = energest_type_time(MY_SENSOR_4);
	      	sensor_5 = energest_type_time(MY_SENSOR_5);
	      	sensor_6 = energest_type_time(MY_SENSOR_6);

	      	printf("!!!e %ld %ld %ld %ld %ld %ld\n",sensor_1, sensor_2, sensor_3, sensor_4, sensor_5, sensor_6 );

	      	log_num = 0;
      	}
      	etimer_reset_with_new_interval(&period_timer, SENDSOR_TIME_UNIT + random_rand() % SENDSOR_TIME_UNIT + reed);
      }
		}
	}
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void sennsor_schedule(energest_type_t type, uint8_t action)
{
	if(action){
		ENERGEST_ON(type);
	} else{
		ENERGEST_OFF(type);
	}
}
