/**
 * @file channel.c
 * @brief Spectrum Analyzer (SA) Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/modules/sa/channel.h"
#include <stddef.h>

static meas_status_t sa_configure(meas_channel_t *base_ch) {
  (void)base_ch;
  // Boilerplate for VTable
  return MEAS_OK;
}

static meas_status_t sa_start(meas_channel_t *base_ch) {
  (void)base_ch;
  return MEAS_OK;
}

static meas_channel_api_t sa_api = {
    .configure = sa_configure,
    .start_sweep = sa_start,
    // ... others NULL for scaffolding
};

meas_status_t meas_sa_channel_init(meas_sa_channel_t *ch, meas_trace_t *trace) {
  if (!ch || !trace)
    return MEAS_ERROR;

  ch->base.base.api = (const meas_object_api_t *)&sa_api;
  ch->output_trace = trace;
  ch->fft_size = 1024;

  // -- Build Pipeline --
  meas_chain_init(&ch->pipeline);

  // 1. Window
  meas_node_window_init(&ch->node_window, &ch->ctx_window, DSP_WINDOW_HANN);

  // 2. FFT
  meas_node_fft_init(&ch->node_fft, &ch->ctx_fft, ch->fft_size, false);

  // 3. Magnitude
  meas_node_mag_init(&ch->node_mag, NULL);

  // 4. LogMag (dB)
  meas_node_logmag_init(&ch->node_logmag, NULL);

  // 5. Sink
  meas_node_sink_trace_init(&ch->node_sink, &ch->ctx_sink, trace);

  // Link
  ch->pipeline.api->append(&ch->pipeline, &ch->node_window);
  ch->pipeline.api->append(&ch->pipeline, &ch->node_fft);
  ch->pipeline.api->append(&ch->pipeline, &ch->node_mag);
  ch->pipeline.api->append(&ch->pipeline, &ch->node_logmag);
  ch->pipeline.api->append(&ch->pipeline, &ch->node_sink);

  return MEAS_OK;
}
