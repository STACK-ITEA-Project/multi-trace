#include "contiki.h"
#include "sys/node-id.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/energest.h"
#include "shell.h"
#include "shell-commands.h"
#include "app-message.h"
#include "icmp6-stats.h"
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#include "net/routing/rpl-lite/rpl-timers.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#endif
#include <inttypes.h>

#ifndef IN_DEVICE_DETECTION
#define IN_DEVICE_DETECTION 0
#endif /* IN_DEVICE_DETECTION */

#define BLACKHOLE_ATTACK (0 && IN_DEVICE_DETECTION)
#define FLOODING_ATTACK   1

#if IN_DEVICE_DETECTION
#include "network-attacks.h"
#include "attack-detection.h"

#ifdef CONTIKI_TARGET_COOJA
/* Cooja */
#define BLACKHOLE_ATTACK_NODE 9
#define BLACKHOLE_ATTACK_TIME (45 * 60 * CLOCK_SECOND)
/* #define BLACKHOLE_ATTACK_DURATION (60 * 60 * CLOCK_SECOND) */
#else /* CONTIKI_TARGET_COOJA */
/* RISE Testbed */
#define BLACKHOLE_ATTACK_NODE 29933 /* attacking: pi08 */
#define BLACKHOLE_ATTACK_TIME (30 * 60 * CLOCK_SECOND)
/* #define BLACKHOLE_ATTACK_DURATION (60 * 60 * CLOCK_SECOND) */
#endif /* ! CONTIKI_TARGET_COOJA */

#endif /* IN_DEVICE_DETECTION */

#include "sys/log.h"

#include "rpl.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)
#define SHOW_ENERGEST             (0 && ENERGEST_CONF_ON)

/* Implementation by Yonsei */
#define data_size 17
char dis_attack_state = 0;
#if FLOODING_ATTACK
static struct etimer attack_timer;
#endif /* FLOODING_ATTACK */
bool flooding_attack_send_dis = false;
/* __Implementation by Yonsei */

static struct simple_udp_connection udp_conn;
static bool is_attacker;

/*---------------------------------------------------------------------------*/
#if BLACKHOLE_ATTACK
static struct ctimer blackhole_attack_timer;
#ifdef BLACKHOLE_ATTACK_DURATION
static void
stop_attack(void *ptr)
{
  network_attacks_rpl_dio_fake_rank = 0;
  network_attacks_udp_drop_fwd = false;
  network_attacks_rpl_dao_fake_accept = false;
  network_attacks_rpl_dio_only_parent = false;
  network_attacks_rpl_dio_send = false;
  network_attacks_rpl_dio_reset = false;
  is_attacker = false;
  LOG_INFO("=========================\n");
  LOG_INFO("BLACKHOLE ATTACK - END!\n");
  LOG_INFO("=========================\n");
  /* Indicate no longer attacking but has been an attacker */
  LOG_ANNOTATE("#A color=WHITE\n");
}
#endif /* BLACKHOLE_ATTACK_DURATION */
/*---------------------------------------------------------------------------*/
static void
start_attack(void *ptr)
{
  network_attacks_init();
  LOG_INFO("=========================\n");
  LOG_INFO("BLACKHOLE ATTACK - BEGIN!\n");
  LOG_INFO("=========================\n");
  network_attacks_rpl_dio_fake_rank = 128;
  network_attacks_udp_drop_fwd = true;
  network_attacks_rpl_dao_fake_accept = true;
  network_attacks_rpl_dio_only_parent = true;
  network_attacks_rpl_dio_send = true;
  network_attacks_rpl_dio_reset = true;
  is_attacker = true;
  LOG_ANNOTATE("#A color=RED\n");
#ifdef BLACKHOLE_ATTACK_DURATION
  ctimer_set(&blackhole_attack_timer, BLACKHOLE_ATTACK_DURATION, stop_attack, NULL);
#endif /* BLACKHOLE_ATTACK_DURATION */
}
#endif /* BLACKHOLE_ATTACK */
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
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
  app_message_t *msg = (app_message_t *)data;
  int8_t rssi = (int8_t)uipbuf_get_attr(UIPBUF_ATTR_RSSI);
  
  LOG_INFO("Received ");
  if(datalen != sizeof(app_message_t)) {
    LOG_INFO_("unknown data of size %u from ", datalen);
  } else {
    LOG_INFO_("response %"PRIu32" from ", app_read_uint32(msg->seqno));
  }
  LOG_INFO_6ADDR(sender_addr);
  
  LOG_INFO_(", RSSI: (%d dBm)", rssi);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
/* Implementation by Yonsei */
static
PT_THREAD(cmd_dis_repeat_attack(struct pt *pt, shell_output_func output, char *args))
{

  PT_BEGIN(pt);
#if FLOODING_ATTACK
  dis_attack_state = 1;
  etimer_set(&attack_timer, 360 * CLOCK_SECOND);
#endif /* FLOODING_ATTACK */
  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
static const struct shell_command_t client_commands[] = {
  { "dis-repeat-attack", cmd_dis_repeat_attack, "'> attack': Sets the node in dis-repeat attack mode." },
  { NULL, NULL, NULL },
};
/* __Implementation by Yonsei */
/*---------------------------------------------------------------------------*/
static struct shell_command_set_t client_shell_command_set = {
  .next = NULL,
  .commands = client_commands,
};
/*---------------------------------------------------------------------------*/
#if SHOW_ENERGEST
static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}
#endif /* SHOW_ENERGEST */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint32_t count;
  static app_message_t msg;
  uip_ipaddr_t dest_ipaddr;
  rpl_instance_t *default_instance;
  uint16_t rank;
  uint8_t dag_version;

  /* Implementation by Yonsei */
  uint32_t data_arr[data_size];
  /* __Implementation by Yonsei */
  PROCESS_BEGIN();

  shell_command_set_register(&client_shell_command_set);

#if IN_DEVICE_DETECTION
  /* Implementation by RISE */
  attack_detection_init();
#endif /* IN_DEVICE_DETECTION */

#if BLACKHOLE_ATTACK
  /* Implementation by RISE */
  if(node_id == BLACKHOLE_ATTACK_NODE) {
    LOG_INFO("Preparing attack in %lu seconds\n",
             (unsigned long)(BLACKHOLE_ATTACK_TIME / CLOCK_SECOND));
    ctimer_set(&blackhole_attack_timer, BLACKHOLE_ATTACK_TIME, start_attack, NULL);
  }
#endif /* BLACKHOLE_ATTACK */

  /* Initialize ICMP6/RPL statistics */
  icmp6_stats_init();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  LOG_ANNOTATE("#A color=GREEN\n");

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    
    default_instance = rpl_get_default_instance();
    rank = default_instance ? default_instance->dag.rank : RPL_INFINITE_RANK;
    dag_version = default_instance ? default_instance->dag.version : 0;

    /* Implementation by Yonsei */
    data_arr[0]  = (uint32_t) count;
    data_arr[1]  = (uint32_t) rank;
    data_arr[2]  = (uint32_t) dag_version;
    data_arr[3]  = (uint32_t) icmp6_stats.dis_uc_recv;
    data_arr[4]  = (uint32_t) icmp6_stats.dis_mc_recv;
    data_arr[5]  = (uint32_t) icmp6_stats.dis_uc_sent;
    data_arr[6]  = (uint32_t) icmp6_stats.dis_mc_sent;
    data_arr[7]  = (uint32_t) icmp6_stats.dio_uc_recv;
    data_arr[8]  = (uint32_t) icmp6_stats.dio_mc_recv;
    data_arr[9]  = (uint32_t) icmp6_stats.dio_uc_sent;
    data_arr[10] = (uint32_t) icmp6_stats.dio_mc_sent;
    data_arr[11] = (uint32_t) icmp6_stats.dao_recv;
    data_arr[12] = (uint32_t) icmp6_stats.dao_sent;
    data_arr[13] = (uint32_t) icmp6_stats.dao_ack_recv;
    data_arr[14] = (uint32_t) icmp6_stats.dao_ack_sent;
    data_arr[15] = (uint32_t) curr_instance.dag.dio_intcurrent;
    data_arr[16] = (uint32_t) curr_instance.dag.lifetime;

    /* __Implementation by Yonsei */

    
        // LOG_INFO_(",dia-r:%"PRIu32",tots:%"PRIu32"\n",
    //         icmp6_stats.dao_recv, icmp6_stats.rpl_total_sent);
    // LOG_INFO_(",rssi:%"PRIu32,
    //           cc2420_last_rssi);

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      simple_udp_sendto(&udp_conn, data_arr, data_size*sizeof(uint32_t), &dest_ipaddr);
      
      LOG_INFO("Sending request %"PRIu32" to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      memset(&msg, 0, sizeof(msg));
      app_write_uint32(msg.seqno, count);
      default_instance = rpl_get_default_instance();
      app_write_uint16(msg.rpl_rank, default_instance->dag.rank);

      simple_udp_sendto(&udp_conn, &msg, sizeof(msg), &dest_ipaddr);
      count++;

      /* Hide init, show rank */
      if(count == 1) {
        LOG_ANNOTATE("#A init\n");
      }
      LOG_ANNOTATE("#A r=%u\n", rank);

#if IN_DEVICE_DETECTION
      /* Implementation by RISE */
      if(is_attacker) {
        // Reset variables
        attack_detection_reset();
      } else {
        if(attack_detection_run()) {
          /* Blackhole-like attack detected */
          LOG_ANNOTATE("#A color=YELLOW\n");
        }
      }
#endif /* IN_DEVICE_DETECTION */

      /* Implementation by Yonsei */

#if FLOODING_ATTACK
      LOG_INFO("dis_attack_state: %d\n", dis_attack_state);
      if (dis_attack_state == 1 && etimer_expired(&attack_timer)){
        dis_attack_state = 2;
        LOG_INFO("FLOODING ATTACK IS STARTED!\n");
        flooding_attack_send_dis = true; // Used for sending dis packet even after joining the DODAG
        rpl_timers_dio_reset("FLOODING ATTACK IS STARTED!");
        is_attacker = true;
        etimer_set(&attack_timer, 360 * CLOCK_SECOND);
        LOG_ANNOTATE("#A color=MAGENTA\n");
      } else if (dis_attack_state == 2  && etimer_expired(&attack_timer)){
        dis_attack_state = 1;
        LOG_INFO("FLOODING ATTACK IS FINISHED!\n");
        flooding_attack_send_dis = false;
        rpl_timers_dio_reset("FLOODING ATTACK IS FINISHED!");
        is_attacker = false;
        LOG_ANNOTATE("#A color=WHITE\n");
        etimer_set(&attack_timer, 360 * CLOCK_SECOND);
      }


      // Naive dis-send right after a udp packet send
      if(flooding_attack_send_dis){
        rpl_icmp6_dis_output(NULL);
      }
#endif /* FLOODING_ATTACK */
      /* __Implementation by Yonsei */
    } else {
      LOG_INFO("Not reachable yet\n");
    }
  
/* Implementation by Yonsei */
if(flooding_attack_send_dis){
    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL/2
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
    }
    else
    {
    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
}
/* __Implementation by Yonsei */
    /* Example printing energest data */
#if SHOW_ENERGEST
    /*
     * Update all energest times. Should always be called before energest
     * times are read.
     */
    energest_flush();

    printf("\nEnergest:\n");
    printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
    printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));
#endif /* SHOW_ENERGEST */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/