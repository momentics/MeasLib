# Build Instructions

## Prerequisites

- **CMake** 3.16+
- **ARM GCC Toolchain** (`arm-none-eabi-gcc`) for Firmware
- **GCC/Clang** for Host Tests

## Building Firmware (Embedded)

To build the firmware for the default target (STM32F303):

```bash
mkdir build_fw
cd build_fw
cmake .. -DMEASLIB_TARGET=STM32F303
make
```

Artifacts: `MeasLibFW.bin`, `MeasLibFW.hex`

## Building Tests (Host / Linux)

To run the host-based unit and integration tests (using Mock drivers):

```bash
mkdir build_test
cd build_test
cmake .. -DMEASLIB_BUILD_TESTS=ON
make
# Run the test runner
./MeasLib_Test_Runner
```

## Supported Targets

- `STM32F303` (Default, NanoVNA-V2)
- `AT32F403` (Variant)
- `STM32F072` (Display Unit - In Progress)
