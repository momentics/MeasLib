/**
 * @file scpi_utils.c
 * @brief SCPI Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/scpi/scpi_utils.h"
#include "measlib/sys/scpi/scpi_types.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool scpi_strncasecmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && *s2) {
    if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
      return false;
    }
    s1++;
    s2++;
    n--;
  }
  if (n == 0) {
    return true;
  }
  return *s1 == *s2; // check for null terminator equality if n > 0
}

// Helper to check if a character is a separator
bool scpi_is_separator(char c) { return c == ':' || c == ' ' || c == '\t'; }

// Helper to check for message terminator
bool scpi_is_terminator(char c) { return c == '\n' || c == '\r' || c == ';'; }
