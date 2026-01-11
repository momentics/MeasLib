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
#include <string.h>

// -- Private Event Handler --

// -- VTable Forward Declarations --
static meas_status_t vna_set_prop(meas_object_t *obj, meas_id_t key,
                                  meas_variant_t val);
static meas_status_t vna_get_prop(meas_object_t *obj, meas_id_t key,
                                  meas_variant_t *val);
static const char *vna_get_name(meas_object_t *obj);

// -- Private Event Handler --

static void vna_on_event(const meas_event_t *ev, void *ctx) {
  if (!ev || !ctx)
    return;
  meas_vna_channel_t *ch = (meas_vna_channel_t *)ctx;

  // Check if this event matters to us
  if (ev->type == EVENT_DATA_READY) {
    // If the driver provided a buffer pointer, we should use it.
    if (ev->payload.type == PROP_TYPE_PTR && ev->payload.p_val != NULL) {
      // Driver provided Zero-Copy buffer
      ch->active_buffer = ev->payload.p_val;
    } else {
      // Fallback: Use our user-provided buffer
      ch->active_buffer = ch->user_buffer;
    }
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
    ch->active_buffer = NULL; // Reset pointer
    if (rx && rx->start) {
      // Pass the user buffer (if any). If NULL, driver MUST push data via event
      // or error out.
      rx->start(NULL, ch->user_buffer, ch->points * sizeof(meas_complex_t));
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
      meas_data_block_t block = {
          .source_id = 0, // TODO: Set ID
          .sequence = ch->current_point,
          .size = ch->points * sizeof(meas_complex_t), // Actual utilized size
          // Prefer active_buffer (from event), fallback to user_buffer
          .data = ch->active_buffer ? ch->active_buffer : ch->user_buffer};

      if (ch->pipeline.api && ch->pipeline.api->run) {
        // Only run if we actually have data
        if (block.data != NULL) {
          ch->pipeline.api->run(&ch->pipeline, &block);
        }
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
      // Linear Sweep: Start + (k * Step)
      if (ch->points > 1) {
        meas_real_t step = (meas_real_t)(ch->stop_freq_hz - ch->start_freq_hz) /
                           (ch->points - 1);
        ch->current_freq_hz =
            ch->start_freq_hz + (uint32_t)(ch->current_point * step);
      } else {
        ch->current_freq_hz = ch->start_freq_hz;
      }

      ch->state = VNA_CH_STATE_SETUP; // Loop
    }
    break;
  }
}

// -- VTable Implementation --

static meas_status_t vna_configure(meas_channel_t *base_ch) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)base_ch;
  // Apply defaults if not set
  if (ch->points == 0)
    ch->points = VNA_MAX_POINTS;
  if (ch->start_freq_hz == 0 || ch->start_freq_hz < VNA_MIN_FREQ)
    ch->start_freq_hz = VNA_MIN_FREQ;
  if (ch->stop_freq_hz == 0 || ch->stop_freq_hz > VNA_MAX_FREQ)
    ch->stop_freq_hz = VNA_MAX_FREQ;

  // Subscribe to HAL events (In a real app, 'NULL' would be replaced by
  // specific HAL driver instances)
  meas_subscribe(NULL, vna_on_event, ch);

  // -- Pipeline Initialization --
  meas_chain_init(&ch->pipeline);

  // 1. Init DDC Node
  meas_node_ddc_init(&ch->node_ddc, &ch->ctx_ddc);
  // DDC Context is initialized by init, table ptr set.

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

  // Final Sanity Check: If Start > Stop, swap them or error?
  // For configure, we will force Stop >= Start to ensure a valid state.
  if (ch->start_freq_hz > ch->stop_freq_hz) {
    // Auto-correct: Move Stop to Start (CW Mode default)
    ch->stop_freq_hz = ch->start_freq_hz;
  }

  // VALIDATION: Check Buffer Capacity
  // If we have a user buffer, points must fit.
  if (ch->user_buffer != NULL) {
    if (ch->points > ch->user_buffer_cap) {
      // Error: Buffer too small for requested points
      return MEAS_ERROR;
    }
  } else {
    // No user buffer. Be permissive, assuming driver will Push via Event.
    // But warn if Points > MAX_LOGICAL (if we care).
    // For now, allow it.
  }

  return MEAS_OK;
}

static meas_status_t vna_start_sweep(meas_channel_t *base_ch) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)base_ch;

  // Validation: Cannot run if params are invalid
  if (ch->start_freq_hz > ch->stop_freq_hz) {
    return MEAS_ERROR;
  }
  // Check if points are valid logical range
  if (ch->points == 0 || ch->points > VNA_MAX_POINTS) {
    return MEAS_ERROR;
  }
  // Check memory safety
  if (ch->user_buffer != NULL && ch->points > ch->user_buffer_cap) {
    return MEAS_ERROR;
  }

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

// -- Property Accessors --

static const char *vna_get_name(meas_object_t *obj) {
  (void)obj;
  return "VNA_Channel";
}

static meas_status_t vna_set_prop(meas_object_t *obj, meas_id_t key,
                                  meas_variant_t val) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)obj;
  switch (key) {
  case MEAS_PROP_VNA_START_FREQ:
    if (val.type == PROP_TYPE_INT64) {
      if (val.i_val < VNA_MIN_FREQ || val.i_val > VNA_MAX_FREQ)
        return MEAS_ERROR;
      ch->start_freq_hz = (uint64_t)val.i_val;
    } else if (val.type == PROP_TYPE_REAL) {
      if (val.r_val < VNA_MIN_FREQ || val.r_val > VNA_MAX_FREQ)
        return MEAS_ERROR;
      ch->start_freq_hz = (uint64_t)val.r_val;
    } else
      return MEAS_ERROR;
    return MEAS_OK;

  case MEAS_PROP_VNA_STOP_FREQ:
    if (val.type == PROP_TYPE_INT64) {
      if (val.i_val < VNA_MIN_FREQ || val.i_val > VNA_MAX_FREQ)
        return MEAS_ERROR;
      ch->stop_freq_hz = (uint64_t)val.i_val;
    } else if (val.type == PROP_TYPE_REAL) {
      if (val.r_val < VNA_MIN_FREQ || val.r_val > VNA_MAX_FREQ)
        return MEAS_ERROR;
      ch->stop_freq_hz = (uint64_t)val.r_val;
    } else
      return MEAS_ERROR;
    return MEAS_OK;

  case MEAS_PROP_VNA_POINTS:
    if (val.type == PROP_TYPE_INT64) {
      if (val.i_val <= 0 || val.i_val > VNA_MAX_POINTS)
        return MEAS_ERROR;
      // If buffer is already set, validate immediately
      if (ch->user_buffer != NULL && (size_t)val.i_val > ch->user_buffer_cap)
        return MEAS_ERROR;
      ch->points = (uint32_t)val.i_val;
    } else
      return MEAS_ERROR;
    return MEAS_OK;

  case MEAS_PROP_VNA_BUFFER_PTR:
    if (val.type == PROP_TYPE_PTR) {
      ch->user_buffer = val.p_val;
    } else
      return MEAS_ERROR;
    return MEAS_OK;

  case MEAS_PROP_VNA_BUFFER_CAP:
    if (val.type == PROP_TYPE_INT64) {
      if (val.i_val < 0)
        return MEAS_ERROR;
      ch->user_buffer_cap = (size_t)val.i_val;
    } else
      return MEAS_ERROR;
    return MEAS_OK;

  default:
    return MEAS_ERROR;
  }
}

static meas_status_t vna_get_prop(meas_object_t *obj, meas_id_t key,
                                  meas_variant_t *val) {
  meas_vna_channel_t *ch = (meas_vna_channel_t *)obj;
  switch (key) {
  case MEAS_PROP_VNA_START_FREQ:
    val->type = PROP_TYPE_INT64;
    val->i_val = ch->start_freq_hz;
    return MEAS_OK;

  case MEAS_PROP_VNA_STOP_FREQ:
    val->type = PROP_TYPE_INT64;
    val->i_val = ch->stop_freq_hz;
    return MEAS_OK;

  case MEAS_PROP_VNA_POINTS:
    val->type = PROP_TYPE_INT64;
    val->i_val = ch->points;
    return MEAS_OK;

  case MEAS_PROP_VNA_BUFFER_PTR:
    val->type = PROP_TYPE_PTR;
    val->p_val = ch->user_buffer;
    return MEAS_OK;

  case MEAS_PROP_VNA_BUFFER_CAP:
    val->type = PROP_TYPE_INT64;
    val->i_val = (int64_t)ch->user_buffer_cap;
    return MEAS_OK;

  default:
    return MEAS_ERROR;
  }
}

// Global VTable Instance for VNA Channels
const meas_channel_api_t vna_channel_api = {.base =
                                                {
                                                    .get_name = vna_get_name,
                                                    .set_prop = vna_set_prop,
                                                    .get_prop = vna_get_prop,
                                                    .destroy = NULL,
                                                },
                                            .configure = vna_configure,
                                            .start_sweep = vna_start_sweep,
                                            .abort_sweep = vna_abort_sweep,
                                            .tick = vna_fsm_tick};

// ============================================================================
// Public Initializer
// ============================================================================

meas_status_t meas_vna_channel_init(meas_vna_channel_t *ch,
                                    meas_trace_t *trace) {
  if (!ch || !trace) {
    return MEAS_ERROR;
  }

  // 1. Zero out context
  memset(ch, 0, sizeof(meas_vna_channel_t));

  // 2. Set VTable
  ch->base.base.api = (const meas_object_api_t *)&vna_channel_api;

  // 3. Set Defaults
  ch->state = VNA_CH_STATE_IDLE;
  ch->start_freq_hz = VNA_MIN_FREQ;
  ch->stop_freq_hz = VNA_MAX_FREQ;
  ch->points = VNA_MAX_POINTS;
  ch->output_trace = trace;

  // 4. Initialize Data Ready Flag
  ch->is_data_ready = false;

  return MEAS_OK;
}
