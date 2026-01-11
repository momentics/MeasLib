/**
 * @file chain.h
 * @brief User-Defined DSP Processing Pipeline (Chain).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides a framework for linking specialized processing nodes (FFT, Filter,
 * Window) into a configurable execute chain.
 */

#ifndef MEASLIB_DSP_CHAIN_H
#define MEASLIB_DSP_CHAIN_H

#include "measlib/core/data.h"
#include "measlib/core/object.h"

// ============================================================================
// Processing Node (The Block)
// ============================================================================

typedef struct meas_node_s meas_node_t;

/**
 * @brief Node Interface (VTable).
 * Defines the behavior of a single processing block.
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Process a data chunk.
   *
   * @param node The node instance.
   * @param input Input data block (Zero-Copy).
   * @param[out] output Output data block. Node may fill this or pass 'input'
   * through.
   * @return MEAS_OK if successful.
   */
  meas_status_t (*process)(meas_node_t *node, const meas_data_block_t *input,
                           meas_data_block_t *output);

  /**
   * @brief Reset node state (e.g. clear filters).
   */
  meas_status_t (*reset)(meas_node_t *node);

} meas_node_api_t;

struct meas_node_s {
  const meas_node_api_t *api;
  void *impl;         // Private context (e.g. coefficients)
  meas_node_t *next;  // Linked List: Pointer to next node
  uint32_t ref_count; // Lifecycle
};

// ============================================================================
// Chain Manager (The Container)
// ============================================================================

typedef struct meas_chain_s meas_chain_t;

typedef struct {
  meas_object_api_t base;

  /**
   * @brief Append a node to the end of the chain.
   */
  meas_status_t (*append)(meas_chain_t *chain, meas_node_t *node);

  /**
   * @brief Clear all nodes from the chain.
   * Does NOT destroy the nodes (they might be static), just unlinks them unless
   * they are ref-counted.
   */
  meas_status_t (*clear)(meas_chain_t *chain);

  /**
   * @brief Run the chain for a given input block.
   * Traverses the linked list, calling process() on each node.
   */
  meas_status_t (*run)(meas_chain_t *chain, const meas_data_block_t *input);

} meas_chain_api_t;

struct meas_chain_s {
  const meas_chain_api_t *api;
  meas_node_t *head;
  meas_node_t *tail;
  uint32_t ref_count;
};

// ============================================================================
// API Functions
// ============================================================================

/**
 * @brief Initialize a statically allocated chain.
 */
meas_status_t meas_chain_init(meas_chain_t *chain);

/**
 * @brief Helper to link two nodes directly (for manual chaining without
 * managers).
 */
void meas_node_link(meas_node_t *source, meas_node_t *destination);

#endif // MEASLIB_DSP_CHAIN_H
