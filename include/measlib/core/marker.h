/**
 * @file marker.h
 * @brief Analysis Tools (Markers).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the `meas_marker_t` object which attaches to a Trace to provide
 * specific data point readouts.
 */

#ifndef MEASLIB_CORE_MARKER_H
#define MEASLIB_CORE_MARKER_H

#include "measlib/core/trace.h"

/**
 * @brief Marker Object Handle
 */
typedef struct {
  meas_object_t base;
} meas_marker_t;

/**
 * @brief Marker API (VTable)
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Attach marker to a source trace.
   * @param m Marker instance.
   * @param trace Target Trace instance.
   * @return meas_status_t
   */
  meas_status_t (*set_source)(meas_marker_t *m, meas_trace_t *trace);

  /**
   * @brief Move marker to a specific data index.
   * @param m Marker instance.
   * @param index Data point index (0 to N-1).
   * @return meas_status_t
   */
  meas_status_t (*set_index)(meas_marker_t *m, size_t index);

  /**
   * @brief Read the complex value at the marker position.
   * @param m Marker instance.
   * @param val Output pointer for value.
   * @return meas_status_t
   */
  meas_status_t (*get_value)(meas_marker_t *m, meas_complex_t *val);

  /**
   * @brief Read the stimulus value (Frequency/Time) at the marker position.
   * @param m Marker instance.
   * @param freq Output pointer for stimulus value.
   * @return meas_status_t
   */
  meas_status_t (*get_stimulus)(meas_marker_t *m, meas_real_t *freq);

} meas_marker_api_t;

#endif // MEASLIB_CORE_MARKER_H
