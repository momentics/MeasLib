/**
 * @file channel.c
 * @brief Channel Logic Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/channel.h"

// Global Tick Wrapper (called from main loop)
void meas_channel_tick(meas_channel_t *ch) {
  if (ch && ch->base.api) {
    // Safe cast to channel API
    const meas_channel_api_t *api = (const meas_channel_api_t *)ch->base.api;
    if (api->tick) {
      api->tick(ch);
    }
  }
}
