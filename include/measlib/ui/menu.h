/**
 * @file menu.h
 * @brief Static Menu System.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the structure for creating static, RAM-efficient menu trees.
 */

#ifndef MEASLIB_UI_MENU_H
#define MEASLIB_UI_MENU_H

#include <stdbool.h>

/**
 * @brief Menu Item Types
 */
typedef enum {
  MENU_ITEM_ACTION,  /**< Triggers a callback function */
  MENU_ITEM_SUBMENU, /**< Opens a child menu */
  MENU_ITEM_TOGGLE,  /**< Toggles a boolean value */
  MENU_ITEM_EDIT_NUM /**< Opens a numeric keypad */
} meas_menu_type_t;

/**
 * @brief Menu Item Definition
 * Designed to be allocated in Flash (const).
 */
typedef struct meas_menu_item_s {
  const char *label;     /**< Display Text */
  meas_menu_type_t type; /**< Behavior Type */
  union {
    void (*action)(void *ctx);              /**< Action Callback */
    const struct meas_menu_item_s *submenu; /**< Child Menu Pointer */
    struct {
      bool *check_target; /**< Pointer to bool to toggle */
    } toggle;
  };
} meas_menu_item_t;

#endif // MEASLIB_UI_MENU_H
