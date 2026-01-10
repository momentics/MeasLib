cmake_minimum_required(VERSION 3.16)

# Project Definition
project(MeasLib VERSION 0.1 LANGUAGES C ASM)

# Add Include Directory to Global Path (Solves IDE Issues)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
    add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        VERBATIM
    )
    add_custom_target(compile_commands ALL DEPENDS "${CMAKE_SOURCE_DIR}/compile_commands.json")
endif()
include_directories(include)

# User Options
set(MEASLIB_TARGET "STM32F303" CACHE STRING "Target MCU: STM32F072, STM32F303 or AT32F403")

# MCU Flags
if(MEASLIB_TARGET STREQUAL "STM32F072")
    set(MCU_FLAGS -mcpu=cortex-m0 -mthumb -DARM_MATH_CM0)
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/STM32F072/STM32F072xB.ld")
    set(STACK_FLAGS -Wl,--defsym=__process_stack_size__=0x1C0 -Wl,--defsym=__main_stack_size__=0x100)
    add_compile_definitions(F072)
elseif(MEASLIB_TARGET STREQUAL "STM32F303")
    set(MCU_FLAGS -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -DARM_MATH_CM4)
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/STM32F303/STM32F303xC.ld")
    set(STACK_FLAGS -Wl,--defsym=__process_stack_size__=0x400 -Wl,--defsym=__main_stack_size__=0x100)
    add_compile_definitions(F303)
elseif(MEASLIB_TARGET STREQUAL "AT32F403")
    set(MCU_FLAGS -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -DARM_MATH_CM4)
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/AT32F403/AT32F403xC.ld")
    set(STACK_FLAGS -Wl,--defsym=__process_stack_size__=0x400 -Wl,--defsym=__main_stack_size__=0x100)
    add_compile_definitions(AT32)
else()
    message(FATAL_ERROR "Unsupported Target: ${MEASLIB_TARGET}")
endif()

# Common Compiler Flags (Matching Reference)
add_compile_options(
    ${MCU_FLAGS}
    -Wall -Wextra -Werror=implicit-function-declaration -Wstrict-prototypes
    -Os -g -gdwarf-2
    -ffast-math -fsingle-precision-constant
    -fno-common -ffunction-sections -fdata-sections
    --specs=nano.specs
)

add_link_options(
    ${MCU_FLAGS}
    ${STACK_FLAGS}
    -T${LINKER_SCRIPT}
    -L${CMAKE_CURRENT_SOURCE_DIR}/boards/ld
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    -lc -lm -lnosys
)

# Define the Core Library
add_library(MeasLib STATIC
    # Core
    src/core/event.c
    src/core/object.c
    src/core/device.c
    src/core/channel.c
    src/core/trace.c
    src/core/io.c
    
    # UI
    src/ui/core.c
    src/ui/input.c

    # Modules
    src/modules/vna/channel.c

    # Utils
    src/utils/math.c
    src/dsp/dsp.c

    # Drivers
    src/drivers/registry.c
    
    # Board Support
    boards/${MEASLIB_TARGET}/board.c
)

# Includes
target_include_directories(MeasLib PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Startup / Entry Point (Dummy for now, usually would be startup_stm32*.s)
# Using main.c as entry for the ELF to verify link
add_executable(FirmwareElf src/main.c)
target_link_libraries(FirmwareElf PRIVATE MeasLib)

# Binary / Hex Generation
if(CMAKE_OBJCOPY)
    add_custom_command(TARGET FirmwareElf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary FirmwareElf FirmwareElf.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex FirmwareElf FirmwareElf.hex
        COMMENT "Generating bin and hex output..."
    )
endif()
