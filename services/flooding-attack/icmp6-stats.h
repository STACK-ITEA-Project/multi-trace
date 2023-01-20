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

#ifndef ICMP6_STATS_H_
#define ICMP6_STATS_H_

#include "contiki.h"

struct icmp6_stats {
  uint32_t dis_uc_recv;
  uint32_t dis_mc_recv;
  uint32_t dis_uc_sent;
  uint32_t dis_mc_sent;
  uint32_t dio_uc_recv;
  uint32_t dio_mc_recv;
  uint32_t dio_uc_sent;
  uint32_t dio_mc_sent;
  uint32_t dao_recv;
  uint32_t dao_sent;

  uint32_t dao_ack_recv;
  uint32_t dao_ack_sent;
  uint32_t rpl_total_sent;
  uint32_t rpl_total_recv;
  uint32_t total_sent;
  uint32_t total_recv;
};

extern struct icmp6_stats icmp6_stats;
extern bool flooding_attack_drop_dio;

void icmp6_stats_init(void);

#endif /* ICMP6_STATS_H_ */
