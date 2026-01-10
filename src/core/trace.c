/**
 * @file trace.c
 * @brief Trace Data Accessor.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/trace.h"

// Wrapper Helper
meas_status_t meas_trace_get_data(meas_trace_t *t, const meas_real_t **x,
                                  const meas_real_t **y, size_t *count) {
  if (t && t->base.api) {
    // Safe cast to trace API - strictly speaking we should check type/magic
    const meas_trace_api_t *api = (const meas_trace_api_t *)t->base.api;
    if (api->get_data) {
      return api->get_data(t, x, y, count);
    }
  }
  return MEAS_ERROR;
}
