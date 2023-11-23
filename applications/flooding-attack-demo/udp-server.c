/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "app-message.h"
#include "icmp6-stats.h"
#include <inttypes.h>
#include "project-conf.h"

/* Implementation by Yonsei */
#if RUN_MODEL
#include "model.h"
#endif /* RUN_MODEL */
#define FDATK_NCLI 19 // CHANGE ME:number of clients
/* __Implementation by Yonsei */

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;

/* Implementation by Yonsei */

typedef struct {
  uint32_t data[FDATK_NCOL];
} FDATK_DATA_PERCLI;

typedef struct {
  FDATK_DATA_PERCLI data[FDATK_NCLI];
  uip_ipaddr_t sender[FDATK_NCLI];
  uint16_t n_registered_cli;
  bool has_new_data[FDATK_NCLI];
} FDATK_DATA_INPUT;

FDATK_DATA_INPUT fdatk_model_input[1];
// #endif /* RUN_MODEL */
/* __Implementation by Yonsei */


PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/


/* Implementation by Yonsei */
#if RUN_MODEL
static int
run_inference(int* inference_result)
{
  // call detection function
    float input[FDATK_NCLI * FDATK_NCOL];

    for (int i=0; i<FDATK_NCLI; i++){
        for (int j=0; j<FDATK_NCOL; j++){
            input[i*FDATK_NCOL + j] = fdatk_model_input->data[i].data[j];
        }
    }

  *inference_result = AttackDectectionExample(input);
  return 0;
}
# endif /* RUN_MODEL */

static void reset_model(){
  LOG_INFO("reset_model\n");
  for (int i=0; i < FDATK_NCLI; i++){
      fdatk_model_input->has_new_data[i] = false;
  }
}

static void init_model(){
  LOG_INFO("Init model\n");
# if RUN_MODEL
  Init_Detection();
# endif /* RUN_MODEL */
  
  for (int i=0; i < FDATK_NCLI; i++){
      uip_ip6addr(&(fdatk_model_input->sender[i]), 0, 0, 0, 0, 0, 0, 0, 0);
  }
  fdatk_model_input->n_registered_cli = 0;
  reset_model();
}

static bool input_ready(){
  int has_new_data = 0;
  for (int i=0; i < FDATK_NCLI; i++){
    // if any client has not sent data, return false
    if (fdatk_model_input->has_new_data[i]){
      has_new_data++;
    }
  }

  LOG_INFO("input_ready: %d/%d\n", has_new_data+1, FDATK_NCLI);
  return has_new_data == FDATK_NCLI-1;
}
/* __Implementation by Yonsei */

static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  
  LOG_INFO("Received ");
  int8_t rssi = (int8_t)uipbuf_get_attr(UIPBUF_ATTR_RSSI);
  if(datalen == sizeof(app_message_t))
  {
    app_message_t *msg = (app_message_t *)data;
    LOG_INFO_("message %"PRIu32" (rank %u) from ",
              app_read_uint32(msg->seqno),
              app_read_uint16(msg->rpl_rank));
  }
  /* Implementation by Yonsei */
  else if(datalen == sizeof(FDATK_DATA_PERCLI))
  {
    // FLOODING ATTACK DETECTION MODULE
    // RECEIVES RPL STATISTICS FROM THE CLIENTS

    // received successfully

    FDATK_DATA_PERCLI *data_percli = (FDATK_DATA_PERCLI *)data;
    LOG_INFO_("successfully received data of size %u from ", datalen);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    
    // check existing sender
    int sender_idx = -1;

    for (int i=0; i < fdatk_model_input->n_registered_cli; i++){
      if (uip_ipaddr_cmp(&(fdatk_model_input->sender[i]), sender_addr)){
        sender_idx = i;
        break;
      }
    }

    // LOG_INFO_("STEP3 sender_idx: %d, n_registered_cli: %d", sender_idx, fdatk_model_input->n_registered_cli);
    // LOG_INFO_("\n");
    // if new, add to the list
    if (sender_idx == -1){
      sender_idx = fdatk_model_input->n_registered_cli;
      fdatk_model_input->n_registered_cli++;
      uip_ipaddr_copy(&(fdatk_model_input->sender[sender_idx]), sender_addr);
    }

    // LOG_INFO_("STEP4 sender_idx: %d, n_registered_cli: %d", sender_idx, fdatk_model_input->n_registered_cli);
    // LOG_INFO_("\n");
    // copy data
    LOG_INFO("Data:");
    for (int i=0; i<FDATK_NCOL; i++){
      LOG_INFO_("%"PRIu32",", data_percli->data[i]);
      fdatk_model_input->data[sender_idx].data[i] = data_percli->data[i];
    }
    fdatk_model_input->has_new_data[sender_idx] = true;
    LOG_INFO_("\n");

    // check if all clients have sent data
    if (input_ready() == true){

      int inference_result = -1;
      run_inference(&inference_result);
      LOG_INFO("inference_result: %d\n", inference_result);

      // LOG DATA
      LOG_INFO_("FDATK_DATA:");
      for (int i=0; i<FDATK_NCLI; i++){
        for (int j=0; j<FDATK_NCOL; j++){
          LOG_INFO_("%"PRIu32",", fdatk_model_input->data[i].data[j]);
        }
      }
      LOG_INFO_("\n");

      // reset
      reset_model();
    }

    /* __Implementation by Yonsei */
      
  }
  else
  {
    // exception handling
    LOG_INFO_("unknown data of size %u from ", datalen);
  }
  LOG_INFO_(" from ");
  LOG_INFO_6ADDR(sender_addr);
  
  LOG_INFO_(", RSSI: (%d dBm)", rssi);

  LOG_INFO_("\n");
#if WITH_SERVER_REPLY
  /* send back the same string to the client as an echo reply */
  LOG_INFO("Sending response.\n");
  simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize ICMP6/RPL statistics */
  icmp6_stats_init();

  /* Implementation by Yonsei */
  /* Init flooding attack detection model*/
  init_model();
  /* __Implementation by Yonsei */

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  /* Hide the root attribute in Cooja */
  LOG_ANNOTATE("#A root\n");
  LOG_ANNOTATE("#A color=LIGHTGRAY\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
