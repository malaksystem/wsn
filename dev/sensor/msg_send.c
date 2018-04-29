#include "msg_send.h"
#include "net/routing/routing.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "collect.h"

#include "net/link-stats.h"
#include "net/ipv6/uip.h"

#include "sys/log.h"

#include <stdio.h> /* For LOG_INFO() */
#include <string.h>
#include <ctype.h>



#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

#include "collect-sensor.h"

#define READ_DATA_LEN       600


#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO


static int send_active = 0;

static int record_fd, file_num_fd;

int open_data_recording(void)
{
  uint32_t num = 0;
  char buf[16];
  file_num_fd = cfs_open("num", CFS_READ);
  if (file_num_fd < 0) {
    file_num_fd = cfs_open("num", CFS_READ | CFS_WRITE);
    if (file_num_fd < 0){
      printf("write num file error!\n");
    } else {
      cfs_seek(file_num_fd, 0, CFS_SEEK_SET);
      cfs_write(file_num_fd, &num, sizeof(num));
    }
  } else {
    cfs_close(file_num_fd);
    file_num_fd = cfs_open("num", CFS_READ | CFS_WRITE | CFS_APPEND);
    cfs_seek(file_num_fd, 0, CFS_SEEK_SET);
    cfs_read(file_num_fd, &num, sizeof(num));
    num++;
    cfs_seek(file_num_fd, 0, CFS_SEEK_SET);
    cfs_write(file_num_fd, &num, sizeof(num));
  }
  cfs_close(file_num_fd);

  sprintf(buf, "data%ld", num);

  record_fd = cfs_open(buf, CFS_READ | CFS_WRITE | CFS_APPEND);
  if(record_fd < 0) {
    printf("open record file fail!\n");
  }
  return record_fd;
}

void
collect_common_set_send_active(int active)
{
  send_active = active;
}

int is_send_active(void)
{
  return send_active;
}
/*---------------------------------------------------------------------------*/
static void
ipaddr2str(const uip_ipaddr_t *addr, char buf[40])
{
  if (addr == NULL) {
    memcpy(buf, "NULL", 5);
    return;
  }
  uint16_t a;
  int i, f;
  uint8_t c = 0;
  for(i = 0, f = 0; i < (int)sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        buf[c++] = ':';
        buf[c++] = ':';
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        buf[c++] = ':';
      }
      c += snprintf(buf + c, 40 - c, "%x", a);
    }
  }
  buf[c] = 0;
}
/*---------------------------------------------------------------------------*/
void
collect_common_send(struct simple_udp_connection *c)
{
  static uint16_t seqno;
  struct {
    uint16_t seqno;
    uint16_t type;
    struct collect_data_msg msg;
  } msg;
  /* struct collect_neighbor *n; */
  uint16_t parent_etx;
  uint16_t rtmetric;
  uint16_t num_neighbors;
  uint16_t beacon_interval;
  rpl_parent_t *preferred_parent;
  linkaddr_t parent;
  rpl_dag_t *dag;

  memset(&msg, 0, sizeof(msg));
  seqno++;
  if(seqno == 0) {
    /* Wrap to 128 to identify restarts */
    seqno = 128;
  }
  msg.seqno = seqno;
  msg.type = SENSOR_DATA;

  linkaddr_copy(&parent, &linkaddr_null);
  parent_etx = 0;

  /* Let's suppose we have only one instance */
  dag = rpl_get_any_dag();
  if(dag != NULL) {
    preferred_parent = dag->preferred_parent;
    if(preferred_parent != NULL) {
      uip_ds6_nbr_t *nbr;
      nbr = uip_ds6_nbr_lookup(rpl_parent_get_ipaddr(preferred_parent));
      if(nbr != NULL) {
        /* Use parts of the IPv6 address as the parent address, in reversed byte order. */
        parent.u8[LINKADDR_SIZE - 1] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 2];
        parent.u8[LINKADDR_SIZE - 2] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1];
        //parent_etx = rpl_get_parent_rank((uip_lladdr_t *) uip_ds6_nbr_get_ll(nbr)) / 2;
        parent_etx = 0;
      }
    }
    rtmetric = dag->rank;
    beacon_interval = (uint16_t) ((2L << dag->dio_intcurrent) / 1000);
    num_neighbors = uip_ds6_nbr_num();
  } else {
    rtmetric = 0;
    beacon_interval = 0;
    num_neighbors = 0;
  }
  collect_construct_message(&msg.msg, &parent,
                                 parent_etx, rtmetric,
                                 num_neighbors, beacon_interval);

  
  if(is_send_active()) {
    LOG_INFO("send sensor!\n");
    simple_udp_sendto(c, &msg, sizeof(msg),
                        &dag->dag_id);
  }
  
  LOG_INFO("record!\n");
  cfs_write(record_fd, &msg, sizeof(msg));

  cfs_write(record_fd, "\r\n", 2);
}
/*---------------------------------------------------------------------------*/
void topology_send(struct simple_udp_connection *c)
{
  uip_ds6_nbr_t *nbr;
  const linkaddr_t *lladdr;
  rpl_dag_t *dag;
  static uint16_t seqno;

  char buf[40];

  uint8_t i;

  struct {
    uint16_t seqno;
    uint16_t type;
    uint16_t neighbor_num;
    uint16_t clock;
    struct neighbor_info neighbor[NBR_TABLE_MAX_NEIGHBORS];
  } msg;

  memset(&msg, 0, sizeof(msg));
  seqno++;
  if(seqno == 0) {
    /* Wrap to 128 to identify restarts */
    seqno = 128;
  }

  msg.seqno = seqno;
  msg.type = TOPOLOGY_DATA;

  dag = rpl_get_any_dag();

  LOG_INFO("-----------Nbr Table-----------\n");
  for(nbr = nbr_table_head(ds6_neighbors), i = 0; nbr; nbr = nbr_table_next(ds6_neighbors, nbr), i++) {
    lladdr =
      (const linkaddr_t *)uip_ds6_nbr_lladdr_from_ipaddr(&nbr->ipaddr);
    msg.neighbor[i].ipaddr = *(uip_ds6_nbr_ipaddr_from_lladdr((const uip_lladdr_t *)lladdr));
    msg.neighbor[i].rssi = link_stats_from_lladdr(lladdr)->rssi;

    ipaddr2str(&msg.neighbor[i].ipaddr, buf);
    
    LOG_INFO("neighbor: %d, ip: %s, rssi: %d\n", i, buf, msg.neighbor[i].rssi);
  }
  LOG_INFO("-------------End---------------\n");



  msg.neighbor_num = i;
  msg.clock = clock_time();

  if(is_send_active()) {
    LOG_INFO("send topology!\n");
    simple_udp_sendto(c, &msg, sizeof(msg) - (NBR_TABLE_MAX_NEIGHBORS - i) * sizeof(struct neighbor_info),
                        &dag->dag_id);
  }

  cfs_write(record_fd, &msg, sizeof(msg) - (NBR_TABLE_MAX_NEIGHBORS - i) * sizeof(struct neighbor_info));
  cfs_write(record_fd, "\r\n", 2);
}
