cmake_minimum_required(VERSION 3.22)

# BLE passive-scan UUID trigger test, built from the CubeWB BLE_p2pClient example.
# HAL / WPAN middleware / utilities are compiled straight from the installed CubeWB package.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME ble_scan)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
project(${CMAKE_PROJECT_NAME})
enable_language(C ASM)

set(CUBE_WB "$ENV{HOME}/STM32Cube/Repository/STM32Cube_FW_WB_V1.24.0" CACHE PATH "STM32CubeWB package root")
set(HAL ${CUBE_WB}/Drivers/STM32WBxx_HAL_Driver)
set(WPAN ${CUBE_WB}/Middlewares/ST/STM32_WPAN)

add_executable(${CMAKE_PROJECT_NAME}
    startup_stm32wb55rgvx.s
    Core/Src/app_debug.c
    Core/Src/app_entry.c
    Core/Src/hw_timerserver.c
    Core/Src/hw_uart.c
    Core/Src/main.c
    Core/Src/stm32_lpm_if.c
    Core/Src/stm32wbxx_hal_msp.c
    Core/Src/stm32wbxx_it.c
    Core/Src/system_stm32wbxx.c
    Core/Src/syscalls.c
    Core/Src/sysmem.c
    STM32_WPAN/App/app_ble.c
    STM32_WPAN/App/uuid_trigger.c
    STM32_WPAN/Target/hw_ipcc.c

    ${CUBE_WB}/Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.c
    ${HAL}/Src/stm32wbxx_hal.c
    ${HAL}/Src/stm32wbxx_hal_cortex.c
    ${HAL}/Src/stm32wbxx_hal_dma.c
    ${HAL}/Src/stm32wbxx_hal_dma_ex.c
    ${HAL}/Src/stm32wbxx_hal_exti.c
    ${HAL}/Src/stm32wbxx_hal_flash.c
    ${HAL}/Src/stm32wbxx_hal_flash_ex.c
    ${HAL}/Src/stm32wbxx_hal_gpio.c
    ${HAL}/Src/stm32wbxx_hal_hsem.c
    ${HAL}/Src/stm32wbxx_hal_ipcc.c
    ${HAL}/Src/stm32wbxx_hal_pwr.c
    ${HAL}/Src/stm32wbxx_hal_pwr_ex.c
    ${HAL}/Src/stm32wbxx_hal_rcc.c
    ${HAL}/Src/stm32wbxx_hal_rcc_ex.c
    ${HAL}/Src/stm32wbxx_hal_rtc.c
    ${HAL}/Src/stm32wbxx_hal_rtc_ex.c
    ${HAL}/Src/stm32wbxx_hal_tim.c
    ${HAL}/Src/stm32wbxx_hal_tim_ex.c
    ${HAL}/Src/stm32wbxx_hal_uart.c
    ${HAL}/Src/stm32wbxx_hal_uart_ex.c

    ${WPAN}/ble/core/auto/ble_gap_aci.c
    ${WPAN}/ble/core/auto/ble_gatt_aci.c
    ${WPAN}/ble/core/auto/ble_hal_aci.c
    ${WPAN}/ble/core/auto/ble_hci_le.c
    ${WPAN}/ble/core/auto/ble_l2cap_aci.c
    ${WPAN}/ble/core/template/osal.c
    ${WPAN}/ble/svc/Src/svc_ctl.c
    ${WPAN}/interface/patterns/ble_thread/shci/shci.c
    ${WPAN}/interface/patterns/ble_thread/tl/hci_tl.c
    ${WPAN}/interface/patterns/ble_thread/tl/hci_tl_if.c
    ${WPAN}/interface/patterns/ble_thread/tl/shci_tl.c
    ${WPAN}/interface/patterns/ble_thread/tl/shci_tl_if.c
    ${WPAN}/interface/patterns/ble_thread/tl/tl_mbox.c
    ${WPAN}/utilities/dbg_trace.c
    ${WPAN}/utilities/otp.c
    ${WPAN}/utilities/stm_list.c
    ${WPAN}/utilities/stm_queue.c
    ${CUBE_WB}/Utilities/lpm/tiny_lpm/stm32_lpm.c
    ${CUBE_WB}/Utilities/sequencer/stm32_seq.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    STM32_WPAN/App
    ${CUBE_WB}/Drivers/BSP/P-NUCLEO-WB55.Nucleo
    ${CUBE_WB}/Drivers/CMSIS/Device/ST/STM32WBxx/Include
    ${CUBE_WB}/Drivers/CMSIS/Include
    ${HAL}/Inc
    ${HAL}/Inc/Legacy
    ${WPAN}
    ${WPAN}/ble
    ${WPAN}/ble/core
    ${WPAN}/ble/core/auto
    ${WPAN}/ble/core/template
    ${WPAN}/ble/svc/Inc
    ${WPAN}/ble/svc/Src
    ${WPAN}/interface/patterns/ble_thread
    ${WPAN}/interface/patterns/ble_thread/shci
    ${WPAN}/interface/patterns/ble_thread/tl
    ${WPAN}/utilities
    ${CUBE_WB}/Utilities/lpm/tiny_lpm
    ${CUBE_WB}/Utilities/sequencer
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32WB55xx
    USE_STM32WBXX_NUCLEO
    $<$<CONFIG:Debug>:DEBUG>
)

target_link_libraries(${CMAKE_PROJECT_NAME} m)
