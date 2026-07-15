#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "sys/energest.h"
#include "net/linkaddr.h"

#define LOG_MODULE "RPL-CLIENT"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (30 * CLOCK_SECOND)
#define STATS_INTERVAL (60 * CLOCK_SECOND)


#define VOLTAGE 3.0
#define I_TX    0.017
#define I_RX    0.019
#define I_CPU   0.0018
#define I_LPM   0.0000545

static struct simple_udp_connection udp_conn;


typedef struct {
  uint8_t  node_id;
  uint32_t seq;
  clock_time_t send_time;
} packet_t;


static uint32_t tx_count = 0;
static uint32_t seq_id = 0;


PROCESS(rpl_client_process, "RPL Client");
AUTOSTART_PROCESSES(&rpl_client_process);

PROCESS_THREAD(rpl_client_process, ev, data)
{
  static struct etimer send_timer;
  static struct etimer stats_timer;
  static packet_t pkt;
  uip_ipaddr_t dest_ipaddr;

  uint8_t my_node_id = linkaddr_node_addr.u8[LINKADDR_SIZE - 1];

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn,
                      UDP_CLIENT_PORT,
                      NULL,
                      UDP_SERVER_PORT,
                      NULL);

  etimer_set(&send_timer, SEND_INTERVAL);
  etimer_set(&stats_timer, STATS_INTERVAL);

  LOG_INFO("Client %u waiting for DODAG...\n", my_node_id);

  while(1) {
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&send_timer)) {

      if(NETSTACK_ROUTING.node_is_reachable() &&
         NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

        pkt.node_id = my_node_id;
        pkt.seq = seq_id++;
        pkt.send_time = clock_time();

        simple_udp_sendto(&udp_conn,
                          &pkt,
                          sizeof(pkt),
                          &dest_ipaddr);

        tx_count++;
        LOG_INFO("TX node=%u seq=%lu\n",
                 my_node_id, (unsigned long)pkt.seq);
      } else {
        LOG_INFO("Root unreachable\n");
      }

      etimer_reset(&send_timer);
    }

    if(etimer_expired(&stats_timer)) {
      energest_flush();

      uint64_t t_tx  = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      uint64_t t_rx  = energest_type_time(ENERGEST_TYPE_LISTEN);
      uint64_t t_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      uint64_t t_lpm = energest_type_time(ENERGEST_TYPE_LPM);

      double E_tx  = (t_tx  * I_TX  * VOLTAGE) / ENERGEST_SECOND;
      double E_rx  = (t_rx  * I_RX  * VOLTAGE) / ENERGEST_SECOND;
      double E_cpu = (t_cpu * I_CPU * VOLTAGE) / ENERGEST_SECOND;
      double E_lpm = (t_lpm * I_LPM * VOLTAGE) / ENERGEST_SECOND;

      double E_total = E_tx + E_rx + E_cpu + E_lpm;

      LOG_INFO("CLIENT_STATS node=%u TX=%lu ENERGY=%.2f\n",
               my_node_id, (unsigned long)tx_count, E_total * 1000);

      etimer_reset(&stats_timer);
    }
  }

  PROCESS_END();
}
