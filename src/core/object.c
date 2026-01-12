/**
 * @file object.c
 * @brief Base Object Logic.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/object.h"

// Public Wrapper API (Helper functions to make VTable access cleaner)

const char *meas_object_get_name(meas_object_t *obj) {
  if (obj && obj->api && obj->api->get_name) {
    return obj->api->get_name(obj);
  }
  return "Unknown";
}

meas_status_t meas_object_set_prop(meas_object_t *obj, meas_id_t key,
                                   meas_variant_t val) {
  if (obj && obj->api && obj->api->set_prop) {
    return obj->api->set_prop(obj, key, val);
  }
  return MEAS_ERROR;
}

meas_status_t meas_object_get_prop(meas_object_t *obj, meas_id_t key,
                                   meas_variant_t *val) {
  if (obj && obj->api && obj->api->get_prop) {
    return obj->api->get_prop(obj, key, val);
  }
  return MEAS_ERROR;
}

void meas_object_destroy(meas_object_t *obj) {
  if (obj && obj->api && obj->api->destroy) {
    obj->api->destroy(obj);
  }
}

meas_object_t *meas_object_ref(meas_object_t *obj) {
  if (obj) {
    obj->ref_count++;
  }
  return obj;
}

void meas_object_unref(meas_object_t *obj) {
  if (obj) {
    if (obj->ref_count > 0) {
      obj->ref_count--;
      if (obj->ref_count == 0) {
        meas_object_destroy(obj);
      }
    }
  }
}
