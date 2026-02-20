/**
 * @file shell_service.c
 * @brief Shell Service Implementation (USB CDC).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/shell_service.h"
#include "measlib/drivers/hal.h"
#include "measlib/sys/scpi/scpi_core.h"
#include <stddef.h>
#include <string.h>

// Extern declaration for command tree initialization
void scpi_def_init(void);

static struct {
  const meas_hal_link_api_t *link;
  void *ctx;
  // SCPI Context
  scpi_context_t scpi_ctx;
  char scpi_line_buf[128];
  char rx_buf[64];
} shell_ctx;

static size_t shell_scpi_write(scpi_context_t *ctx, const char *data,
                               size_t len) {
  if (!ctx || !ctx->user_context)
    return 0;

  // ctx->user_context is the link_api pointer passed to scpi_init
  const meas_hal_link_api_t *link_api =
      (const meas_hal_link_api_t *)ctx->user_context;

  if (link_api && link_api->send) {
    // We need the link context (shell_ctx.ctx) for the send function
    return link_api->send(shell_ctx.ctx, (const uint8_t *)data, len);
  }
  return 0;
}

meas_status_t meas_shell_service_init(const void *link_api, void *ctx) {
  if (!link_api) {
    return MEAS_ERROR;
  }

  shell_ctx.link = (const meas_hal_link_api_t *)link_api;
  shell_ctx.ctx = ctx;

  // Initialize SCPI
  // Pass link_api as user_context to SCPI
  scpi_init(&shell_ctx.scpi_ctx, shell_ctx.scpi_line_buf,
            sizeof(shell_ctx.scpi_line_buf), (void *)link_api,
            shell_scpi_write);
  scpi_def_init();

  return MEAS_OK;
}

void meas_shell_service_poll(void) {
  if (!shell_ctx.link || !shell_ctx.link->recv)
    return;

  size_t read_len = 0;
  meas_status_t status = shell_ctx.link->recv(
      shell_ctx.ctx, shell_ctx.rx_buf, sizeof(shell_ctx.rx_buf), &read_len);

  if (status == MEAS_OK && read_len > 0) {
    // Process through SCPI
    scpi_process(&shell_ctx.scpi_ctx, shell_ctx.rx_buf, read_len);
  }
}
