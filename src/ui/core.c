/**
 * @file core.c
 * @brief UI Controller Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/ui/core.h"
#include <stddef.h>

void meas_ui_tick(meas_ui_t *ui) {
  if (ui && ui->base.api) {
    // Safe cast to UI API
    const meas_ui_api_t *api = (const meas_ui_api_t *)ui->base.api;
    if (api->update) {
      api->update(ui);
    }
  }
}
