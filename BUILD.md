# Build Instructions

## Prerequisites

- **CMake**: Version 3.16 or higher
- **Compiler**: GCC or Clang (Standard C11)
- **Make** or Ninja

## Building the Project

The project is configured using CMake. It supports out-of-source builds to keep the source directory clean.

### 1. Configure

Create a build directory and generate the build system. You must specify the target MCU.

**Supported Targets:**

<!-- - `STM32F072` -->
- `STM32F303` (Default)
- `AT32F403`

<!--
**Example (STM32F072):**

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DMEASLIB_TARGET=STM32F072 -B build_stm32f072
```
-->

**Example (STM32F303):**

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DMEASLIB_TARGET=STM32F303 -B build_stm32f303
```

**Example (AT32F403):**

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DMEASLIB_TARGET=AT32F403 -B build_at32f403
```

### 2. Compile

Build the firmware:

<!--
```bash
cmake --build build_stm32f072
```
-->
```bash
cmake --build build_stm32f303
```
