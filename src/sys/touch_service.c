/**
 * @file touch_service.c
 * @brief Touch Input Service Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/touch_service.h"
#include "measlib/core/event.h"
#include <stddef.h>

static struct {
  const meas_hal_touch_api_t *api;
  void *ctx;
  bool was_pressed;
} touch_srv;

void meas_touch_service_init(const meas_hal_touch_api_t *api, void *ctx) {
  touch_srv.api = api;
  touch_srv.ctx = ctx;
  touch_srv.was_pressed = false;
}

void meas_touch_service_poll(void) {
  if (!touch_srv.api || !touch_srv.api->read_point)
    return;

  meas_point_t pt = {0};
  // read_point returns MEAS_OK if touched/valid
  meas_status_t status = touch_srv.api->read_point(touch_srv.ctx, &pt);

  if (status == MEAS_OK) {
    // Touch Detected
    // Pack coordinates: X (low 16), Y (high 16)
    // Using int64 i_val
    int64_t payload = ((int64_t)pt.y << 16) | (pt.x & 0xFFFF);

    meas_event_t ev = {.type = EVENT_INPUT_TOUCH,
                       .source = NULL,
                       .payload = {.i_val = payload}};
    meas_event_publish(ev);
    touch_srv.was_pressed = true;
  } else {
    // Release (Optional: Dispatch Release event?)
    // For now, UI handles Move/Down. Release inferred by absence or specific
    // event? If we want drag, we need continuous events. If status != MEAS_OK,
    // it means no touch.
    if (touch_srv.was_pressed) {
      // Send special "Release" coordinate? Or separate event type?
      // current event type is generic.
      // Simple: Just stop sending. UI handles timeout or loss.
      touch_srv.was_pressed = false;
    }
  }
}
