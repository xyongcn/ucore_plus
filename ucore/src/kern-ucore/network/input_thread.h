#include <types.h>
#include <sem.h>

struct netif;
struct pbuf;

void network_input_thread_init();
void network_input_thread_notify(struct netif *netif, struct pbuf *pbuf);
void network_input_thread_main(void* args);
