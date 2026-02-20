/**
 * @file scpi_core.c
 * @brief SCPI Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/scpi/scpi_core.h"
#include "measlib/sys/scpi/scpi_types.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const scpi_command_t *scpi_tree_root = NULL;

// Forward declarations
static scpi_status_t scpi_parse_line(scpi_context_t *ctx, char *line);
static const scpi_command_t *scpi_find_command(const scpi_command_t *node,
                                               const char *token);

void scpi_init(scpi_context_t *ctx, char *buffer, size_t buffer_len,
               void *user_context, scpi_write_t write_cb) {
  if (ctx) {
    ctx->buffer = buffer;
    ctx->buffer_len = buffer_len;
    ctx->write_pos = 0;
    ctx->user_context = user_context;
    ctx->write = write_cb;
    if (buffer && buffer_len > 0) {
      buffer[0] = '\0';
    }
  }
}

void scpi_register_tree(const scpi_command_t *root) { scpi_tree_root = root; }

scpi_status_t scpi_process(scpi_context_t *ctx, const char *data, size_t len) {
  if (!ctx || !ctx->buffer || !data) {
    return SCPI_RES_ERR_SYNTAX;
  }

  scpi_status_t last_status = SCPI_RES_OK;

  for (size_t i = 0; i < len; i++) {
    char c = data[i];

    if (ctx->write_pos >= ctx->buffer_len - 1) {
      // Buffer overflow - reset
      ctx->write_pos = 0;
      ctx->buffer[0] = '\0';
      last_status = SCPI_RES_ERR_SYNTAX; // Mark potential error
    }

    if (c == '\n' || c == '\r') {
      if (ctx->write_pos > 0) {
        ctx->buffer[ctx->write_pos] = '\0';
        last_status = scpi_parse_line(ctx, ctx->buffer);
        ctx->write_pos = 0;
      }
    } else {
      ctx->buffer[ctx->write_pos++] = c;
    }
  }
  return last_status;
}

#include "measlib/sys/scpi/scpi_utils.h"

static bool scpi_match_pattern(const char *pattern, const char *token) {
  size_t pat_len = strlen(pattern);
  size_t tok_len = strlen(token);

  // Simple check: token must be shorter or equal to pattern (Long form)
  // But SCPI allows SHORT form.
  // E.g. Pattern "MEASure", Token "MEAS" -> Match
  // Token "MEASURE" -> Match
  // Token "M" -> No match (unless pattern is M)

  // For now, let's use the utility to check case-insensitive match up to token
  // length And ensure token length is at least the short length (uppercase
  // part).
  // TODO: We need to know the short length. Conventional SCPI parsers store
  // this or infer it (uppercase). Assuming pattern is "MEASure" (Mixed case).

  // Check if token matches the start of pattern
  if (!scpi_strncasecmp(pattern, token, tok_len)) {
    return false;
  }

  // Now check length constraints.
  // If token length == pattern length -> Full Match.
  if (tok_len == pat_len)
    return true;

  // If token length is less, we need to check if it covers all uppercase
  // letters. Example: MEASure. Short is MEAS.
  size_t short_len = 0;
  for (size_t i = 0; i < pat_len; i++) {
    if (isupper((unsigned char)pattern[i])) {
      short_len++;
    } else {
      break; // Stop at first lowercase
    }
  }

  if (tok_len == short_len)
    return true;

  return false;
}

static const scpi_command_t *scpi_find_command(const scpi_command_t *list,
                                               const char *token) {
  const scpi_command_t *cmd = list;
  while (cmd && cmd->pattern) {
    if (scpi_match_pattern(cmd->pattern, token)) {
      return cmd;
    }
    cmd++;
  }
  return NULL;
}

// Helper to tokenize safely without strtok
static char *scpi_next_token(char **str, char delimiter) {
  if (!str || !*str)
    return NULL;

  char *start = *str;
  if (!*start)
    return NULL;

  char *end = strchr(start, delimiter);
  if (end) {
    *end = '\0';
    *str = end + 1;
  } else {
    *str = NULL; // End of string
  }
  return start;
}

static scpi_status_t scpi_parse_line(scpi_context_t *ctx, char *line) {
  if (!line || !scpi_tree_root)
    return SCPI_RES_OK;

  char *remaining = line;
  char *token;
  const scpi_command_t *curr_node = scpi_tree_root;
  const scpi_command_t *cmd = NULL;

  while ((token = scpi_next_token(&remaining, ':')) != NULL) {
    // Strip leading spaces
    while (isspace((unsigned char)*token))
      token++;

    // Skip empty tokens (e.g. from consecutive '::')
    if (*token == '\0')
      continue;

    // Check for parameters (space)
    char *param_start = strpbrk(token, " \t");
    if (param_start) {
      *param_start = '\0'; // Terminate header
      ctx->params = param_start + 1;
      // Skip leading spaces in params
      while (isspace((unsigned char)*ctx->params))
        ctx->params++;
    } else {
      ctx->params = NULL;
    }

    // Find match in current level
    cmd = scpi_find_command(curr_node, token);

    if (!cmd) {
      return SCPI_RES_ERR_INVALID_HEADER;
    }

    if (cmd->callback) {
      // Found a leaf/executable command
      return cmd->callback(ctx);
    }

    if (cmd->children) {
      curr_node = cmd->children;
    } else {
      return SCPI_RES_ERR_INVALID_HEADER;
    }
  }

  return SCPI_RES_OK;
}

scpi_status_t scpi_param_string(scpi_context_t *ctx, char *str, size_t len) {
  if (!ctx || !ctx->params || !str || len == 0)
    return SCPI_RES_ERR_MISSING_PARAM;

  // Get next param (comma sep)
  // We modify ctx->params to advance it
  // Note: strtok is okay here if we don't nest, but we want to retain
  // ctx->params state Let's use our next_token helper or similar logic

  char *start = ctx->params;
  if (!*start)
    return SCPI_RES_ERR_MISSING_PARAM;

  char *end = strchr(start, ',');
  size_t p_len;

  if (end) {
    p_len = end - start;
    ctx->params = end + 1; // Advance past comma
  } else {
    p_len = strlen(start);
    ctx->params = start + p_len; // Point to end
  }

  // Copy content
  if (p_len >= len)
    p_len = len - 1;
  memcpy(str, start, p_len);
  str[p_len] = '\0';

  return SCPI_RES_OK;
}

scpi_status_t scpi_param_int(scpi_context_t *ctx, int32_t *val) {
  char buf[32];
  scpi_status_t res = scpi_param_string(ctx, buf, sizeof(buf));
  if (res != SCPI_RES_OK)
    return res;

  // Parse int
  char *endptr;
  *val = (int32_t)strtol(buf, &endptr, 0);
  if (endptr == buf)
    return SCPI_RES_ERR_DATA_TYPE;

  return SCPI_RES_OK;
}

scpi_status_t scpi_param_float(scpi_context_t *ctx, float *val) {
  char buf[32];
  scpi_status_t res = scpi_param_string(ctx, buf, sizeof(buf));
  if (res != SCPI_RES_OK)
    return res;

  // Parse float
  char *endptr;
  *val = strtof(buf, &endptr);
  if (endptr == buf)
    return SCPI_RES_ERR_DATA_TYPE;

  return SCPI_RES_OK;
}
