#include <network.h>
#include <ethernet.h>
#include "lwip/init.h"
#include "input_thread.h"

void network_init()
{
  ethernet_init();
  lwip_init();
  network_input_thread_init();
}
