#ifndef MSG_SEND_H_
#define MSG_SEND_H_

#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/tcp-socket.h"


enum msg_type
{
  SENSOR_DATA,
  TOPOLOGY_DATA,
  ENERGY_DATA,
};

struct neighbor_info
{
  uip_ipaddr_t ipaddr;
  int16_t rssi;      
};

#define TCP_SERVER_PORT 80




int open_data_recording(void);
void collect_common_set_send_active(int active);
int is_send_active(void);

int8_t write_txpower_file(int value);

void collect_common_send(struct simple_udp_connection *c);
void topology_send(struct simple_udp_connection *c);


#endif /* MSG_SEND_H_ */

