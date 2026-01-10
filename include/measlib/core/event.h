/**
 * @file event.h
 * @brief Pub/Sub Messaging System.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the generic event structure and subscriber interface used to decouple
 * hardware drivers from the UI and application logic.
 */

#ifndef MEASLIB_CORE_EVENT_H
#define MEASLIB_CORE_EVENT_H

#include "measlib/core/object.h"

/**
 * @brief Event Types
 * Categorizes the nature of the event for proper dispatching.
 */
typedef enum {
  EVENT_PROP_CHANGED,  /**< A property value has changed */
  EVENT_DATA_READY,    /**< New measurement data is available */
  EVENT_STATE_CHANGED, /**< Operational state (e.g. Idle->Sweep) changed */
  EVENT_ERROR          /**< An error condition occurred */
} meas_event_type_t;

/**
 * @brief Generic Event Structure
 * Passed to all subscribers when an event occurs.
 */
typedef struct {
  meas_event_type_t type; /**< Type of the event */
  meas_object_t *source;  /**< The object that generated the event */
  meas_variant_t payload; /**< Data associated with the event */
} meas_event_t;

/**
 * @brief Event Callback Function Signature
 */
typedef void (*meas_event_cb_t)(const meas_event_t *event, void *user_data);

/**
 * @brief Subscribe to events from a publisher.
 *
 * @param pub Pointer to the publishing object (Device/Channel/etc).
 * @param cb Callback function to invoke.
 * @param ctx User context pointer passed to the callback.
 * @return meas_status_t MEAS_OK on success.
 */
meas_status_t meas_subscribe(meas_object_t *pub, meas_event_cb_t cb, void *ctx);

/**
 * @brief Publish an event to the system.
 * Internal or External use.
 *
 * @param ev The event structure to publish.
 * @return meas_status_t MEAS_OK on success, MEAS_BUSY if queue full.
 */
meas_status_t meas_event_publish(meas_event_t ev);

#endif // MEASLIB_CORE_EVENT_H
