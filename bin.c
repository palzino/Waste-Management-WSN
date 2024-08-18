#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/log.h"
#include "sys/node-id.h"

#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#define LOG_MODULE "Bin"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define BUF_SIZE 4

#define SEND_INTERVAL (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static int dirty_time = 0; // Time in minutes between cleans
static int real_time = 0; // Time in minutes real
static int fullness = 0; // Fullness percentage of the bin

int random_in_range(int min, int max) {
    // Ensure min is less than or equal to max
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    // Calculate the range and generate the random number
    return min + (random_rand() % (max - min + 1));
}

static int calculate_waste_exponential(int initial_waste, int growth_rate, int time) {
  double growth_rate_decimal = (double)growth_rate / 100;
  double result = initial_waste * expf(growth_rate_decimal * time);
  return (int)result;
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "Bin");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  // Convert to null-terminated string
  char *received_data = (char *)data;
  received_data[datalen] = '\0';
  // Compare to "EMPTY"
  if(strcmp(received_data, "EMPTY") == 0) {
    // Log empty has been received
    LOG_INFO("[emptied]{");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("}\n");
    // Reset the bin
    fullness = 0;
    dirty_time = 0;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint8_t buf[BUF_SIZE];
  uip_ipaddr_t dest_ipaddr;

  static int initial_waste = 0;
  static int growth_rate = 0;

  PROCESS_BEGIN();

  // Set seed as node id
  random_init(node_id);

  // Calculate values at random
  initial_waste = random_in_range(0, 10);
  growth_rate = random_in_range(0, 5);
  // Print these
  LOG_INFO("[model_params]{%d,%d}\n", initial_waste, growth_rate);

  // Initialise the UDP connection
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);
  // Set the time for the interval
  etimer_set(&periodic_timer, SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      // Calculate the fullness value at current time
      fullness += calculate_waste_exponential(initial_waste, growth_rate, dirty_time);
      // Serialize fullness int to byte array
      memcpy(buf, &fullness, sizeof(fullness));
      // Send the data
      simple_udp_sendto(&udp_conn, buf, sizeof(buf), &dest_ipaddr);
      // Log message sent
      LOG_INFO("[sent_fullness_log]{%u,%d}\n",(uint8_t)fullness, real_time);
    } else {
      LOG_INFO("[no_connection]\n");
    }

    dirty_time += 1;
    real_time += 1;
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
