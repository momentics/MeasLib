/**
 * @file scpi_core.h
 * @brief SCPI Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_SYS_SCPI_CORE_H
#define MEASLIB_SYS_SCPI_CORE_H

#include "measlib/sys/scpi/scpi_types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the SCPI context
 * @param ctx Pointer to context
 * @param buffer Buffer for input processing
 * @param buffer_len Length of buffer
 * @param user_context User defined context pointer
 * @param write_cb Output callback function
 */
void scpi_init(scpi_context_t *ctx, char *buffer, size_t buffer_len,
               void *user_context, scpi_write_t write_cb);

/**
 * @brief Process an input string
 * @param ctx Pointer to context
 * @param data Input data
 * @param len Length of data
 * @return scpi_status_t
 */
scpi_status_t scpi_process(scpi_context_t *ctx, const char *data, size_t len);

/**
 * @brief Register the command tree
 * @param root Root of the command tree
 */
void scpi_register_tree(const scpi_command_t *root);

/**
 * @brief Get the next parameter from the context
 * @param ctx Context
 * @param str Output string buffer
 * @param len Max length
 * @return scpi_status_t
 */
scpi_status_t scpi_param_string(scpi_context_t *ctx, char *str, size_t len);

/**
 * @brief Get the next parameter as an integer
 * @param ctx Context
 * @param val Output value
 * @return scpi_status_t
 */
scpi_status_t scpi_param_int(scpi_context_t *ctx, int32_t *val);

/**
 * @brief Get the next parameter as a float
 * @param ctx Context
 * @param val Output value
 * @return scpi_status_t
 */
scpi_status_t scpi_param_float(scpi_context_t *ctx, float *val);

#endif // MEASLIB_SYS_SCPI_CORE_H
