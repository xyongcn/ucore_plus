#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "ethernet.h"
#include "input_thread.h"

semaphore_t thread_data_available;

static struct netif *last_netif = NULL;
static struct pbuf *last_pbuf = NULL;

void network_input_thread_init()
{
  sem_init(&thread_data_available, 0);
}

void network_input_thread_notify(struct netif *netif)
{
  /*if(last_pbuf != NULL) {
    kprintf("=======network_input_thread_notify\n");
  }
  last_netif = netif;
  last_pbuf = pbuf;*/
  up(&thread_data_available);
}

void network_input_thread_main(void* args)
{
  for(;;) {
    down(&thread_data_available);
    ethernet_lwip_process_data();
  }
}
