/**
 * @file main.c
 * @brief Firmware Entry Point & Superloop.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the high-level execution model defined in
 * doc/project_structure.md. Coordinates system initialization, event dispatch,
 * and component ticking.
 */

#include "measlib/core/channel.h"
#include "measlib/core/device.h"
#include "measlib/core/event.h"
#include "measlib/drivers/api.h"
#include "measlib/dsp/dsp.h"
#include "measlib/sys/input_service.h"
#include "measlib/sys/render_service.h"
#include "measlib/sys/shell_service.h"
#include "measlib/sys/touch_service.h"
#include "measlib/ui/core.h"

// -- Global Singletons (Statically Allocated) --
static meas_device_t *device = NULL;
static meas_channel_t *active_ch = NULL;
static meas_ui_t *ui = NULL;

// Dummy Implementations for Test Build
void sys_wait_for_interrupt(void) { __asm("wfi"); }

// Forward declarations of tick wrappers (implemented in respective core
// sources)
extern void meas_device_tick(meas_device_t *dev);
extern void meas_channel_tick(meas_channel_t *ch);
extern void meas_ui_tick(meas_ui_t *ui);

// Dispatcher (implemented in core/event.c or locally if simple)
extern void meas_dispatch_events(void);

int main(void) {
  // 1. Hardware Initialization
  sys_init();
  meas_dsp_tables_init();

  // 2. Main Superloop
  while (1) {
    // 2.1 Dispatch Events (High Priority)
    // Processes the queue populated by ISRs (Touch, USB, DMA)
    meas_dispatch_events();

    // Poll Input Service
    meas_input_service_poll();
    meas_touch_service_poll();
    meas_shell_service_poll();

    // 2.2 Component Ticks (Cooperative Tasks)
    // Each function must execute < 100us to maintain UI responsiveness
    if (device)
      meas_device_tick(device);
    if (active_ch)
      meas_channel_tick(active_ch);
    if (ui)
      meas_ui_tick(ui);

    // 2.3 Power Management
    // Wait for next Interrupt (WFI) if idle
    sys_wait_for_interrupt();
  }
  return 0;
}
