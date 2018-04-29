#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/tcp-socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 80

static struct tcp_socket socket;

#define INPUTBUFSIZE 400
static uint8_t inputbuf[INPUTBUFSIZE];

#define OUTPUTBUFSIZE 400
static uint8_t outputbuf[OUTPUTBUFSIZE];



PROCESS(tcp_client_process, "TCP client");
AUTOSTART_PROCESSES(&tcp_client_process);

uip_ipaddr_t server_ipaddr;
/*---------------------------------------------------------------------------*/
static int
input(struct tcp_socket *s, void *ptr,
      const uint8_t *inputptr, int inputdatalen)
{
    printf("inputptr '%.*s'\n", inputdatalen, inputptr);
    /* Return the number of data bytes we received, to keep them all
       in the buffer. */
    return 0;
}
/*---------------------------------------------------------------------------*/
static void
event(struct tcp_socket *s, void *ptr,
      tcp_socket_event_t ev)
{
  printf("event %d\n", ev);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcp_client_process, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer period_timer;

  tcp_socket_register(&socket, NULL,
               inputbuf, sizeof(inputbuf),
               outputbuf, sizeof(outputbuf),
               input, event);
  //tcp_socket_listen(&socket, SERVER_PORT);
  etimer_set(&period_timer, CLOCK_SECOND * 5);
  while(1) {
    PROCESS_WAIT_EVENT();
    if(data == &period_timer) {
      if(ev == PROCESS_EVENT_TIMER){
        if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&server_ipaddr)) {
          if(tcp_socket_connect(&socket, &server_ipaddr, SERVER_PORT) > 0) {
            printf("Connecting on %d\n", SERVER_PORT);
            /* Send header */
            printf("sending header\n");
            tcp_socket_send_str(&socket, "HTTP/1.0 200 ok\r\nServer: Contiki tcp-socket example\r\n\r\n");

            tcp_socket_close(&socket);
          } else {
            printf("tcp Connecting fail!\n");
          }
        } else{
          printf("not reachable yet!\n");
        }
      }
      etimer_reset(&period_timer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
