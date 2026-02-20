/**
 * @file scpi_types.h
 * @brief SCPI Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_SYS_SCPI_TYPES_H
#define MEASLIB_SYS_SCPI_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// SCPI Status Codes
typedef enum {
  SCPI_RES_OK = 0,
  SCPI_RES_ERR_SYNTAX = -100,
  SCPI_RES_ERR_INVALID_HEADER = -113,
  SCPI_RES_ERR_PARAM_NOT_ALLOWED = -108,
  SCPI_RES_ERR_MISSING_PARAM = -109,
  SCPI_RES_ERR_DATA_TYPE = -104,
} scpi_status_t;

// Context structure
struct scpi_context_s;
typedef size_t (*scpi_write_t)(struct scpi_context_s *ctx, const char *data,
                               size_t len);

typedef struct scpi_context_s {
  char *buffer;
  size_t buffer_len;
  size_t write_pos;
  void *user_context;
  scpi_write_t write;
  char *params; // Pointer to current command parameters
} scpi_context_t;

// Callback function type
typedef scpi_status_t (*scpi_callback_t)(scpi_context_t *ctx);

// Command structure
typedef struct scpi_command_s {
  const char *pattern;
  scpi_callback_t callback;
  const struct scpi_command_s *children;
} scpi_command_t;

#define SCPI_CMD_LIST_END {NULL, NULL, NULL}

#endif // MEASLIB_SYS_SCPI_TYPES_H
