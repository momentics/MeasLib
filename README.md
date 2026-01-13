# MeasLib Framework Design

The `MeasLib` project is a modular, C11-based generic firmware framework for measurement instruments (VNA, SA, Signal Generators, Multimeters). It is designed strictly for **Embedded Execution**, running directly on hardware (AT32, FPGA) without an underlying OS or transport layer.

## Architecture Overview

The system architecture is strictly divided into two logical planes to ensure deterministic performance:

### 1. Control Plane (Configuration & orchestration)

* **Low Bandwidth / High Latency Tolerance**.
* **Property System**: `meas_object_t` / `meas_variant_t`.
* **Orchestration**: `meas_device_t` -> `meas_channel_t`.
* **Messaging**: `meas_event_t` (Publisher/Subscriber) for async updates.
* **Files**: `object.h`, `device.h`, `channel.h`, `event.h`.

### 2. Data Plane (High-Throughput IO)

* **High Bandwidth / Zero-Copy / Real-time**.
* **Direct Memory Access**: Drivers write directly to app-provided buffers.
* **Pipeline**: `meas_data_block_t` -> `meas_trace_t` -> `meas_ui_t`.
* **No Allocation**: All data paths use static/shared memory.
* **Files**: `data.h`, `trace.h`, `render.h` (UI).

### 3. System Services (Background Tasks)

* **Bridge**: Connects Hardware Events to Application Logic.
* **Services**:
  * `input_service`: Polling buttons/encoders.
  * `touch_service`: Touchscreen coordinate mapping and gesture detection.
  * `render_service`: Consumes UI dirty map and pushes pixels to LCD.
  * `shell_service`: SCPI-like command interface over USB/VCP.
* **Files**: `measlib/sys/*.h`

## Project Structure

```text
MeasLib/
├── boards/             # Target Specific Implementations
│   ├── AT32F403/       # Cortex-M4F (Artery)
│   ├── STM32F072/      # Cortex-M0 (ST)
│   └── STM32F303/      # Cortex-M4F (ST)
├── include/
│   ├── measlib/
│   │   ├── types.h             # Core Primitives
│   │   ├── utils/
│   │   │   └── math.h          # Interpolation, Approx, Statistics
│   │   ├── dsp/
│   │   │   ├── dsp.h           # FFT, Windowing, Filtering
│   │   │   ├── chain.h         # Node Pipeline
│   │   │   ├── node_types.h    # Shared Context Structs
│   │   │   └── analysis.h      # Peak Search, Regression, Matching
│   │   ├── core/               # Framework Kernel
│   │   │   ├── object.h        # Base Object
│   │   │   ├── device.h        # Device Facade
│   │   │   ├── channel.h       # Measurement Channel
│   │   │   ├── data.h          # Data Blocks
│   │   │   ├── trace.h         # Trace Handling
│   │   │   ├── marker.h        # Markers
│   │   │   ├── event.h         # Messaging
│   │   │   ├── io.h            # Streams
│   │   │   └── storage.h       # Filesystem
│   │   ├── modules/            # Instrument Domains
│   │   │   ├── vna/            # Vector Network Analyzer
│   │   │   ├── sa/             # Spectrum Analyzer
│   │   │   ├── gen/            # Signal Generator
│   │   │   └── dmm/            # Multimeter
│   │   ├── ui/                 # Generic UI System
│   │   │   ├── core.h          # UI Core & Ticking
│   │   │   ├── render.h        # Drawing Backend API
│   │   │   ├── input.h         # Input Types & Calibration
│   │   │   ├── colors.h        # Theme Definitions
│   │   │   ├── fonts.h         # Font Assets
│   │   │   ├── menu.h          # Menu System
│   │   │   └── components/     # Reusable Widgets
│   │   ├── sys/                # System Services
│   │   │   ├── input_service.h # Button/Encoder Polling
│   │   │   ├── touch_service.h # Touch Processing
│   │   │   ├── shell_service.h # Command Line Interface
│   │   │   └── render_service.h# Display Refresh Manager
│   │   └── drivers/            # Abstract Hardware Interfaces
│   │       └── api.h           # Driver Registry API
├── src/
│   ├── main.c                  # Entry Point & Superloop
│   ├── core/
│   │   ├── object.c            # Object System Implementation
│   │   ├── event.c             # Event Dispatcher
│   │   ├── device.c            # Device Facade
│   │   ├── channel.c           # Channel Manager
│   │   ├── trace.c             # Trace Types
│   │   └── io.c                # IO Streams
│   ├── drivers/
│   │   ├── registry.c          # Driver Registration Logic
│   │   └── stm32f303/          # Reference Target Implementation
│   │       ├── drv_adc.c       # ADC Driver
│   │       ├── drv_controls.c  # GPIO/Rotary
│   │       ├── drv_flash.c     # Internal Flash Access
│   │       ├── drv_i2c.c       # I2C Bus Driver
│   │       ├── drv_lcd.c       # LCD Interface
│   │       ├── drv_sd.c        # SD Card SPI
│   │       ├── drv_synth.c     # Frequency Synthesizer
│   │       ├── drv_touch.c     # Touch Controller
│   │       ├── drv_usb_vcp.c   # USB Serial (VCP)
│   │       └── drv_watchdog.c  # Independent Watchdog
│   ├── dsp/
│   │   ├── dsp.c               # Core DSP functions
│   │   ├── chain.c             # Node Processing Pipeline
│   │   ├── analysis.c          # Higher Level Analysis
│   │   └── nodes/              # Individual DSP Nodes
│   │       ├── node_gain.c     # Gain/Offset/Linear
│   │       ├── node_math.c     # Mag/Phase/Avg/GroupDelay
│   │       ├── node_spectral.c # FFT/Window
│   │       ├── node_source.c   # Signal Generation
│   │       ├── node_radio.c    # DDC/Downconversion
│   │       ├── node_sink.c     # Data Sinks
│   │       └── node_calibration.c # Vector Error Correction
│   ├── modules/                # Domain Logic Implementation
│   │   ├── vna/
│   │   ├── sa/
│   │   ├── gen/
│   │   └── dmm/
│   ├── sys/                    # Service Implementations
│   │   ├── input_service.c
│   │   ├── render_service.c
│   │   ├── shell_service.c
│   │   └── touch_service.c
│   ├── ui/
│   │   ├── core.c              # UI Lifecycle
│   │   ├── render_cell.c       # Tile-based Renderer
│   │   ├── layout_main.c       # Main Screen Layout
│   │   ├── colors.c            # Color Palette
│   │   ├── components/         # Widget implementations
│   │   └── fonts/              # Font Data
│   └── utils/
│       └── math.c
└── tests/              # Host-based Test Suite
    ├── mocks/          # Simulated hardware drivers
    └── src/            # Unit/Integration tests
```

## 1. Memory Management (No-Heap Design)

The framework is designed to run in environments **without dynamic memory allocation (malloc/free)**.

### 1.1 Static Allocation Strategy

* **Objects**: All framework objects (`meas_device_t`, `meas_channel_t`) are designed effectively as "c-style classes" that can be statically allocated in `.bss` or on the stack.
* **Initialization**: API functions accept pointers to pre-allocated memory:

    ```c
    // Example: Static initialization
    static meas_channel_t my_channel;
    meas_status_t status = device->api->create_channel(device, 1, &my_channel);
    ```

### 1.2 Buffer Ownership

* **Caller-Owned**: The framework never allocates data buffers internally.
* **Shared Memory**: Channels operate on buffers provided by the application via properties (e.g., `MEAS_PROP_VNA_BUFFER_PTR`).
* **Zero-Copy**: Data pointers are passed through the system without copying (or via Event Payload).

### 1.3 Build System (CMake)

The project uses CMake (3.16+) with support for multiple targets:

* **MEASLIB_TARGET**: `STM32F303`, `STM32F072`, `AT32F403`.
* **MEASLIB_BUILD_TESTS**: Build for Linux Host (Unit Tests).

## 2. Generic Object Model

All entities in the framework inherit from `meas_object_t`. This provides a uniform way to handle configuration (properties) and lifecycle.

```c
// Base Object Handle
typedef struct meas_object_s meas_object_t;

// Generic Variant for Properties
typedef enum {
    PROP_TYPE_INT64,
    PROP_TYPE_REAL,    // Abstracted floating point (float/double/fixed)
    PROP_TYPE_STRING,
    PROP_TYPE_BOOL,
    PROP_TYPE_COMPLEX,
    PROP_TYPE_PTR      // Pointer to external memory (Caller-Owned Buffers)
} meas_prop_type_t;

// Numeric Abstraction
typedef double meas_real_t; // Default to double, configurable in build
typedef uint32_t meas_id_t; // Resource Identifier

typedef struct {
    meas_real_t re;
    meas_real_t im;
} meas_complex_t;

typedef enum {
    MEAS_OK = 0,
    MEAS_ERROR,
    MEAS_PENDING,
    MEAS_BUSY
} meas_status_t;

typedef struct {
    meas_prop_type_t type;
    union {
        int64_t i_val;
        meas_real_t r_val;
        char* s_val;
        bool b_val;
        meas_complex_t c_val;
        void* p_val; // For PROP_TYPE_PTR
    };
} meas_variant_t;

// Object Interface (VTable)
typedef struct {
    const char* (*get_name)(meas_object_t* obj);
    meas_status_t (*set_prop)(meas_object_t* obj, meas_id_t key, meas_variant_t val);
    meas_status_t (*get_prop)(meas_object_t* obj, meas_id_t key, meas_variant_t* val);
    void (*destroy)(meas_object_t* obj);
} meas_object_api_t;

// The Base Struct impementing inheritance
struct meas_object_s {
    const meas_object_api_t* api; // Virtual Function Table
    void* impl;                   // Private Implementation (PIMPL)
    uint32_t ref_count;           // Reference Counting
};
```

## 3. Driver Layer (Composite Model)

The framework uses a **Composite Driver Model** to handle complex embedded devices. The "Device" is a facade that orchestrates lower-level hardware components.

### 3.1 Instrument Facade (`meas_device_t`)

The Application Layer interacts *only* with this interface.

```c
// Device Handle
typedef struct {
    meas_object_t base; // Inherits Object
} meas_device_t;

// Forward Declaration for Factory
typedef struct meas_channel_s meas_channel_t;
typedef struct {
    char name[32];
    uint32_t version;
    uint32_t capabilities;
} meas_device_info_t;

// Device Interface
typedef struct {
    meas_object_api_t base;
    
    // Core Operations
    // resource_id: Hardware identifier (e.g., "ADC1", "SPI2")
    meas_status_t (*open)(meas_device_t* dev, const char* resource_id);
    meas_status_t (*close)(meas_device_t* dev);
    meas_status_t (*reset)(meas_device_t* dev);
    meas_status_t (*get_info)(meas_device_t* dev, meas_device_info_t* info);
    
    // Factory: Creates Channels bound to this device
    meas_status_t (*create_channel)(meas_device_t* dev, meas_id_t ch_id, meas_channel_t** out_ch);
} meas_device_api_t;
```

### 3.2 Hardware Components (Internal HAL)

Drivers are built by composing these reusable abstractions. These are **not** exposed to the App layer directly.

```c
// Frequency Synthesizer (e.g. Si5351, ADF4350)
typedef struct {
    meas_status_t (*set_freq)(void* ctx, meas_real_t hz);
    meas_status_t (*set_power)(void* ctx, meas_real_t dbm);
    meas_status_t (*enable_output)(void* ctx, bool enable);
} meas_hal_synth_api_t;

// Receiver / ADC (e.g. TLV320, Internal ADC)
typedef struct {
    meas_status_t (*configure)(void* ctx, meas_real_t sample_rate, int decimation);
    meas_status_t (*start)(void* ctx, void* buffer, size_t size); // DMA Start
    meas_status_t (*stop)(void* ctx);
} meas_hal_rx_api_t;

// RF Frontend (Switches, Attenuators)
typedef struct {
    meas_status_t (*set_path)(void* ctx, int path_id); // e.g., S11, S21
} meas_hal_fe_api_t;
```

### 3.3 Driver Registration (`meas_driver_api_t`)

Mechanims for registering hardware drivers at runtime (or link time).

```c
typedef struct {
    const char* name;
    meas_status_t (*init)(void); // Called during sys_init()
    meas_status_t (*probe)(void);
} meas_driver_desc_t;

// Registry API
meas_status_t meas_driver_register(const meas_driver_desc_t* desc);
```

### 3.4 Implemented Drivers

In `src/drivers/stm32f303/`, the following drivers are implemented:

* `drv_adc.c`: Internal ADC driver.
* `drv_synth.c`: Frequency synthesizer control.
* `drv_lcd.c`, `drv_touch.c`: Display and Touch Input.
* `drv_usb_vcp.c`: Virtual COM Port for CLI.
* `drv_flash.c`: Internal Flash storage.

## 4. Core Domain Entities

### 4.1 Measurement Channel (`meas_channel_t`)

A Logic Unit that coordinates a sweep. It owns Traces and manages the timing of the Driver components.

```c
struct meas_channel_s {
    meas_object_t base;
};

typedef struct {
    meas_object_api_t base;
    
    meas_status_t (*configure)(meas_channel_t* ch); // Applies properties to hardware
    meas_status_t (*start_sweep)(meas_channel_t* ch);
    meas_status_t (*abort_sweep)(meas_channel_t* ch);
} meas_channel_api_t;
```

### 4.2 Trace System (`meas_trace_t`)

Container for measurement data.

```c
typedef enum {
    TRACE_FMT_COMPLEX, // Real/Imag
    TRACE_FMT_REAL     // Magnitude only
} meas_trace_fmt_t;

typedef struct {
    meas_object_t base;
} meas_trace_t;

typedef struct {
    meas_object_api_t base;
    
    // Zero-Copy Data Access
    meas_status_t (*get_data)(meas_trace_t* t, const meas_real_t** x, const meas_real_t** y, size_t* count);
    meas_status_t (*set_format)(meas_trace_t* t, meas_trace_fmt_t fmt);
} meas_trace_api_t;
```

### 4.3 Analysis Tools (`meas_marker_t`)

Markers attach to a Trace and provide readout of specific data points.

```c
typedef struct {
    meas_object_t base;
} meas_marker_t;

typedef struct {
    meas_object_api_t base;
    
    // Configuration
    meas_status_t (*set_source)(meas_marker_t* m, meas_trace_t* trace);
    meas_status_t (*set_index)(meas_marker_t* m, size_t index);
    
    // Readout
    meas_status_t (*get_value)(meas_marker_t* m, meas_complex_t* val);
    meas_status_t (*get_stimulus)(meas_marker_t* m, meas_real_t* freq);
} meas_marker_api_t;
```

## 5. Messaging & Data Flow

### 5.1 Publisher/Subscriber

Decouples UI/Logic from Hardware Events.

```c
typedef enum {
    EVENT_PROP_CHANGED,
    EVENT_DATA_READY,
    EVENT_STATE_CHANGED,
    EVENT_ERROR
} meas_event_type_t;

typedef struct {
    meas_event_type_t type;
    meas_object_t* source;
    meas_variant_t payload;
} meas_event_t;

typedef void (*meas_event_cb_t)(const meas_event_t* event, void* user_data);

meas_status_t meas_subscribe(meas_object_t* pub, meas_event_cb_t cb, void* ctx);
```

**Constraints**:

* `MAX_SUBSCRIBERS` (32): Fixed static array.
* `MAX_EVENT_QUEUE` (16): Fixed ring buffer for pending events.

### 5.2 Generic Data Block (Zero-Copy)

Used for high-throughput data transfer (e.g., ADC data to Processing Chain).

```c
typedef struct {
    meas_id_t source_id;
    uint32_t sequence;
    size_t size;
    void* data; // Pointer to DMA buffer or Shared Memory
} meas_data_block_t;
```

## 6. User Interface (UI)

The UI layer is designed as a **Consumer** of the core framework. It runs on the same zero-copy principles and strictly avoids heap allocation.

### 6.1 Graphics Back-End (HAL)

Abstracts the physical display hardware and drawing primitives.

```c
// Pixel Format (e.g., RGB565)
typedef uint16_t meas_pixel_t;

// Render Context (Stack/Static)
// Defines the clipping region and buffer for drawing operations
typedef struct {
    meas_pixel_t* buffer;   // Pointer to display buffer (or tile)
    int16_t width;
    int16_t height;
    int16_t x_offset;       // Absolute X (for tile rendering)
    int16_t y_offset;       // Absolute Y (for tile rendering)
    meas_pixel_t fg_color;
    meas_pixel_t bg_color;
} meas_render_ctx_t;

// Drawing Interface (VTable)
// Implemented by the Display Driver (LCD/OLED)
typedef struct {
    void (*draw_pixel)(meas_render_ctx_t* ctx, int16_t x, int16_t y);
    void (*draw_line)(meas_render_ctx_t* ctx, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void (*fill_rect)(meas_render_ctx_t* ctx, int16_t x, int16_t y, int16_t w, int16_t h);
    void (*blit)(meas_render_ctx_t* ctx, int16_t x, int16_t y, int16_t w, int16_t h, const void* img, uint8_t alpha);
    
    // Font System
    void (*set_font)(meas_render_ctx_t* ctx, const meas_font_t* font);
    int16_t (*get_text_width)(meas_render_ctx_t* ctx, const char* text);
    int16_t (*get_text_height)(meas_render_ctx_t* ctx, const char* text);
    void (*draw_text)(meas_render_ctx_t* ctx, int16_t x, int16_t y, const char* text, uint8_t alpha);

    void (*get_dims)(meas_render_ctx_t* ctx, int16_t* w, int16_t* h);
} meas_render_api_t;
```

### 6.2 Font System

Fonts are defined as `meas_font_t` structures (in `measlib/ui/fonts.h`).

* **5x7**: Standard system font.
* **11x14**: Large readable font.
* Fonts are distinct zero-copy resources commonly stored in Flash.

### 6.3 UI Controller (`meas_ui_t`)

The central object creating the graphical interface. It subscribes to Device/channel events to trigger redraws.

```c
// Hit-Test Region
typedef struct {
    int16_t x, y, w, h;
    meas_id_t widget_id; // ID for callback routing
    void (*on_touch)(void* ctx, int16_t x, int16_t y);
} meas_ui_zone_t;

typedef struct {
    meas_object_t base;
    
    // State
    meas_id_t focused_id;           // Currently manipulated widget (Graph/Marker)
    meas_object_t* active_modal;    // Blocking Popup (e.g. Numpad)
    bool menu_open;
    
    // Scene Descriptor (Static list of clickable zones for current frame)
    meas_ui_zone_t hit_zones[16];   // Max 16 interactive areas
    uint8_t zone_count;
} meas_ui_t;

typedef struct {
    meas_object_api_t base;
    
    // Core UI Loop
    meas_status_t (*update)(meas_ui_t* ui);         // Process inputs/animations
    meas_status_t (*draw)(meas_ui_t* ui, const meas_render_api_t* draw_api);
    
    // Input Handling
    meas_status_t (*handle_input)(meas_ui_t* ui, meas_variant_t input_event);
} meas_ui_api_t;
```

### 6.4 Rendering Pipeline

The UI uses a **Stage-Based Pipeline** to strictly order drawing operations. This allows optimization (e.g. skipping "Traces" if the channel is idle).

```c
typedef enum {
    RENDER_STAGE_BG,        // Clear / Gradient
    RENDER_STAGE_GRID,      // Grids / Smith Charts
    RENDER_STAGE_TRACE,     // Measurement Points
    RENDER_STAGE_MARKER,    // Markers / Deltas
    RENDER_STAGE_OVERLAY,   // Menus / Status Bar
    RENDER_STAGE_COUNT
} meas_render_stage_t;

typedef struct {
    meas_render_stage_t stage;
    bool (*condition)(const void* ctx);
    void (*execute)(const void* ctx, const meas_render_api_t* api);
} meas_render_step_t;
```

### 6.5 Input & Interaction

Input is handled via a unified event system affecting the **Focus Model**.

```c
typedef enum {
    INPUT_TYPE_TOUCH_PRESS,
    INPUT_TYPE_TOUCH_MOVE,
    INPUT_TYPE_TOUCH_RELEASE,
    INPUT_TYPE_KEY_PRESS,
    INPUT_TYPE_ROTARY_ENC
} meas_input_type_t;

typedef struct {
    meas_input_type_t type;
    int16_t x;      // Touch X or Key Code
    int16_t y;      // Touch Y or Delta
    uint32_t timestamp;
} meas_input_event_t;
```

### 6.6 Menu System (Static Composition)

Menus are defined as static constant arrays of `meas_menu_item_t` to save RAM.

```c
typedef enum {
    MENU_ITEM_ACTION,
    MENU_ITEM_SUBMENU,
    MENU_ITEM_TOGGLE,
    MENU_ITEM_EDIT_NUM  // Spawns Keypad
} meas_menu_type_t;

typedef struct meas_menu_item_s {
    const char* label;
    meas_menu_type_t type;
    union {
        void (*action)(void* ctx);
        const struct meas_menu_item_s* submenu;
        struct {
             bool* check_target;
        } toggle;
    };
} meas_menu_item_t;
```

## 7. Calibration System

### 7.1 Measurement Calibration (`meas_cal_t`)

Handles vector error correction (SOLT model). It attaches to a **Channel** and intercepts data before it reaches Traces.

```c
// Calibration Standard Types
typedef enum {
    CAL_STD_OPEN,
    CAL_STD_SHORT,
    CAL_STD_LOAD,
    CAL_STD_THRU,
    CAL_STD_ISOLATION
} meas_cal_std_t;

// Error Terms Container (SOLT)
// Uses meas_real_t (float/double) to match system precision
typedef struct {
    meas_complex_t* ed; // Directivity
    meas_complex_t* es; // Source Match
    meas_complex_t* er; // Reflection Tracking
    meas_complex_t* et; // Transmission Tracking
    meas_complex_t* ex; // Isolation
} meas_cal_coefs_t;

typedef struct {
    meas_object_t base;
} meas_cal_t;

typedef struct {
    meas_object_api_t base;
    
    // Application
    meas_status_t (*apply)(meas_cal_t* cal, meas_data_block_t* data);
    
    // Collection
    meas_status_t (*measure_std)(meas_cal_t* cal, meas_cal_std_t std);
    meas_status_t (*compute)(meas_cal_t* cal, meas_cal_coefs_t* out_coefs);
} meas_cal_api_t;
```

### 7.2 UI Calibration (Touch)

Transforms raw touch coordinates to screen space using a 3-point calibration matrix.

```c
typedef struct {
    int32_t a, b, c, d, e, f, div;
} meas_touch_cal_t;

// Interface for Input Driver
meas_status_t meas_ui_calibrate_touch(meas_input_event_t* ev, const meas_touch_cal_t* cal);
meas_status_t meas_cal_save(meas_cal_t* cal, const char* filename);
meas_status_t meas_cal_load(meas_cal_t* cal, const char* filename);
```

## 8. Mathematics & DSP

The framework includes a high-performance math layer to handle signal processing tasks. It relies on `meas_real_t` to auto-scale precision (Float/Double/Fixed).

### 8.1 Math Utilities (`measlib/utils/math.h`)

Provides generic algorithms for analysis.

```c
// Interpolation, Extrapolation & Approximation
meas_real_t meas_math_interp_linear(meas_real_t x, meas_real_t x0, meas_real_t y0, meas_real_t x1, meas_real_t y1);
meas_real_t meas_math_interp_parabolic(meas_real_t y1, meas_real_t y2, meas_real_t y3, meas_real_t x);
meas_real_t meas_math_interp_cosine(meas_real_t y1, meas_real_t y2, meas_real_t x);
meas_real_t meas_math_extrap_linear(meas_real_t x, meas_real_t x0, meas_real_t y0, meas_real_t x1, meas_real_t y1);
bool meas_math_is_close(meas_real_t a, meas_real_t b, meas_real_t epsilon);

// Fast Math (Platform Optimized)
// Uses ASM instructions (VSQRT) or LUTs on supported boards
meas_real_t meas_math_sqrt(meas_real_t x);
meas_real_t meas_math_cbrt(meas_real_t x);
meas_real_t meas_math_log(meas_real_t x);
meas_real_t meas_math_log10(meas_real_t x);
meas_real_t meas_math_exp(meas_real_t x);
meas_real_t meas_math_atan(meas_real_t x);
meas_real_t meas_math_atan2(meas_real_t y, meas_real_t x);
meas_real_t meas_math_modf(meas_real_t x, meas_real_t *iptr);
void meas_math_sincos(meas_real_t angle, meas_real_t *sin_val, meas_real_t *cos_val);

// Statistics & Filtering
void meas_math_stats(const meas_real_t* data, size_t count, meas_real_t* mean, meas_real_t* std_dev, meas_real_t* min_val, meas_real_t* max_val);
meas_real_t meas_math_rms(const meas_real_t *data, size_t count);
void meas_math_sma(const meas_real_t *input, size_t count, size_t window_size, meas_real_t *output, size_t *out_count);
meas_real_t meas_math_ema(meas_real_t current_avg, meas_real_t new_sample, meas_real_t alpha);

// Complex Operations (if not supported by compiler)
meas_real_t meas_cabs(meas_complex_t z);
meas_real_t meas_carg(meas_complex_t z);
```

### 8.2 DSP Engine (`measlib/dsp/dsp.h` & `measlib/dsp/analysis.h`)

Abstracts hardware acceleration using `dsp_ops.h` (inline assembly for Cortex-M4 DSP instructions) instead of CMSIS-DSP dependency.

**Analysis Layer**:

* Peak Search (`meas_dsp_peak_find_max`)
* RF Matching (`meas_dsp_lc_match`)

```c
typedef enum {
    DSP_WINDOW_RECT,
    DSP_WINDOW_HANN,
    DSP_WINDOW_HAMMING,
    DSP_WINDOW_BLACKMAN
} meas_dsp_window_t;

// FFT Context (Pre-allocated tables)
typedef struct {
    size_t length;
    bool inverse;
    void* backend_handle; // e.g., arm_rfft_fast_instance_f32
} meas_dsp_fft_t;

// API
meas_status_t meas_dsp_fft_init(meas_dsp_fft_t* ctx, size_t length, bool inverse);
meas_status_t meas_dsp_fft_exec(meas_dsp_fft_t* ctx, const meas_complex_t* input, meas_complex_t* output);
meas_status_t meas_dsp_apply_window(meas_real_t* buffer, size_t size, meas_dsp_window_t win_type);
```

### 8.3 Processing Pipeline (`measlib/dsp/chain.h`)

A **Zero-Copy, Static Linked-List** architecture for defining signal processing flows at runtime.
Replaces hardcoded function calls with a configurable chain of **Nodes**:

`Source (ADC) -> Node (Window) -> Node (FFT) -> Sink (Trace)`

* **Node Interface**: Standardized `process(ctx, input, output)` vtable.
* **Zero-Copy**: Nodes pass pointers (`meas_data_block_t*`) downstream.
* **Static Memory**: Nodes are allocated in `.bss` or Stack, never Heap.
* **Implemented Nodes**:
  * `Node_Gain`, `Node_Linear` (Basic Math)
  * `Node_Window`, `Node_FFT` (Spectral)
  * `Node_Magnitude`, `Node_LogMag`, `Node_Phase`, `Node_GroupDelay`, `Node_Average` (Analysis)
  * `Node_DDC`, `Node_SParam`, `Node_Calibration` (Radio/VNA)
  * `Node_WaveGen` (Source)
  * `Node_SinkTrace` (Output)

## 9. Execution Model (Bare Metal)

The architecture uses a **Cooperative Superloop** combined with **Event-Driven FSMs** to achieve determinism without an RTOS.

### 9.1 The Main Loop

The logical core of the firmware (`main.c`) is a non-blocking loop dispatching events and ticking state machines.

```c
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
```

### 9.2 Event-Driven FSMs

Long-running operations (Sweeps, Calibration) are implemented as **Finite State Machines** to avoid blocking the CPU.

**Example: Sweep FSM (`meas_channel_t`)**

1. **IDLE**: Waits for `start` command.
2. **SETUP**: Configures PLL/Switch. Sets callback timer for settling time. Returns immediately.
3. **WAIT_SETTLE**: Entered via `EVENT_TIMER`. Triggers ADC DMA. Returns.
4. **WAIT_DMA**: Entered via `EVENT_DATA_READY` (from ISR). Processes data chunk.
5. **NEXT_POINT**: Loops back or completes.

### 9.3 Critical Sections & Concurrency

* **ISRs**: Only flag events (Publisher). No complex logic.
* **Main Loop**: Consumers (Subscribers). Runs atomic logic.
* **Data Integrity**: Double-buffering is used where ISR writes and Main reads (e.g., Audio/ADC stream).

## 10. External Connectivity & Storage

This layer handles communication with the outside world (PC, SD Card) using the same non-blocking, zero-copy principles.

### 10.1 Communication Streams (`meas_io_t`)

Abstracts byte-oriented interfaces (USB CDC, UART, BLE). Acts as both an Event Source (Input) and Data Sink (Output).

```c
typedef struct {
    meas_object_t base;
} meas_io_t;

typedef struct {
    meas_object_api_t base;
    
    // Transmission (Zero-Copy)
    // Queues buffer pointer for DMA transmission
    meas_status_t (*send)(meas_io_t* io, const void* data, size_t size);
    
    // Status
    size_t (*get_available)(meas_io_t* io); // Input buffer bytes
    bool (*is_connected)(meas_io_t* io);    // DTR state etc.
    meas_status_t (*receive)(meas_io_t* io, void* buffer, size_t size, size_t* read_count);
    meas_status_t (*flush)(meas_io_t* io);
} meas_io_api_t;
```

### 10.2 Storage System (`meas_fs_t`)

Unified interface for file systems (FatFS on SD, LittleFS on Flash).

```c
typedef struct {
    meas_object_t base;
} meas_file_t;

// Filesystem Facade
typedef struct {
    meas_object_api_t base;
    
    meas_status_t (*mount)(meas_object_t* fs_drv);
    meas_status_t (*unmount)(meas_object_t* fs_drv);
    
    // File Operations
    meas_status_t (*open)(meas_object_t* fs_drv, const char* path, meas_file_t** out_file);
    meas_status_t (*write)(meas_file_t* file, const void* data, size_t size);
    meas_status_t (*read)(meas_file_t* file, void* buffer, size_t size, size_t* read_count);
    meas_status_t (*seek)(meas_file_t* file, size_t offset);
    meas_status_t (*tell)(meas_file_t* file, size_t* offset);
    meas_status_t (*close)(meas_file_t* file);
    
    // Management
    meas_status_t (*mkdir)(meas_object_t* fs_drv, const char* path);
    meas_status_t (*remove)(meas_object_t* fs_drv, const char* path);
    meas_status_t (*stat)(meas_object_t* fs_drv, const char* path, size_t* size);
} meas_fs_api_t;
```
