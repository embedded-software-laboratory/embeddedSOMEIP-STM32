// Plain UDP echo baseline server. See udp_baseline_app.h.
// Uses the lwIP RAW UDP API directly, so the RTT is the stack floor.
#include "udp_baseline_app.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"

#include <stdio.h>

// Same port as the SOME/IP server, so clients keep their default
#define BASELINE_PORT 8010

// Runs on the tcpip thread, so no core lock is needed here.
// udp_sendto does not take the pbuf, so free it.
static void baseline_echo_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);
  if (p == NULL) {
    return;
  }
  udp_sendto(pcb, p, addr, port);
  pbuf_free(p);
}

static void UdpBaselineTask(void *arg)
{
  (void)arg;

  // wait for DHCP to bind an address
  while (netif_default == NULL ||
         !netif_is_up(netif_default) ||
         ip4_addr_isany_val(*netif_ip4_addr(netif_default)))
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  // these calls run off the tcpip thread, so take the core lock
  struct udp_pcb *pcb = udp_new();
  if (pcb != NULL) {
    LOCK_TCPIP_CORE();
    err_t err = udp_bind(pcb, IP_ADDR_ANY, BASELINE_PORT);
    if (err == ERR_OK) {
      udp_recv(pcb, baseline_echo_cb, NULL);
    }
    UNLOCK_TCPIP_CORE();
    if (err != ERR_OK) {
      udp_remove(pcb);
      pcb = NULL;
    }
  }

  if (pcb != NULL) {
    printf("[UDP-BASE] plain-UDP echo listening on port %u\n", BASELINE_PORT);
  } else {
    printf("[UDP-BASE] ERROR: failed to open echo port %u\n", BASELINE_PORT);
  }

  // all echo work happens in the lwIP tcpip thread
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void udp_baseline_start(void)
{
  osThreadAttr_t attr = {0};
  attr.name = "UdpBase";
  // no SD serialization on this stack, so 2 KB is enough
  attr.stack_size = 2048;
  attr.priority = osPriorityNormal;
  osThreadNew(UdpBaselineTask, NULL, &attr);
}
