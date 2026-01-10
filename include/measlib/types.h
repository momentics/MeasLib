/**
 * @file types.h
 * @brief Core Primitives and Type Definitions for MeasLib Framework.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines basic types used across the framework including generic property
 * variants, numeric abstractions (Float/Double/Fixed), and complex numbers.
 */

#ifndef MEASLIB_TYPES_H
#define MEASLIB_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Numeric Abstraction
 * Defaults to double, but can be configured to float or fixed-point in build
 * system.
 */
typedef double meas_real_t;

/**
 * @brief Resource Identifier
 * Used to uniquely identify channels, properties, and hardware resources.
 */
typedef uint32_t meas_id_t;

/**
 * @brief Complex Number Structure
 * Represents a complex number with real and imaginary parts.
 */
typedef struct {
  meas_real_t re;
  meas_real_t im;
} meas_complex_t;

/**
 * @brief Standard Return Status
 * Used by almost all API functions to indicate success or specific failure
 * modes.
 */
typedef enum {
  MEAS_OK = 0,  /**< Operation completed successfully */
  MEAS_ERROR,   /**< Generic error occurred */
  MEAS_PENDING, /**< Operation is in progress (async) */
  MEAS_BUSY     /**< Resource is currently busy */
} meas_status_t;

/**
 * @brief Property Variant Types
 * Enumeration of supported data types for the generic property system.
 */
typedef enum {
  PROP_TYPE_INT64,
  PROP_TYPE_REAL, // Abstracted floating point (float/double/fixed)
  PROP_TYPE_STRING,
  PROP_TYPE_BOOL,
  PROP_TYPE_COMPLEX
} meas_prop_type_t;

/**
 * @brief Generic Property Variant
 * A tagged union capable of holding any supported property value.
 */
typedef struct {
  meas_prop_type_t type;
  union {
    int64_t i_val;
    meas_real_t r_val;
    char *s_val;
    bool b_val;
    meas_complex_t c_val;
  };
} meas_variant_t;

#endif // MEASLIB_TYPES_H
