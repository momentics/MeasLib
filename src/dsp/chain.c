/**
 * @file chain.c
 * @brief Implementation of DSP Processing Chain.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include <stddef.h>

// ============================================================================
// Internal Helpers
// ============================================================================

static const char *chain_get_name(meas_object_t *obj) {
  (void)obj;
  return "DSP_Chain";
}

static meas_status_t chain_set_prop(meas_object_t *obj, meas_id_t key,
                                    meas_variant_t val) {
  (void)obj;
  (void)key;
  (void)val;
  return MEAS_ERROR; // No properties currently
}

static meas_status_t chain_get_prop(meas_object_t *obj, meas_id_t key,
                                    meas_variant_t *val) {
  (void)obj;
  (void)key;
  (void)val;
  return MEAS_ERROR;
}

static void chain_destroy(meas_object_t *obj) {
  // Static allocation model - nothing to free usually.
  // Ideally, we would traverse and unref nodes if we supported dynamic
  // lifecycle.
  meas_chain_t *chain = (meas_chain_t *)obj;
  chain->head = NULL;
  chain->tail = NULL;
}

// ============================================================================
// Chain API Implementation
// ============================================================================

static meas_status_t chain_append(meas_chain_t *chain, meas_node_t *node) {
  if (!chain || !node) {
    return MEAS_ERROR;
  }

  node->next = NULL; // Ensure new tail terminates

  if (chain->head == NULL) {
    // First node
    chain->head = node;
    chain->tail = node;
  } else {
    // Link previous tail to new node
    chain->tail->next = node;
    chain->tail = node;
  }

  return MEAS_OK;
}

static meas_status_t chain_clear(meas_chain_t *chain) {
  if (!chain) {
    return MEAS_ERROR;
  }
  chain->head = NULL;
  chain->tail = NULL;
  return MEAS_OK;
}

static meas_status_t chain_run(meas_chain_t *chain,
                               const meas_data_block_t *input) {
  if (!chain || !input) {
    return MEAS_ERROR;
  }

  meas_node_t *current = chain->head;
  meas_data_block_t current_data = *input; // Copy struct, not buffer
  meas_data_block_t next_data;

  while (current != NULL) {
    if (current->api && current->api->process) {
      // Execute Node
      // Note: Nodes are responsible for populating 'next_data'
      // If a node is a sink (end of line), it might not produce output.
      meas_status_t status =
          current->api->process(current, &current_data, &next_data);

      if (status != MEAS_OK) {
        return status; // Stop chain on error
      }

      // Output of this node becomes input of the next
      current_data = next_data;
    }
    current = current->next;
  }

  return MEAS_OK;
}

// VTable
static const meas_chain_api_t chain_api_impl = {
    .base =
        {
            .get_name = chain_get_name,
            .set_prop = chain_set_prop,
            .get_prop = chain_get_prop,
            .destroy = chain_destroy,
        },
    .append = chain_append,
    .clear = chain_clear,
    .run = chain_run,
};

// ============================================================================
// Public Functions
// ============================================================================

meas_status_t meas_chain_init(meas_chain_t *chain) {
  if (!chain) {
    return MEAS_ERROR;
  }
  chain->api = &chain_api_impl;
  chain->head = NULL;
  chain->tail = NULL;
  chain->ref_count = 1;
  return MEAS_OK;
}

void meas_node_link(meas_node_t *source, meas_node_t *destination) {
  if (source) {
    source->next = destination;
  }
}
