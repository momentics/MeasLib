cmake_minimum_required(VERSION 3.16)

# Project Definition
project(MeasLib VERSION 0.1 LANGUAGES C ASM)

# Options
option(MEASLIB_BUILD_TESTS "Build Host Tests (Linux)" OFF)
set(MEASLIB_TARGET "STM32F303" CACHE STRING "Target MCU: STM32F072, STM32F303 or AT32F403")

# Export Compile Commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Includes
include_directories(include)

# ==============================================================================
# SOURCES
# ==============================================================================
set(MEASLIB_CORE_SOURCES
    src/core/event.c
    src/core/object.c
    src/core/device.c
    src/core/channel.c
    src/core/trace.c
    src/core/io.c
    src/ui/core.c
    src/ui/input.c
    src/utils/math.c
    src/dsp/dsp.c
    src/dsp/analysis.c
    src/dsp/chain.c
    src/dsp/nodes/node_gain.c
    src/dsp/nodes/node_spectral.c
    src/dsp/nodes/node_sink.c
    src/dsp/nodes/node_math.c
    src/dsp/nodes/node_source.c
    src/dsp/nodes/node_radio.c
    src/dsp/nodes/node_calibration.c
    src/modules/sa/channel.c
    src/modules/gen/channel.c
    src/modules/vna/channel.c
    src/modules/dmm/channel.c
    src/drivers/registry.c
)

# ==============================================================================
# CONFIGURATION
# ==============================================================================

if(MEASLIB_BUILD_TESTS)
    # --- HOST TEST BUILD ---
    message(STATUS "Building for HOST (Testing Mode)")
    add_compile_definitions(MEASLIB_HOST_TEST)
    
    # Compiler Flags (Generic)
    set(CMAKE_C_FLAGS "-O0 -g -Wall -Wextra")

    # Library with Mocks
    add_library(MeasLib STATIC ${MEASLIB_CORE_SOURCES} tests/mocks/mock_hal.c)
    target_link_libraries(MeasLib PUBLIC m)
    target_include_directories(MeasLib PUBLIC 
        include 
        tests/mocks
    )

    # Test Runner
    add_executable(MeasLib_Test_Runner 
        tests/main_test.c 
        tests/src/modules/vna/test_vna_sanity.c
        tests/src/modules/vna/test_vna_pipeline.c
        tests/src/utils/test_math.c
        tests/src/utils/test_fast_math.c
        tests/src/dsp/test_fft.c
        tests/src/core/test_core_object.c
    tests/src/core/test_core_trace.c
    tests/src/dsp/nodes/test_node_window.c
    tests/src/core/test_events.c
    )
    target_link_libraries(MeasLib_Test_Runner PRIVATE MeasLib)
    target_include_directories(MeasLib_Test_Runner PRIVATE tests/framework)

    # Utils
    enable_testing()
    add_test(NAME VNA_Sanity_Test COMMAND MeasLib_Test_Runner)

else()
    # --- EMBEDDED FIRMWARE BUILD ---
    
    # 1. MCU Flags
    if(MEASLIB_TARGET STREQUAL "STM32F072")
        # set(MCU_FLAGS -mcpu=cortex-m0 -mthumb -DARM_MATH_CM0)
        # set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/STM32F072/STM32F072xB.ld")
        # add_compile_definitions(F072)
        message(FATAL_ERROR "F072 Config Not Complete in Stub")
    elseif(MEASLIB_TARGET STREQUAL "STM32F303")
        set(MCU_FLAGS -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -DARM_MATH_CM4)
        set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/STM32F303/STM32F303xC.ld")
        add_compile_definitions(F303)
        set(STARTUP_SRCS boards/STM32F303/startup.s boards/STM32F303/vectors.c)
    elseif(MEASLIB_TARGET STREQUAL "AT32F403")
        set(MCU_FLAGS -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -DARM_MATH_CM4)
        set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/AT32F403/AT32F403xG.ld")
        add_compile_definitions(AT32)
        set(STARTUP_SRCS boards/AT32F403/startup.s boards/AT32F403/vectors.c)
    else()
        message(FATAL_ERROR "Unsupported Target: ${MEASLIB_TARGET}")
    endif()

    # 2. Compiler Options
    add_compile_options(
        ${MCU_FLAGS}
        -Wall -Wextra -Werror=implicit-function-declaration -Wstrict-prototypes
        -ffast-math -fsingle-precision-constant
        -fno-common -ffunction-sections -fdata-sections
        --specs=nano.specs
    )

    add_link_options(
        ${MCU_FLAGS}
        -T${LINKER_SCRIPT}
        -L${CMAKE_CURRENT_SOURCE_DIR}/boards/ld
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
        -lc -lm
    )

    # 3. Library (with Real Board Support)
    add_definitions(-DF303)
    add_library(MeasLib STATIC 
        ${MEASLIB_CORE_SOURCES} 
        src/drivers/stm32f303/drv_adc.c
        src/drivers/stm32f303/drv_synth.c
        src/drivers/stm32f303/drv_controls.c
        src/drivers/stm32f303/drv_lcd.c
        src/drivers/stm32f303/drv_touch.c
        src/drivers/stm32f303/drv_i2c.c
        src/drivers/stm32f303/drv_watchdog.c
        src/drivers/stm32f303/drv_flash.c
        src/drivers/stm32f303/drv_usb_vcp.c
        boards/${MEASLIB_TARGET}/board.c
    )
    target_link_libraries(MeasLib PUBLIC m)
    
    target_include_directories(MeasLib PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/boards/${MEASLIB_TARGET}/include>
    )

    # 4. Firmware Executable
    add_executable(MeasLibFW src/main.c ${STARTUP_SRCS})
    target_link_libraries(MeasLibFW PRIVATE MeasLib)

    # 5. Hex Generation
    if(CMAKE_OBJCOPY)
        add_custom_command(TARGET MeasLibFW POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary MeasLibFW MeasLibFW.bin
            COMMAND ${CMAKE_OBJCOPY} -O ihex MeasLibFW MeasLibFW.hex
            COMMENT "Generating bin and hex output..."
        )
    endif()
endif()
