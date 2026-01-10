/**
 * @file event.c
 * @brief Event System Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements a simple, static-allocation friendly Event Dispatcher.
 * Uses a fixed-size queue for events and a static subscriber list.
 */

#include "measlib/core/event.h"
#include <stddef.h>

#define MAX_SUBSCRIBERS 32
#define MAX_EVENT_QUEUE 16

typedef struct {
  meas_object_t *pub;
  meas_event_cb_t cb;
  void *ctx;
} meas_sub_node_t;

// Static Storage
static meas_sub_node_t subscribers[MAX_SUBSCRIBERS];
static size_t sub_count = 0;

static meas_event_t event_queue[MAX_EVENT_QUEUE];
static size_t q_head = 0;
static size_t q_tail = 0;

meas_status_t meas_subscribe(meas_object_t *pub, meas_event_cb_t cb,
                             void *ctx) {
  if (sub_count >= MAX_SUBSCRIBERS)
    return MEAS_ERROR;

  subscribers[sub_count].pub = pub;
  subscribers[sub_count].cb = cb;
  subscribers[sub_count].ctx = ctx;
  sub_count++;

  return MEAS_OK;
}

// Internal: Push event from ISR or App
meas_status_t meas_event_publish(meas_event_t ev) {
  size_t next = (q_head + 1) % MAX_EVENT_QUEUE;
  if (next == q_tail)
    return MEAS_BUSY; // Queue Full

  event_queue[q_head] = ev;
  q_head = next;
  return MEAS_OK;
}

void meas_dispatch_events(void) {
  while (q_tail != q_head) {
    meas_event_t ev = event_queue[q_tail];
    q_tail = (q_tail + 1) % MAX_EVENT_QUEUE;

    // Broadcast to matching subscribers
    for (size_t i = 0; i < sub_count; i++) {
      if (subscribers[i].pub == ev.source || subscribers[i].pub == NULL) {
        // NULL pub means "listen to all" (wildcard)
        if (subscribers[i].cb) {
          subscribers[i].cb(&ev, subscribers[i].ctx);
        }
      }
    }
  }
}
