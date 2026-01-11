/**
 * @file trace.h
 * @brief Measurement Data Container.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the `meas_trace_t` object which holds measurement data (e.g.
 * S-parameters) and provides accessors for the UI and Math layers. Supports
 * zero-copy access.
 */

#ifndef MEASLIB_CORE_TRACE_H
#define MEASLIB_CORE_TRACE_H

#include "measlib/core/object.h"

/**
 * @brief Trace Data Formats
 * Specifies how the underlying data buffer is interpreted (Complex vs
 * Magnitude).
 */
typedef enum {
  TRACE_FMT_COMPLEX, /**< Array of meas_complex_t (Interleaved IQ) */
  TRACE_FMT_REAL     /**< Array of meas_real_t (Magnitude only) */
} meas_trace_fmt_t;

/**
 * @brief Trace Object Handle
 */
typedef struct {
  meas_object_t base;
} meas_trace_t;

/**
 * @brief Trace API (VTable)
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Access data buffers directly (Zero-Copy).
   *
   * @param t Trace instance.
   * @param x Output pointer to X-axis buffer (Stimulus/Frequency).
   * @param y Output pointer to Y-axis buffer (Response).
   * @param count Output variable for number of points.
   * @return meas_status_t
   */
  meas_status_t (*get_data)(meas_trace_t *t, const meas_real_t **x,
                            const meas_real_t **y, size_t *count);

  /**
   * @brief Set the data format.
   * @param t Trace instance.
   * @param fmt Desired format.
   * @return meas_status_t
   */

  /**
   * @brief Copy data into the trace buffer.
   * @param t Trace instance.
   * @param data Source buffer.
   * @param size Size of data in bytes.
   * @return meas_status_t
   */
  meas_status_t (*copy_data)(meas_trace_t *t, const void *data, size_t size);

} meas_trace_api_t;

/**
 * @brief Helper: Copy data into trace.
 */
meas_status_t meas_trace_copy_data(meas_trace_t *t, const void *data,
                                   size_t size);

#endif // MEASLIB_CORE_TRACE_H
