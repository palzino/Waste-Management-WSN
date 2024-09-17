#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"

#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "Control"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define BUF_SIZE 4

#define CHECK_INTERVAL (60 * CLOCK_SECOND)
#define BUSY_INTERVAL (30 * 60 * CLOCK_SECOND)
#define MAX_BINS 100

static struct simple_udp_connection udp_conn;

static bool will_empty = false;
static bool actually_emptying = false;
static uip_ipaddr_t *emptying = NULL;

typedef struct {
  uip_ipaddr_t *addr;
  int fullness;
} fullness_log;

typedef struct {
  fullness_log logs[MAX_BINS];
  size_t size;
} fullness_list;

void fullness_list_init(fullness_list *list) {
  list->size = 0;
}

void fullness_list_free(fullness_list *list) {
  for (size_t i = 0; i < list->size; i++) {
    free(list->logs[i].addr);
  }
  list->size = 0;
}

bool fullness_list_add(fullness_list *list, uip_ipaddr_t *addr, float fullness) {
  // Check first if the addr is already in the list
  for (size_t i = 0; i < list->size; i++) {
    if (uip_ipaddr_cmp(list->logs[i].addr, addr)) {
      // Just update the fullness reading
      list->logs[i].fullness = fullness;
      return false;
    }
  }
  // Otherwise add to the list
  if (list->size < MAX_BINS) {
    // Allocate new memory for the address and copy the data
    list->logs[list->size].addr = malloc(sizeof(uip_ipaddr_t));
    if (list->logs[list->size].addr != NULL) {
      uip_ipaddr_copy(list->logs[list->size].addr, addr);
      list->logs[list->size].fullness = fullness;
      list->size++;
      return true;
    } else {
      // Handle memory allocation failure
      printf("[error]{Memory allocation failed for IP address}\n");
      return false;
    }
  } else {
    // Handle list full scenario
    printf("[error]{List is full, cannot add more entries}\n");
    return false;
  }
}

bool fullness_list_size_is(const fullness_list *list, size_t size) {
    return list->size == size;
}

int sort_logs(const void *a, const void *b) {
  int val_a = ((fullness_log *)a)->fullness;
  int val_b = ((fullness_log *)b)->fullness;
  return (val_b > val_a) - (val_a > val_b); // Descending order
}

void fullness_list_sort(fullness_list *list) {
  for (size_t i = 0; i < list->size - 1; i++) {
    for (size_t j = 0; j < list->size - 1 - i; j++) {
      if (list->logs[j].fullness < list->logs[j+1].fullness) {
        // Swap
        fullness_log temp = list->logs[j];
        list->logs[j] = list->logs[j+1];
        list->logs[j+1] = temp;
      }
    }
  }
}

uip_ipaddr_t *fullness_list_get_max_ip(fullness_list *list) {
    if (list->size == 0) return NULL;
    return list->logs[0].addr;
}

static fullness_list list; // The fullness list of bins
static struct etimer check_timer; // Timer to check if all readings are in
static struct etimer busy_timer; // Timer to wait for whilst "busy" during cleaning

// Register the receiver and cleaner processes
PROCESS(receiver_process, "Receiver Process");
PROCESS(cleaner_process, "Cleaner Process");
PROCESS(emptying_processs, "Busy Process");

/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  if(datalen == BUF_SIZE) {
    int received_fullness;
    // Deserialise the bytes into the float
    memcpy(&received_fullness, data, sizeof(received_fullness));
    // Record this in the fullness log
    bool appended = fullness_list_add(&list, (uip_ipaddr_t *)sender_addr, received_fullness);
    // Print details depending on if was added new or updated
    if (appended) {
      LOG_INFO("[new_log]{");
    } else {
      LOG_INFO("[updated_log]{");
    }
    LOG_INFO_6ADDR((uip_ipaddr_t *)sender_addr);
    LOG_INFO_(",%d}\n", received_fullness);
  }
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(receiver_process, ev, data)
{
  PROCESS_BEGIN();

  // Initialise the fullness list
  fullness_list_init(&list);

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(cleaner_process, ev, data)
{
  PROCESS_BEGIN();

  // Set up the timer
  etimer_set(&check_timer, CHECK_INTERVAL);

  while (1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER && data == &check_timer) {
      LOG_INFO("[buffer_size]{%d/21}\n", list.size);
      if (fullness_list_size_is(&list, 21) && !(actually_emptying || will_empty)) {
        // Sort the list of fullness readings
        fullness_list_sort(&list);
        // Get the IP of the fullest bin
        uip_ipaddr_t *addr = fullness_list_get_max_ip(&list);
        // Store as emptying
        emptying = addr;
        // Print details
        LOG_INFO("[will_empty]{");
        LOG_INFO_6ADDR(addr);
        LOG_INFO_("}\n");
        // Set will empty
        will_empty = true;
      }

      // Reset the time
      etimer_reset(&check_timer);
    }
  }

  PROCESS_END();
}

PROCESS_THREAD(emptying_processs, ev, data)
{
  PROCESS_BEGIN();

  // Set up the timer
  etimer_set(&busy_timer, CHECK_INTERVAL);

  while (1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER && data == &busy_timer) {
      // If actually busy and the timer has gone off
      if (actually_emptying) {
        // Prepare the "EMPTY" string
        static char str[10];
        snprintf(str, sizeof(str), "EMPTY");
        // Send the string
        simple_udp_sendto(&udp_conn, str, strlen(str), emptying);
        // Print details
        LOG_INFO("[sent_empty_signal]{");
        LOG_INFO_6ADDR(emptying);
        LOG_INFO_("}\n");
        // Print details
        LOG_INFO("[buffer_reset]\n");
        // Clear the list
        fullness_list_free(&list);
        // Reset lock
        will_empty = false;
        actually_emptying = false;
      }


      if (will_empty) {
        // Reset the time
        etimer_set(&busy_timer, BUSY_INTERVAL);
        actually_emptying = true;
        will_empty = false;
        // Prepare the "EMPTYING" string
        static char str[10];
        snprintf(str, sizeof(str), "EMPTYING");
        // Send the string
        simple_udp_sendto(&udp_conn, str, strlen(str), emptying);
        // Print details
        LOG_INFO("[started_emptying]{");
        LOG_INFO_6ADDR(emptying);
        LOG_INFO_("}\n");
      } else {
        // Reset the time
        etimer_set(&busy_timer, CHECK_INTERVAL);
      }
    }
  }

  PROCESS_END();
}

// Start the receiver and cleaner processes
AUTOSTART_PROCESSES(&receiver_process, &cleaner_process, &emptying_processs);