/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden AB.
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

#ifndef APP_MESSAGE_H_
#define APP_MESSAGE_H_

#include "contiki.h"

typedef struct {
  uint8_t seqno[4];
  uint8_t rpl_rank[2];
  uint8_t rpl_dag_version;
} app_message_t;

static inline uint16_t
app_read_uint16(const uint8_t *p)
{
  return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static inline void
app_write_uint16(uint8_t *p, uint16_t v)
{
  p[0] = (v >> 8) & 0xff;
  p[1] = v & 0xff;
}

static inline uint32_t
app_read_uint32(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24)
    | ((uint32_t)p[1] << 16)
    | ((uint32_t)p[2] << 8)
    | (uint32_t)p[3];
}

static inline void
app_write_uint32(uint8_t *p, uint32_t v)
{
  p[0] = (v >> 24) & 0xff;
  p[1] = (v >> 16) & 0xff;
  p[2] = (v >> 8) & 0xff;
  p[3] = v & 0xff;
}

static inline uint64_t
app_read_uint64(const uint8_t *p)
{
  return ((uint64_t)p[0] << 56)
    | ((uint64_t)p[1] << 48)
    | ((uint64_t)p[2] << 40)
    | ((uint64_t)p[3] << 32)
    | ((uint64_t)p[4] << 24)
    | ((uint64_t)p[5] << 16)
    | ((uint64_t)p[6] << 8)
    | (uint64_t)p[7];
}

static inline void
app_write_uint64(uint8_t *p, uint64_t v)
{
  p[0] = (v >> 56) & 0xff;
  p[1] = (v >> 48) & 0xff;
  p[2] = (v >> 40) & 0xff;
  p[3] = (v >> 32) & 0xff;
  p[4] = (v >> 24) & 0xff;
  p[5] = (v >> 16) & 0xff;
  p[6] = (v >> 8) & 0xff;
  p[7] = (v >> 0) & 0xff;
}

#endif /* APP_MESSAGE_H_ */
