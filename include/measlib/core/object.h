/**
 * @file object.h
 * @brief Base Object System for MeasLib.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the root `meas_object_t` from which all framework entities inherit.
 * Provides a uniform interface for property management, lifecycle
 * (ref-counting), and polymorphic identification.
 */

#ifndef MEASLIB_CORE_OBJECT_H
#define MEASLIB_CORE_OBJECT_H

#include "measlib/types.h"

// Forward declaration of the base object handle
typedef struct meas_object_s meas_object_t;

/**
 * @brief Base Object Interface (VTable)
 * Contains function pointers common to all objects in the system.
 */
typedef struct {
  /**
   * @brief Get the object's human-readable name.
   * @param obj Pointer to the object instance.
   * @return const char* String representing the name.
   */
  const char *(*get_name)(meas_object_t *obj);

  /**
   * @brief Set a property value.
   * @param obj Pointer to the object instance.
   * @param key Property ID.
   * @param val Value variant.
   * @return meas_status_t Status code.
   */
  meas_status_t (*set_prop)(meas_object_t *obj, meas_id_t key,
                            meas_variant_t val);

  /**
   * @brief Get a property value.
   * @param obj Pointer to the object instance.
   * @param key Property ID.
   * @param val Pointer to variant to store the result.
   * @return meas_status_t Status code.
   */
  meas_status_t (*get_prop)(meas_object_t *obj, meas_id_t key,
                            meas_variant_t *val);

  /**
   * @brief Destructor / Release Reference.
   * Decrements the reference count and frees resources if it reaches zero.
   * @param obj Pointer to the object instance.
   */
  void (*destroy)(meas_object_t *obj);

} meas_object_api_t;

/**
 * @brief Base Object Structure
 * The "root class" structure that implements the VTable pointer and reference
 * counting.
 */
struct meas_object_s {
  const meas_object_api_t *api; /**< Virtual Function Table pointer */
  void *impl;         /**< Private Implementation (PIMPL) or Driver Context */
  uint32_t ref_count; /**< Reference Counter for lifecycle management */
};

#endif // MEASLIB_CORE_OBJECT_H
