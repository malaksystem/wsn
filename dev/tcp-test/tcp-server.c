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



PROCESS(tcp_server_process, "TCP server");
AUTOSTART_PROCESSES(&tcp_server_process);

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
PROCESS_THREAD(tcp_server_process, ev, data)
{
  PROCESS_BEGIN();

  NETSTACK_ROUTING.root_start();

  tcp_socket_register(&socket, NULL,
               inputbuf, sizeof(inputbuf),
               outputbuf, sizeof(outputbuf),
               input, event);
  tcp_socket_listen(&socket, SERVER_PORT);

  printf("Listening on %d\n", SERVER_PORT);
  while(1) {
    PROCESS_PAUSE();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
