/**
 * @file scpi_def.c
 * @brief SCPI Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/scpi/scpi_core.h"
#include <stdio.h>
#include <string.h>

// Forward declarations of handlers
static scpi_status_t scpi_cmd_idn(scpi_context_t *ctx);
static scpi_status_t scpi_cmd_rst(scpi_context_t *ctx);

// Common commands
static const scpi_command_t scpi_root_cmds[] = {
    {.pattern = "*IDN?", .callback = scpi_cmd_idn, .children = NULL},
    {.pattern = "*RST", .callback = scpi_cmd_rst, .children = NULL},
    SCPI_CMD_LIST_END};

void scpi_def_init(void) { scpi_register_tree(scpi_root_cmds); }

static scpi_status_t scpi_cmd_idn(scpi_context_t *ctx) {
  if (ctx && ctx->write) {
    const char *idn = "MOMENTICS,MeasLib,0,0.1\r\n";
    ctx->write(ctx, idn, strlen(idn));
  }
  return SCPI_RES_OK;
}

static scpi_status_t scpi_cmd_rst(scpi_context_t *ctx) {
  (void)ctx;
  return SCPI_RES_OK;
}
