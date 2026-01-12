/**
 * @file input_service.c
 * @brief Input Service Implementation (GPIO Buttons).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/input_service.h"
#include "measlib/core/event.h"
#include <stddef.h>

// Button Bitmasks (Must match drv_controls.c)
#define BTN_LEFT (1 << 0)
#define BTN_ENTER (1 << 1) // PUSH
#define BTN_RIGHT (1 << 2)
#define BTN_MENU (1 << 3)

// Key Codes
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_ENTER 3

// Context
static struct {
  const meas_hal_io_api_t *io;
  void *io_ctx;
  uint32_t last_state;
} input_ctx;

void meas_input_service_init(const meas_hal_io_api_t *io, void *ctx) {
  input_ctx.io = io;
  input_ctx.io_ctx = ctx;
  input_ctx.last_state = 0;
}

void meas_input_service_poll(void) {
  if (!input_ctx.io)
    return;

  // Read Raw State
  uint32_t raw = input_ctx.io->read_buttons(input_ctx.io_ctx);

  uint32_t changed = raw ^ input_ctx.last_state;

  if (changed) {
    // Dispatch Events for Pressed Buttons (0->1 Transition)

    if ((changed & BTN_LEFT) && (raw & BTN_LEFT)) {
      meas_event_t ev = {.type = EVENT_INPUT_KEY,
                         .source = NULL,
                         .payload = {.i_val = KEY_LEFT}};
      meas_event_publish(ev);
    }
    if ((changed & BTN_RIGHT) && (raw & BTN_RIGHT)) {
      meas_event_t ev = {.type = EVENT_INPUT_KEY,
                         .source = NULL,
                         .payload = {.i_val = KEY_RIGHT}};
      meas_event_publish(ev);
    }
    if ((changed & BTN_ENTER) && (raw & BTN_ENTER)) {
      meas_event_t ev = {.type = EVENT_INPUT_KEY,
                         .source = NULL,
                         .payload = {.i_val = KEY_ENTER}};
      meas_event_publish(ev);
    }
  }
  input_ctx.last_state = raw;
}
