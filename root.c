#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "sys/energest.h"
#include "net/routing/rpl-lite/rpl-dag-root.h"

#define LOG_MODULE "RPL-SERVER"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_SERVER_PORT 5678

#define STATS_INTERVAL (60 * CLOCK_SECOND)
#define SIMULATION_TIME (34 * 60 * CLOCK_SECOND)


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


static uint32_t total_rx = 0;
static clock_time_t delay_sum = 0;
static uint32_t delay_count = 0;


static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
  packet_t *pkt = (packet_t *)data;
  clock_time_t recv_time = clock_time();

  if(pkt->send_time > recv_time ||
     recv_time - pkt->send_time > 10 * CLOCK_SECOND) {
    LOG_INFO("RX node=%u seq=%lu DELAY INVALID, skip\n",
             pkt->node_id, (unsigned long)pkt->seq);
    total_rx++;
    return;
  }

  clock_time_t e2e_delay = recv_time - pkt->send_time;

  total_rx++;
  delay_sum += e2e_delay;
  delay_count++;

  LOG_INFO("RX node=%u seq=%lu delay=%lu ticks\n",
           pkt->node_id,
           (unsigned long)pkt->seq,
           (unsigned long)e2e_delay);
}


PROCESS(rpl_server_process, "RPL Server");
AUTOSTART_PROCESSES(&rpl_server_process);

PROCESS_THREAD(rpl_server_process, ev, data)
{
  static struct etimer stats_timer;
  static struct etimer stop_timer;

  PROCESS_BEGIN();

  rpl_dag_root_start();

  simple_udp_register(&udp_conn,
                      UDP_SERVER_PORT,
                      NULL,
                      0,
                      udp_rx_callback);

  etimer_set(&stats_timer, STATS_INTERVAL);
  etimer_set(&stop_timer, SIMULATION_TIME);

  LOG_INFO("Server started\n");

  while(1) {
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&stop_timer)) {
      LOG_INFO("===== SIMULATION FINISHED =====\n");

      energest_flush();

      clock_time_t avg_delay =
        (delay_count > 0) ? (delay_sum / delay_count) : 0;

      uint64_t t_tx  = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      uint64_t t_rx  = energest_type_time(ENERGEST_TYPE_LISTEN);
      uint64_t t_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      uint64_t t_lpm = energest_type_time(ENERGEST_TYPE_LPM);

      double E_tx  = (t_tx  * I_TX  * VOLTAGE) / ENERGEST_SECOND;
      double E_rx  = (t_rx  * I_RX  * VOLTAGE) / ENERGEST_SECOND;
      double E_cpu = (t_cpu * I_CPU * VOLTAGE) / ENERGEST_SECOND;
      double E_lpm = (t_lpm * I_LPM * VOLTAGE) / ENERGEST_SECOND;

      double E_total = E_tx + E_rx + E_cpu + E_lpm;

      LOG_INFO("FINAL RX        = %lu\n", (unsigned long)total_rx);
      LOG_INFO("FINAL Avg Delay = %lu ticks\n", (unsigned long)avg_delay);
      LOG_INFO("FINAL Energy (mJ): TX=%.2f RX=%.2f CPU=%.2f LPM=%.2f TOTAL=%.2f\n",
               E_tx * 1000, E_rx * 1000, E_cpu * 1000, E_lpm * 1000, E_total * 1000);

      break;
    }

    if(etimer_expired(&stats_timer)) {
      energest_flush();

      clock_time_t avg_delay =
        (delay_count > 0) ? (delay_sum / delay_count) : 0;

      uint64_t t_tx  = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      uint64_t t_rx  = energest_type_time(ENERGEST_TYPE_LISTEN);
      uint64_t t_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      uint64_t t_lpm = energest_type_time(ENERGEST_TYPE_LPM);

      double E_tx  = (t_tx  * I_TX  * VOLTAGE) / ENERGEST_SECOND;
      double E_rx  = (t_rx  * I_RX  * VOLTAGE) / ENERGEST_SECOND;
      double E_cpu = (t_cpu * I_CPU * VOLTAGE) / ENERGEST_SECOND;
      double E_lpm = (t_lpm * I_LPM * VOLTAGE) / ENERGEST_SECOND;

      double E_total = E_tx + E_rx + E_cpu + E_lpm;

      LOG_INFO("SERVER_STATS RX=%lu AvgDelay=%lu ticks EnergyTOTAL=%.2f\n",
               (unsigned long)total_rx, (unsigned long)avg_delay, E_total * 1000);

      etimer_reset(&stats_timer);
    }
  }

  PROCESS_END();
}
