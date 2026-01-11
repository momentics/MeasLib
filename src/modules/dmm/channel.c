/**
 * @file channel.c
 * @brief Digital Multimeter (DMM) Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/modules/dmm/channel.h"

/**
 * @note TEMPORARY STUB / PLACEHOLDER
 * Current development priorities are focused entirely on the VNA module.
 * This implementation is non-functional and serves only as an architectural
 * reference.
 *
 * TODO: Implement full DMM business logic after VNA stabilization.
 */

static meas_status_t dmm_configure(meas_channel_t *base_ch) {
  meas_dmm_channel_t *ch = (meas_dmm_channel_t *)base_ch;
  if (!ch)
    return MEAS_ERROR;

  // Re-initialize pipeline or nodes if needed
  // For now, static init is sufficient
  return MEAS_OK;
}

static meas_channel_api_t dmm_api = {
    .configure = dmm_configure,
    // ... other methods ...
};

meas_status_t meas_dmm_channel_init(meas_dmm_channel_t *ch) {
  if (!ch)
    return MEAS_ERROR;

  // 1. Init Base
  // ch->base initialization would technically happen via device factory,
  // but here we setup the specific API overrides.
  ch->base.base.api = (const meas_object_api_t *)&dmm_api;

  // 2. Init Pipeline
  meas_chain_init(&ch->pipeline);

  // 3. Init Nodes
  // Linear: Slope=1.0, Intercept=0.0 (Calibration would update these later)
  meas_node_linear_init(&ch->node_linear, &ch->ctx_linear, 1.0f, 0.0f);

  // Sink
  meas_node_sink_trace_init(&ch->node_sink, &ch->ctx_sink, ch->output_trace);

  // 4. Build Chain
  // Input (from Driver) -> Linear -> Sink -> Trace
  const meas_chain_api_t *api = ch->pipeline.api;
  api->append(&ch->pipeline, &ch->node_linear);
  api->append(&ch->pipeline, &ch->node_sink);

  return MEAS_OK;
}

meas_status_t meas_dmm_get_reading(meas_dmm_channel_t *ch, meas_real_t *avg,
                                   meas_real_t *min_val, meas_real_t *max_val) {
  if (!ch || !ch->output_trace) {
    return MEAS_ERROR;
  }
  (void)avg;
  (void)min_val;
  (void)max_val;

  // Get data from trace
  // const meas_real_t *y_data = NULL;
  // size_t count = 0;
  // Assuming trace API allows direct access (or we add a helper)
  // For now, we assume standard trace_get_data
  // meas_trace_api_t *t_api = (meas_trace_api_t *)ch->output_trace->base.api;
  // t_api->get_data(ch->output_trace, NULL, &y_data, &count);

  // If trace isn't ready, we can't compute.
  // This is a placeholder for the actual trace integration.

  return MEAS_OK;
}
