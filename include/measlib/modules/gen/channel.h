/**
 * @file channel.h
 * @brief Signal Generator (GEN) Channel.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_MODULES_GEN_CHANNEL_H
#define MEASLIB_MODULES_GEN_CHANNEL_H

#include "measlib/core/channel.h"
#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include "measlib/dsp/node_types.h"

typedef struct {
  meas_channel_t base;

  meas_chain_t pipeline;

  meas_node_t node_wavegen;

  // Contexts
  meas_node_wavegen_ctx_t ctx_wavegen;

} meas_gen_channel_t;

meas_status_t meas_gen_channel_init(meas_gen_channel_t *ch);

#endif // MEASLIB_MODULES_GEN_CHANNEL_H
