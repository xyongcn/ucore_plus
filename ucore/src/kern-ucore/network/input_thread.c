#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "ethernet.h"
#include "input_thread.h"

semaphore_t thread_data_available;

static struct netif *last_netif;
static struct pbuf *last_pbuf;

void network_input_thread_init()
{
  sem_init(&thread_data_available, 0);
}

void network_input_thread_notify(struct netif *netif, struct pbuf *pbuf)
{
  last_netif = netif;
  last_pbuf = pbuf;
  up(&thread_data_available);
}

void network_input_thread_main(void* args)
{
  for(;;) {
    down(&thread_data_available);
    if (last_netif->input(last_pbuf, last_netif) != ERR_OK) {
      kprintf("network_input_thread_main: input error\n");
      pbuf_free(last_pbuf);
      last_pbuf = NULL;
    }
  }
}
