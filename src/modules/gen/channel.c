/**
 * @file channel.c
 * @brief Signal Generator (GEN) Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/modules/gen/channel.h"

/**
 * @note TEMPORARY STUB / PLACEHOLDER
 * Current development priorities are focused entirely on the VNA module.
 * This implementation is non-functional and serves only as an architectural
 * reference.
 *
 * TODO: Implement full Signal Generator business logic after VNA stabilization.
 */

static meas_status_t gen_configure(meas_channel_t *base_ch) {
  (void)base_ch;
  return MEAS_OK;
}

static meas_channel_api_t gen_api = {
    .configure = gen_configure,
    // ...
};

meas_status_t meas_gen_channel_init(meas_gen_channel_t *ch) {
  if (!ch)
    return MEAS_ERROR;

  ch->base.base.api = (const meas_object_api_t *)&gen_api;

  meas_chain_init(&ch->pipeline);

  meas_node_wavegen_init(&ch->node_wavegen, &ch->ctx_wavegen, 1000.0f, 48000.0f,
                         DSP_WAVE_SINE);

  ch->pipeline.api->append(&ch->pipeline, &ch->node_wavegen);

  return MEAS_OK;
}
