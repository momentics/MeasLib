/**
 * @file channel.h
 * @brief Spectrum Analyzer (SA) Channel.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_MODULES_SA_CHANNEL_H
#define MEASLIB_MODULES_SA_CHANNEL_H

#include "measlib/core/channel.h"
#include "measlib/dsp/chain.h"
#include "measlib/dsp/node_types.h"

/**
 * @brief SA FSM States.
 */
typedef enum {
  SA_CH_STATE_IDLE,     ///< Inactive
  SA_CH_STATE_SETUP,    ///< Configuring Hardware (LO/Attenuator)
  SA_CH_STATE_ACQUIRE,  ///< Triggering Acquisition
  SA_CH_STATE_WAIT_DMA, ///< Waiting for Data Ready
  SA_CH_STATE_PROCESS,  ///< Processing DSP Pipeline
  SA_CH_STATE_NEXT      ///< Iterating Frequency/Loop
} meas_sa_state_t;

/**
 * @brief Spectrum Analyzer (SA) Channel Object.
 *
 * Manages the frequency sweep, data acquisition, and DSP chain processing
 * for spectrum analysis.
 */
typedef struct {
  meas_channel_t base; ///< Base Channel Object

  // Pipeline
  meas_chain_t pipeline; ///< Processing Chain Manager

  // Nodes
  meas_node_t node_window; ///< Windowing Node
  meas_node_t node_fft;    ///< FFT Node
  meas_node_t node_mag;    ///< Magnitude Node
  meas_node_t node_logmag; ///< Log-Magnitude Node
  meas_node_t node_sink;   ///< Trace Sink Node

  // Contexts
  meas_node_window_ctx_t ctx_window; ///< Window Context
  meas_node_fft_ctx_t ctx_fft;       ///< FFT Context
  // Mag/LogMag are stateless (void* ctx)
  meas_node_sink_ctx_t ctx_sink; ///< Sink Context

  // Config
  size_t fft_size;            ///< FFT Length (e.g. 1024)
  meas_trace_t *output_trace; ///< Output Trace Object

  // FSM State
  meas_sa_state_t state;       ///< Current FSM State
  volatile bool is_data_ready; ///< Data Ready Flag (ISR -> Main)

  // Sweep Parameters
  uint64_t start_freq_hz;   ///< Start Frequency (Hz)
  uint64_t stop_freq_hz;    ///< Stop Frequency (Hz)
  uint64_t current_freq_hz; ///< Current LO Frequency (Hz)

  // Hardware Abstraction
  void *hal_synth; ///< Pointer to Synthesizer Driver
  void *hal_rx;    ///< Pointer to Receiver Driver

} meas_sa_channel_t;

/**
 * @brief Initialize the SA Channel.
 *
 * @param ch Pointer to the channel structure to initialize.
 * @param trace Pointer to the trace object to write data to.
 * @return MEAS_OK if successful, error code otherwise.
 */
meas_status_t meas_sa_channel_init(meas_sa_channel_t *ch, meas_trace_t *trace);

#endif // MEASLIB_MODULES_SA_CHANNEL_H
