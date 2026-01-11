/**
 * @file channel.c
 * @brief Spectrum Analyzer (SA) Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/modules/sa/channel.h"
#include "measlib/core/event.h"
#include <stddef.h>

// -- Private Event Handler --

/**
 * @brief Event Callback.
 *
 * Handles generic system events. We listen for EVENT_DATA_READY to synchronize
 * with the hardware acquisition process.
 */
static void sa_on_event(const meas_event_t *ev, void *ctx) {
  if (!ev || !ctx)
    return;
  meas_sa_channel_t *ch = (meas_sa_channel_t *)ctx;

  if (ev->type == EVENT_DATA_READY) {
    ch->is_data_ready = true;
  }
}

// -- Private FSM Implementation --

/**
 * @brief FSM Tick Function.
 *
 * Called periodically by the main loop. Manages the state transitions
 * of the spectrum analyzer sweep logic.
 *
 * @param base_ch The generic channel pointer.
 */
static void sa_fsm_tick(meas_channel_t *base_ch) {
  meas_sa_channel_t *ch = (meas_sa_channel_t *)base_ch;

  // TODO: Add HAL casts when drivers are available
  // meas_hal_synth_api_t *synth = (meas_hal_synth_api_t *)ch->hal_synth;
  // meas_hal_rx_api_t *rx = (meas_hal_rx_api_t *)ch->hal_rx;

  switch (ch->state) {
  case SA_CH_STATE_IDLE:
    break;

  case SA_CH_STATE_SETUP:
    // 1. Set LO Frequency
    // if (synth) synth->set_freq(ch->current_freq_hz);

    // 2. Ready to acquire
    ch->state = SA_CH_STATE_ACQUIRE;
    break;

  case SA_CH_STATE_ACQUIRE:
    ch->is_data_ready = false;
    // 1. Start RX DMA
    // if (rx) rx->start(buffer, ch->fft_size);

    // For simulation/mock:
    ch->is_data_ready = true; // Immediate ready in mock

    ch->state = SA_CH_STATE_WAIT_DMA;
    break;

  case SA_CH_STATE_WAIT_DMA:
    if (ch->is_data_ready) {
      ch->state = SA_CH_STATE_PROCESS;
    }
    break;

  case SA_CH_STATE_PROCESS: {
    // Mock Data Block
    static meas_complex_t mock_buffer[1024];
    meas_data_block_t block = {.source_id = 0,
                               .sequence = 0,
                               .size = ch->fft_size * sizeof(meas_complex_t),
                               .data = mock_buffer};

    if (ch->pipeline.api && ch->pipeline.api->run) {
      ch->pipeline.api->run(&ch->pipeline, &block);
    }
  }

    // Notify Data Ready
    {
      meas_event_t ev = {.type = EVENT_DATA_READY,
                         .source = (meas_object_t *)base_ch};
      meas_event_publish(ev);
    }

    ch->state = SA_CH_STATE_NEXT;
    break;

  case SA_CH_STATE_NEXT:
    // Continuous mode: go back to setup or acquire
    // Logic for sweep could be added here
    ch->state = SA_CH_STATE_SETUP;
    break;
  }
}

/**
 * @brief Configure the Channel Hardware.
 * Applies the current settings (Frequency, Span, RBW) to the underlying
 * drivers.
 */
static meas_status_t sa_configure(meas_channel_t *base_ch) {
  meas_sa_channel_t *ch = (meas_sa_channel_t *)base_ch;

  ch->start_freq_hz = 1000000;
  ch->stop_freq_hz = 100000000;
  ch->current_freq_hz = ch->start_freq_hz;

  meas_subscribe(NULL, sa_on_event, ch);

  return MEAS_OK;
}

/**
 * @brief Start the Sweep.
 * Resets the FSM to the initial state.
 */
static meas_status_t sa_start(meas_channel_t *base_ch) {
  meas_sa_channel_t *ch = (meas_sa_channel_t *)base_ch;
  ch->state = SA_CH_STATE_SETUP;
  return MEAS_OK;
}

/**
 * @brief Abort the Sweep.
 * Stops any ongoing operation and returns to IDLE.
 */
static meas_status_t sa_abort(meas_channel_t *base_ch) {
  meas_sa_channel_t *ch = (meas_sa_channel_t *)base_ch;
  ch->state = SA_CH_STATE_IDLE;
  return MEAS_OK;
}

static meas_channel_api_t sa_api = {
    .configure = sa_configure,
    .start_sweep = sa_start,
    .abort_sweep = sa_abort,
    .tick = sa_fsm_tick,
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
