/**
 * @file scpi_utils.h
 * @brief SCPI Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_SYS_SCPI_UTILS_H
#define MEASLIB_SYS_SCPI_UTILS_H

#include <stdbool.h>
#include <stddef.h>

bool scpi_strncasecmp(const char *s1, const char *s2, size_t n);
bool scpi_is_separator(char c);
bool scpi_is_terminator(char c);

#endif // MEASLIB_SYS_SCPI_UTILS_H
