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
#include "net/netstack.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "msg_send.h"
#include "button-sensor.h"
#include "board-peripherals.h"

#include <stdio.h> /* For LOG_INFO() */
#include <string.h>
#include <ctype.h>

#include "collect-sensor.h"
#include "sys/log.h"

#include "cfs/cfs.h"


#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678


#define DEFAULT_RSSI_VALUE      (-3)

#ifndef PERIOD
#define PERIOD 10
#endif
#define RANDWAIT (PERIOD)

#define BUZZER_TIME                 1

#define SENSOR_READ_INTERVAL       (CLOCK_SECOND * 5)


#define TOPOLOGY_THAN_SENSOR  (5 - 1)

extern sensor_data_type sensor_data;

static struct simple_udp_connection udp_conn;


static void init_tx_power(void);
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
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
   /* Ignore incoming data */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer period_timer, read_timer, buzzer_timer;
  static uint8_t topology_wait = 0;
  static uint8_t radio_state = 1, buzzer_time = 0;
  int rf_state;


  PROCESS_BEGIN();

  init_tx_power();

  open_data_recording();

  init_sensors();

   /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&period_timer, CLOCK_SECOND * PERIOD);
  etimer_set(&read_timer, SENSOR_READ_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == sensors_event) {
      if(data == &button_left_sensor){
        if(radio_state){ 
          NETSTACK_MAC.off();

          NETSTACK_RADIO.get_value(RADIO_PARAM_POWER_MODE, &rf_state);
          LOG_INFO("radio state %d\n", rf_state);

          etimer_stop(&read_timer);
          etimer_stop(&period_timer);
          leds_off(LEDS_RED);
          leds_off(LEDS_GREEN);
          buzzer_start(2000);
          etimer_set(&buzzer_timer, CLOCK_SECOND * BUZZER_TIME);
          LOG_INFO("radio off\n");
          radio_state = 0;
        }
        else{
          NETSTACK_MAC.on();

          NETSTACK_RADIO.get_value(RADIO_PARAM_POWER_MODE, &rf_state);
          LOG_INFO("radio state %d\n", rf_state);

          etimer_set(&read_timer, SENSOR_READ_INTERVAL);
          etimer_set(&period_timer, CLOCK_SECOND * PERIOD);
          buzzer_start(1000);
          etimer_set(&buzzer_timer, CLOCK_SECOND * BUZZER_TIME / 2);
          LOG_INFO("radio on\n");
          radio_state = 1;
        }
      } else if(data == &button_right_sensor) {
        watchdog_reboot();
      }
    } else if(ev == PROCESS_EVENT_TIMER) {
      if(data == &period_timer) {
        /* Time to send the data */
        if(NETSTACK_ROUTING.node_is_reachable()){
          collect_common_set_send_active(1);
          leds_on(LEDS_GREEN);
        } else{
          leds_off(LEDS_GREEN);
          collect_common_set_send_active(0);
          LOG_INFO("not reachable yet\n"); 
        }
        collect_common_send(&udp_conn);
        topology_wait++;
        if(topology_wait > TOPOLOGY_THAN_SENSOR){
          topology_send(&udp_conn);
          topology_wait = 0;
        }
        etimer_reset_with_new_interval(&period_timer, CLOCK_SECOND * PERIOD + random_rand() % (CLOCK_SECOND * RANDWAIT));
      } else if(data == &read_timer) {
        leds_toggle(LEDS_RED);
        read_sensors(&sensor_data);
        active_sensors();
        leds_toggle(LEDS_RED);
        LOG_INFO("reading sensors\n");
        etimer_reset(&read_timer);
      } else if(data == &buzzer_timer){
        
        if(buzzer_time == 0 && radio_state) {
          buzzer_time++;
          buzzer_stop();
          etimer_set(&buzzer_timer, CLOCK_SECOND * BUZZER_TIME / 2);
        } else if(buzzer_time == 1 && radio_state) {
          buzzer_time++;
          buzzer_start(1000);
          etimer_set(&buzzer_timer, CLOCK_SECOND * BUZZER_TIME / 2);
        } else {
          buzzer_time = 0;
          buzzer_stop();
        }
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void init_tx_power(void)
{
  int fd;

  int power = DEFAULT_RSSI_VALUE;

  fd = cfs_open("txpower", CFS_READ);
  if (fd < 0) {
    fd = cfs_open("txpower", CFS_READ | CFS_WRITE);
    if (fd < 0){
      printf("write txpower file error!\n");
    } else {
      cfs_seek(fd, 0, CFS_SEEK_SET);
      cfs_write(fd, &power, sizeof(power));
    }
  } else {
    cfs_close(fd);
    fd = cfs_open("txpower", CFS_READ | CFS_WRITE | CFS_APPEND);
    cfs_seek(fd, 0, CFS_SEEK_SET);
    cfs_read(fd, &power, sizeof(power));
    cfs_seek(fd, 0, CFS_SEEK_SET);
    cfs_write(fd, &power, sizeof(power));
  }

  printf("set power %d\n", power);

  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, power);

  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &power);

  printf("power set to %d\n", power);

  cfs_close(fd);
}

int8_t 
write_txpower_file(int value)
{
  int fd;

  fd = cfs_open("txpower", CFS_READ | CFS_WRITE | CFS_APPEND);
  if(fd < 0) {
    printf("open txpower file error!\n");
    return -1;
  } else {
    cfs_seek(fd, 0, CFS_SEEK_SET);
    cfs_write(fd, &value, sizeof(value));
    cfs_close(fd);
    return 0;
  }
}
