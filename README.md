# MeasLib Framework

Modular, C11-based firmware framework for measurement instruments (VNA, SA, Signal Generator, DMM).
Runs bare-metal on ARM Cortex-M (STM32F303, STM32F072, AT32F403).

**Core principles**: Zero dynamic allocation, zero-copy data paths, cooperative superloop with event-driven FSMs.

---

## Architecture

Three logical planes:

| Plane | Purpose | Key Types |
|-------|---------|-----------|
| **Control** | Configuration & orchestration | `meas_object_t`, `meas_device_t`, `meas_channel_t`, `meas_event_t` |
| **Data** | High-throughput IO | `meas_data_block_t`, `meas_trace_t`, `meas_node_t` |
| **Services** | Hardware → App bridge | `input_service`, `touch_service`, `render_service`, `shell_service` (SCPI) |

## Project Structure

```
include/measlib/
├── types.h              # Core primitives (meas_real_t, meas_complex_t, meas_variant_t, etc.)
├── core/
│   ├── object.h         # Base object (VTable, ref-counting, properties)
├── device.h         # Device facade
│   ├── channel.h        # Measurement channel (configure, start_sweep, tick)
│   ├── data.h           # Zero-copy data block
│   ├── trace.h          # Data container (Complex/Real formats)
│   ├── marker.h         # Marker readout
│   ├── event.h          # Pub/Sub messaging
│   ├── io.h             # Communication streams (USB CDC, UART)
│   └── storage.h        # Filesystem facade (FatFS/LittleFS)
├── modules/
│   ├── vna/             # Vector Network Analyzer
│   ├── sa/              # Spectrum Analyzer
│   ├── gen/             # Signal Generator
│   └── dmm/             # Digital Multimeter
├── dsp/
│   ├── dsp.h            # FFT, windowing, DDC, DDS, Goertzel, decimation
│   ├── chain.h          # Processing pipeline (linked nodes)
│   ├── node_types.h     # Node context structs
│   └── analysis.h       # Peak search, regression, LC matching
├── ui/
│   ├── core.h           # UI controller, rendering pipeline, dirty tiles
│   ├── render.h         # Graphics HAL (draw API with clipping, alpha, gradients)
│   ├── input.h          # Input events (touch, key, rotary)
│   ├── colors.h         # Theme definitions
│   ├── fonts.h          # Font assets (5x7, 11x14)
│   └── menu.h           # Static menu system
├── sys/
│   ├── input_service.h  # Button/encoder polling
│   ├── touch_service.h  # Touch processing
│   ├── shell_service.h  # CLI over USB CDC
│   ├── render_service.h # Tile-based display refresh
│   └── scpi/            # SCPI command parser & dispatcher
└── drivers/
    ├── api.h            # Driver registration
    └── hal.h            # HAL interfaces (synth, rx, FE, IO, touch, WDG, flash, link, storage, display)
src/
├── main.c               # Entry point & superloop
├── core/                # Kernel implementations
├── dsp/nodes/           # Individual DSP nodes (gain, math, spectral, radio, cal, sink, source)
├── modules/             # Domain logic (VNA, SA, GEN, DMM channels)
├── sys/                 # Service implementations
├── ui/                  # UI rendering engine
├── drivers/stm32f303/   # Reference target drivers
└── utils/math.c         # Math utilities
boards/
├── STM32F303/           # Board support: startup, vectors, linker script
├── STM32F072/
└── AT32F403/
```

## Core Types

```c
typedef double meas_real_t;       // Numeric abstraction (configurable)
typedef uint32_t meas_id_t;       // Resource identifier
typedef uint16_t meas_pixel_t;    // Pixel color (RGB565)

typedef struct { meas_real_t re; meas_real_t im; } meas_complex_t;
typedef struct { int16_t x; int16_t y; } meas_point_t;
typedef struct { int16_t x, y, w, h; } meas_rect_t;

typedef enum { MEAS_OK = 0, MEAS_ERROR, MEAS_PENDING, MEAS_BUSY } meas_status_t;
```

## Object Model

All entities inherit from `meas_object_t`:

```c
struct meas_object_s {
    const meas_object_api_t *api;   // VTable
    void *impl;                      // PIMPL / driver context
    uint32_t ref_count;              // Reference counting
};
```

VTable provides `get_name`, `set_prop`, `get_prop`, `destroy`. Helper wrappers (`meas_object_get_name`, etc.) route through the VTable.

Properties use `meas_variant_t` — a tagged union supporting `int64`, `real`, `string`, `bool`, `complex`, `ptr`.

## Driver Layer

### Device Facade

The app interacts only with `meas_device_t`:

```c
typedef struct {
    meas_object_api_t base;
    meas_status_t (*open)(meas_device_t *dev, const char *resource_id);
    meas_status_t (*close)(meas_device_t *dev);
    meas_status_t (*reset)(meas_device_t *dev);
    meas_status_t (*get_info)(meas_device_t *dev, meas_device_info_t *info);
    meas_status_t (*create_channel)(meas_device_t *dev, meas_id_t ch_id, meas_channel_t **out_ch);
} meas_device_api_t;
```

### HAL Interfaces

Low-level hardware abstractions (not exposed to app layer):

| Interface | Purpose |
|-----------|---------|
| `meas_hal_synth_api_t` | Frequency synthesizer (freq, power, output) |
| `meas_hal_rx_api_t` | Receiver / ADC (configure, DMA start/stop) |
| `meas_hal_fe_api_t` | RF front-end switching |
| `meas_hal_io_api_t` | LED, buttons |
| `meas_hal_touch_api_t` | Touch screen |
| `meas_hal_wdg_api_t` | Watchdog |
| `meas_hal_flash_api_t` | Internal flash (unlock, erase, program) |
| `meas_hal_link_api_t` | USB CDC / UART link |
| `meas_hal_storage_api_t` | SD card / block device |
| `meas_hal_display_api_t` | Display (window, fill, blit, orientation) |

### Driver Registration

```c
typedef struct {
    const char *name;
    meas_status_t (*init)(void);
    meas_status_t (*probe)(void);
} meas_driver_desc_t;

meas_status_t meas_driver_register(const meas_driver_desc_t *desc);
```

Critical section primitives: `sys_enter_critical()` / `sys_exit_critical()`.

## Channel & Measurement

```c
typedef struct {
    meas_object_api_t base;
    meas_status_t (*configure)(meas_channel_t *ch);
    meas_status_t (*start_sweep)(meas_channel_t *ch);
    meas_status_t (*abort_sweep)(meas_channel_t *ch);
    void (*tick)(meas_channel_t *ch);        // Non-blocking FSM step (<100us)
} meas_channel_api_t;
```

### Module-Specific Channels

| Module | Type | Pipeline |
|--------|------|----------|
| VNA | `meas_vna_channel_t` | DDC → S-Param → Cal → Sink |
| SA | `meas_sa_channel_t` | Window → FFT → Mag → LogMag → Sink |
| GEN | `meas_gen_channel_t` | WaveGen |
| DMM | `meas_dmm_channel_t` | Linear → Sink |

Each channel embeds a `meas_chain_t` pipeline and statically-allocated nodes with their contexts.

## DSP Pipeline

Zero-copy, static linked-list of processing nodes:

```c
typedef struct {
    meas_object_api_t base;
    meas_status_t (*process)(meas_node_t *node, const meas_data_block_t *input, meas_data_block_t *output);
    meas_status_t (*reset)(meas_node_t *node);
} meas_node_api_t;
```

**Available nodes**: Gain, Linear, Window, FFT, Magnitude, LogMagnitude, Phase, GroupDelay, Average, WaveGen, DDC, S-Param, Calibration, SinkTrace.

## Messaging

Event types: `PROP_CHANGED`, `DATA_READY`, `STATE_CHANGED`, `ERROR`, `INPUT_KEY`, `INPUT_TOUCH`.

```c
meas_status_t meas_subscribe(meas_object_t *pub, meas_event_cb_t cb, void *ctx);
meas_status_t meas_event_publish(meas_event_t ev);
void meas_dispatch_events(void);
```

## SCPI Command System

Tree-based SCPI parser with pattern matching and callbacks:

```c
typedef struct scpi_command_s {
    const char *pattern;
    scpi_callback_t callback;
    const struct scpi_command_s *children;
} scpi_command_t;
```

Integrated with `shell_service` for USB CDC / UART command interface.

## UI System

### Render API

Extensive drawing primitives with alpha blending, clipping stack, and alignment:

- Pixel, line, polyline, circle, arc, triangle, polygon
- Rect, round-rect (outline + fill)
- Gradient (horizontal, vertical)
- Text (standard, rotated, aligned, rect-fit)
- Dash/dot line patterns, thick lines
- High-density min-max graph (`draw_minmax_v`)
- Clip stack (push/pop, max 8 levels)

### Rendering Pipeline

Stage-based, tile-driven rendering:

```c
typedef enum {
    RENDER_STAGE_BG, RENDER_STAGE_GRID, RENDER_STAGE_TRACE,
    RENDER_STAGE_MARKER, RENDER_STAGE_OVERLAY, RENDER_STAGE_COUNT
} meas_render_stage_t;
```

UI tracks dirty tiles via bitmask. Render service draws only dirty tiles.

### Input & Touch

Input events: `TOUCH_PRESS`, `TOUCH_MOVE`, `TOUCH_RELEASE`, `KEY_PRESS`, `ROTARY_ENC`.

Touch calibration transforms raw coordinates to screen space using `meas_touch_cal_t` (6-coefficient affine matrix, `int16_t`).

## Calibration

**VNA**: SOLT vector error correction (`meas_cal_t`) with error terms (Ed, Es, Er, Et, Ex). Save/load to filesystem.

**DMM**: Linear gain/offset calibration (`meas_dmm_cal_coefs_t`).

## Execution Model

Cooperative superloop with event-driven FSMs:

```c
int main(void) {
    sys_init();
    meas_dsp_tables_init();
    while (1) {
        meas_dispatch_events();
        meas_input_service_poll();
        meas_touch_service_poll();
        meas_shell_service_poll();
        meas_device_tick(device);
        meas_channel_tick(active_ch);
        meas_ui_tick(ui);
        sys_wait_for_interrupt();
    }
}
```

Long-running operations (sweeps, calibration) use FSM states — never block or call `HAL_Delay` in a tick.

## Build

CMake 3.16+. Targets: `STM32F303`, `STM32F072`, `AT32F403`. Host tests via `MEASLIB_BUILD_TESTS`.

See `BUILD.md` for build instructions.
