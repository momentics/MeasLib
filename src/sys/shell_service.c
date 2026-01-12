/**
 * @file shell_service.c
 * @brief Shell Service Implementation (USB CDC).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/shell_service.h"
#include "measlib/drivers/hal.h"
#include <stddef.h>

static struct {
  const meas_hal_link_api_t *link;
  void *ctx;
  char rx_buf[64];
} shell_ctx;

void meas_shell_service_init(const void *link_api, void *ctx) {
  shell_ctx.link = (const meas_hal_link_api_t *)link_api;
  shell_ctx.ctx = ctx;
}

void meas_shell_service_poll(void) {
  if (!shell_ctx.link || !shell_ctx.link->recv)
    return;

  size_t read_len = 0;
  // Non-blocking read (HAL Recv should return MEAS_OK or MEAS_PENDING/EMPTY)
  // Assuming recv(ctx, buf, len, &read)
  meas_status_t status = shell_ctx.link->recv(
      shell_ctx.ctx, shell_ctx.rx_buf, sizeof(shell_ctx.rx_buf), &read_len);

  if (status == MEAS_OK && read_len > 0) {
    // Echo back
    if (shell_ctx.link->send) {
      shell_ctx.link->send(shell_ctx.ctx, shell_ctx.rx_buf, read_len);
    }
  }
}
