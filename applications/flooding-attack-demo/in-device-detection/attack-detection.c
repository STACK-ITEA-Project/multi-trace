/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden AB.
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

#include "contiki.h"
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#include "net/routing/rpl-lite/rpl-timers.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#endif
#include "attack-detection.h"
#include "icmp6-stats.h"

#include <math.h>
#include <stdlib.h>
#include "RFC_emlearn_15trees.h"

#include "sys/log.h"
#define LOG_MODULE "detect"
#define LOG_LEVEL LOG_LEVEL_INFO

#define n_features 4
#define size_max_per_chunk 100

#define detect_interval 600
#define detect_interval_cnt 61

#define rank_max 1000
#define dior_max 12
#define dios_max 3
#define daor_max 8

/* Variables for IDS */
static float features_input[n_features * 4];
static float features[size_max_per_chunk][n_features];
static float features_max[n_features];
static float features_min[n_features];
static uint16_t cnt_observations;
static uint16_t cnt_detect;
/*---------------------------------------------------------------------------*/
/* digits before point */
static unsigned short
d1(float f)
{
  if(f < 0) {
    f = -f;
  }
  return (signed short)f;
}
/*---------------------------------------------------------------------------*/
/* digits after point */
static unsigned short
d2(float f)
{
  if(f < 0) {
    f = -f;
  }
  return 1000 * (f - d1(f));
}
/*---------------------------------------------------------------------------*/
void
attack_detection_reset(void)
{
  cnt_observations = 0;
  memset(features_input, 0, sizeof(features_input));
  memset(features, 0, sizeof(features));

  /*
    memset(features_max, 0, sizeof(features_max));
    memset(features_min, 0, sizeof(features_min));

    for(int i = 0; i < n_features; i++) {
      features_min[i] = 1000.0;
    }
  */
}
/*---------------------------------------------------------------------------*/
bool
attack_detection_run(void)
{
  rpl_instance_t *default_instance = rpl_get_default_instance();
  uint16_t rank = default_instance ? default_instance->dag.rank : RPL_INFINITE_RANK;
  uint16_t i;
  uint16_t j;
  bool attack_detected = false;

  /* Intrusion detection */
  LOG_INFO("Detection run: %u\n", cnt_detect);
  // Record observations
  features[cnt_observations][0] = rank * 1.0;
  /*features[cnt_observations][1] = (icmp6_stats.dis_uc_recv + icmp6_stats.dis_mc_recv)*1.0;
    features[cnt_observations][2] = (icmp6_stats.dis_uc_sent + icmp6_stats.dis_mc_sent)*1.0;*/
  features[cnt_observations][1] = (icmp6_stats.dio_uc_recv + icmp6_stats.dio_mc_recv)*1.0;
  features[cnt_observations][2] = (icmp6_stats.dio_uc_sent + icmp6_stats.dio_mc_sent)*1.0;
  features[cnt_observations][3] = icmp6_stats.dao_recv*1.0;

  // Update the max and min value
  /*
    for (i=0; i<n_features; i++){
      if (features[cnt_observations][i] < features_min[i]) {
        features_min[i] = features[cnt_observations][i];
      }
      if (features[cnt_observations][i] > features_max[i]) {
        features_max[i] = features[cnt_observations][i];
      }
    }
  */
  cnt_observations++;

  if(cnt_observations == detect_interval_cnt) {
    LOG_INFO("Start IDS\n");
    LOG_INFO_("Number of data items: %u\n", cnt_observations);

    // Transform the accumulated attributes, i.e., features[j][i](new) = features[j][i] - features[j-1][i]
    // Only applied for the number of messages, excluding the rank
    for (i=n_features-1; i>0; i--){
      for (j=cnt_observations-1; j>1; j--){
        features[j][i] = features[j][i] - features[j-1][i];
      }
      features[0][i] = 0.0;
      features[1][i] = 0.0;
    }

    // Normalize to [-1, 1]
    for (i=0; i<n_features; i++){
      for (j=0; j<cnt_observations; j++){
        features[j][i] = (features[j][i] - features_min[i]) * (1.0-(-1.0)) / (features_max[i] - features_min[i]) + (-1.0);
      }
    }

    // Run IDS in the end
    uint16_t m = 0;
    uint16_t k = 0;
    for (m=2; m<6; m++){ // chunk: 0 1 [2 3 4 5]
      for (i=0; i<n_features; i++){
        for (j=m*10+1; j<m*10+10+1 && j<cnt_observations; j++){
          if (features[j][i] <= 1.0 && features[j][i] >= -1.0){
            features_input[(m-2)*n_features+i] += features[j][i];
            k++;
          }
        }
        if (k > 0){
          features_input[(m-2)*n_features+i] /= (k*1.0);
        }
        k = 0;
      }
    }
    LOG_INFO("Running prediction!\n");
    int32_t prediction_result_rfc = RFC_emlearn_predict(features_input, n_features * 4);
    if(prediction_result_rfc) {
      LOG_INFO("ATTACK DETECTED!\n");
      attack_detected = true;
    } else {
      LOG_INFO("No attack detected!\n");
    }

    LOG_INFO("Intrusion detection result using RFC is: %" PRId32 "\n", prediction_result_rfc);
    for (i=0; i<n_features*4; i++){
      if (features_input[i] < 0) {
        LOG_INFO_("features[%u]: -%u.%u\n", i, d1(features_input[i]),d2(features_input[i]));
      } else {
        LOG_INFO_("features[%u]: %u.%u\n", i, d1(features_input[i]),d2(features_input[i]));
      }
    }

    for (i=0; i<n_features; i++){
      LOG_INFO_("features_max[%u]: %u.%u\n", i, d1(features_max[i]),d2(features_max[i]));
      LOG_INFO_("features_min[%u]: %u.%u\n", i, d1(features_min[i]),d2(features_min[i]));
    }

    // Reset variables
    attack_detection_reset();

    cnt_detect++;
  }
  return attack_detected;
}
/*---------------------------------------------------------------------------*/
void
attack_detection_init(void)
{
  memset(features_input, 0, sizeof(features_input));
  memset(features, 0, sizeof(features));
  memset(features_max, 0, sizeof(features_max));
  memset(features_min, 0, sizeof(features_min));

  for(int i = 0; i < n_features; i++){
    features_min[i] = 1000.0;
  }

  cnt_observations = 0;
  cnt_detect = 1;

  // give the max and min for data normalization
  features_min[0] = 256.0;
  for(int i = 1; i < n_features; i++){
    features_min[i] = 0.0;
  }

  features_max[0] = rank_max;
  features_max[1] = dior_max;
  features_max[2] = dios_max;
  features_max[3] = daor_max;
}
/*---------------------------------------------------------------------------*/
