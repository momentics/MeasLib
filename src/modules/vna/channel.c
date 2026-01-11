/**
 * @file channel.c
 * @brief VNA Channel Implementation (FSM).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/modules/vna/channel.h"
#include "measlib/core/event.h"
#include "measlib/drivers/hal.h"
#include <stddef.h>

// -- Private Event Handler --

static void vna_on_event(const meas_event_t *ev, void *ctx) {
  if (!ev || !ctx)
    return;
  meas_vna_channel_t *ch = (meas_vna_channel_t *)ctx;

  // Check if this event matters to us
  if (ev->type == EVENT_DATA_READY) {
    // In real driver, check if source == ch->hal_rx
    ch->is_data_ready = true;
  }
}

// -- Private FSM Implementation --

static void vna_fsm_tick(meas_channel_t *base_ch) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)base_ch;
  // Helper to access HAL drivers (casted from void*)
  meas_hal_synth_api_t *synth = (meas_hal_synth_api_t *)ch->hal_synth;
  meas_hal_rx_api_t *rx = (meas_hal_rx_api_t *)ch->hal_rx;

  switch (ch->state) {
  case VNA_CH_STATE_IDLE:
    // Waiting for User Init
    break;

  case VNA_CH_STATE_SETUP:
    // 1. Set Frequency
    if (synth && synth->set_freq) {
      synth->set_freq(NULL, (meas_real_t)ch->current_freq_hz);
    }
    // 2. Wait for PLL Lock
    // In real driver: check lock status or set timer
    ch->state = VNA_CH_STATE_ACQUIRE;
    break;

  case VNA_CH_STATE_ACQUIRE:
    // 1. Trigger DMA
    ch->is_data_ready = false;
    if (rx && rx->start) {
      rx->start(NULL, NULL, 1024); // Dummy buffer
    }
    // 2. Transition to Wait
    ch->state = VNA_CH_STATE_WAIT_DMA;
    break;

  case VNA_CH_STATE_WAIT_DMA:
    // Poll the flag (Message Passing)
    if (ch->is_data_ready) {
      ch->state = VNA_CH_STATE_PROCESS;
    }
    // else remain in this state (Next tick check)
    break;

  case VNA_CH_STATE_PROCESS:
    // 1. Process Data using Pipeline
    {
      // Assume RX buffer is accessible here.
      // In a real generic system, we'd have a pointer to the buffer from the
      // Event or RX Handle. For now, we mock a data block pointing to a static
      // buffer or similar. Assuming RX HAL wrote to 'ch->rx_buffer' (implied)
      static meas_complex_t dummy_buffer[1024]; // Temp placeholder for demo

      meas_data_block_t block = {
          .source_id = 0, // TODO: Set ID
          .sequence = ch->current_point,
          .size = 1024 * sizeof(meas_complex_t),
          .data = dummy_buffer // In real data plane, this comes from DMA
      };

      if (ch->pipeline.api && ch->pipeline.api->run) {
        ch->pipeline.api->run(&ch->pipeline, &block);
      }
    }

    // 2. Publish Data Ready Event (to UI/Storage)
    {
      meas_event_t ev = {.type = EVENT_DATA_READY,
                         .source = (meas_object_t *)base_ch,
                         .payload = {0}};
      meas_event_publish(ev);
    }

    // 3. Next Point
    ch->state = VNA_CH_STATE_NEXT;
    break;

  case VNA_CH_STATE_NEXT:
    ch->current_point++;
    if (ch->current_point >= ch->points) {
      // Sweep Complete
      ch->state = VNA_CH_STATE_IDLE;
      meas_event_t ev = {.type = EVENT_STATE_CHANGED,
                         .source = (meas_object_t *)base_ch};
      meas_event_publish(ev);
    } else {
      // Calc Next Freq
      // Calc Next Freq
      // Linear Sweep: Start + (k * Step)
      meas_real_t step = (meas_real_t)(ch->stop_freq_hz - ch->start_freq_hz) /
                         (ch->points - 1);
      ch->current_freq_hz =
          ch->start_freq_hz + (uint32_t)(ch->current_point * step);

      ch->state = VNA_CH_STATE_SETUP; // Loop
    }
    break;
  }
}

// -- VTable Implementation --

static meas_status_t vna_configure(meas_channel_t *base_ch) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)base_ch;
  // Apply properties to internal struct
  ch->points = 101; // Default
  ch->start_freq_hz = 1000000;
  ch->stop_freq_hz = 30000000;

  // Subscribe to HAL events (In a real app, 'NULL' would be replaced by
  // specific HAL driver instances)
  meas_subscribe(NULL, vna_on_event, ch);

  // -- Pipeline Initialization --
  meas_chain_init(&ch->pipeline);

  // 1. Init DDC Node
  meas_node_ddc_init(&ch->node_ddc, &ch->ctx_ddc);

  meas_node_sparam_init(&ch->node_sparam, &ch->ctx_sparam);

  // 3. Init Cal Node
  meas_node_cal_init(&ch->node_cal, &ch->ctx_cal, ch->active_cal);

  // 4. Init Sink Node
  if (ch->output_trace) {
    meas_node_sink_trace_init(&ch->node_sink, &ch->ctx_sink, ch->output_trace);
  }

  // 5. Build Chain: DDC -> SParam -> Cal -> Sink
  ch->pipeline.api->append(&ch->pipeline, &ch->node_ddc);
  ch->pipeline.api->append(&ch->pipeline, &ch->node_sparam);
  ch->pipeline.api->append(&ch->pipeline, &ch->node_cal);
  if (ch->output_trace) {
    ch->pipeline.api->append(&ch->pipeline, &ch->node_sink);
  }

  return MEAS_OK;
}

static meas_status_t vna_start_sweep(meas_channel_t *base_ch) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)base_ch;
  ch->current_point = 0;
  ch->current_freq_hz = ch->start_freq_hz;
  ch->state = VNA_CH_STATE_SETUP;
  return MEAS_OK;
}

static meas_status_t vna_abort_sweep(meas_channel_t *base_ch) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)base_ch;
  ch->state = VNA_CH_STATE_IDLE;
  return MEAS_OK;
}

// Global VTable Instance for VNA Channels
const meas_channel_api_t vna_channel_api = {.base = {.get_name = NULL,
                                                     .set_prop = NULL,
                                                     .get_prop = NULL,
                                                     .destroy = NULL},
                                            .configure = vna_configure,
                                            .start_sweep = vna_start_sweep,
                                            .abort_sweep = vna_abort_sweep,
                                            .tick = vna_fsm_tick};
