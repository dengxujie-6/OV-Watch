cmake_minimum_required(VERSION 3.22)

# 本文件提供独立的 GCC/CMake 构建入口，不读取 Keil 工程文件。
# 源文件按项目目录和工具链需求组织，避免 CMake 构建依赖 MDK-ARM 路径。
project(test C ASM)

# 生成 compile_commands.json，方便 clangd、编辑器和静态分析工具使用同一套编译参数。
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 项目关键路径集中定义。GCC 构建使用自己的启动文件、链接脚本和 FreeRTOS port。
set(TARGET_NAME test)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/stm32f411xe.ld")
set(GCC_STARTUP_FILE "${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s")
set(FREERTOS_GCC_PORT_DIR "${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F")
set(FREERTOS_GCC_PORT_FILE "${FREERTOS_GCC_PORT_DIR}/port.c")

# FreeRTOS 的 ARMCC/RVDS port 不能直接给 GCC 使用；这里提前失败，
# 防止后续出现更隐蔽的汇编语法或上下文切换实现错误。
if(NOT EXISTS "${FREERTOS_GCC_PORT_FILE}")
    message(FATAL_ERROR
        "FreeRTOS GCC Cortex-M4F port is missing:\n"
        "  ${FREERTOS_GCC_PORT_FILE}\n"
        "The repository currently only contains portable/RVDS/ARM_CM4F for the ARMCC/RVDS toolchain.\n"
        "Copy the official FreeRTOS V10.3.1 portable/GCC/ARM_CM4F directory here before using the GCC build.")
endif()

# Core/Src 为 CubeMX 生成的系统入口和外设初始化代码，只列入本工程实际使用的源文件。
set(CORE_SOURCES
    Core/Src/main.c
    Core/Src/gpio.c
    Core/Src/dma.c
    Core/Src/rtc.c
    Core/Src/spi.c
    Core/Src/tim.c
    Core/Src/freertos.c
    Core/Src/stm32f4xx_it.c
    Core/Src/stm32f4xx_hal_msp.c
    Core/Src/system_stm32f4xx.c
)

# HAL 源文件显式列出，避免把未启用外设、模板文件或 LL 驱动批量编入。
set(HAL_SOURCES
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
)

# BSP 和 USER 目录是项目自有代码，使用 CONFIGURE_DEPENDS 让新增 .c 文件触发重新配置。
file(GLOB_RECURSE BSP_SOURCES CONFIGURE_DEPENDS
    BSP/*.c
)

file(GLOB_RECURSE USER_SOURCES CONFIGURE_DEPENDS
    USER/FONTS/*.c
    USER/FreeRTOS_Debug/*.c
    USER/GUI_page/*.c
    USER/HwAccess/*.c
    USER/PageManagement/page_manager.c
    USER/Pictures/*.c
    USER/Task/*.c
)

# ui_CompassPage.c 是旧 SquareLine/LVGL 8 参考文件，当前页面实现已迁移到 Tools/compass_page.c。
list(REMOVE_ITEM USER_SOURCES "${CMAKE_SOURCE_DIR}/USER/GUI_page/ui_CompassPage.c")

# FreeRTOS 只编译内核、CMSIS-OS2 包装层、heap_4 和 GCC Cortex-M4F port。
set(FREERTOS_SOURCES
    Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    Middlewares/Third_Party/FreeRTOS/Source/list.c
    Middlewares/Third_Party/FreeRTOS/Source/queue.c
    Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    Middlewares/Third_Party/FreeRTOS/Source/timers.c
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
    Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
    "${FREERTOS_GCC_PORT_FILE}"
)

# LVGL minimal runtime 当前按源码目录整体编译，实际功能由 lv_conf.h 控制。
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    Middlewares/Third_Party/lvgl_minimal_runtime/lvgl/src/*.c
    Middlewares/Third_Party/lvgl_minimal_runtime/port/*.c
)

set(PROJECT_SOURCES
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${BSP_SOURCES}
    ${USER_SOURCES}
    ${FREERTOS_SOURCES}
    ${LVGL_SOURCES}
    "${GCC_STARTUP_FILE}"
)

# 输出文件保持 test.elf 命名，便于后续生成 test.hex/test.bin。
add_executable(${TARGET_NAME} ${PROJECT_SOURCES})
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "${TARGET_NAME}"
    SUFFIX ".elf"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F411xE
)

# 头文件路径按项目分层列出，并额外包含 GCC FreeRTOS port 目录。
target_include_directories(${TARGET_NAME} PRIVATE
    Core/Inc
    USER/HwAccess
    USER/Task/GUI_Task
    USER/Task/HardWare_init_task
    USER/Task/Key_Task
    USER/Task/PROM_Test
    USER/Task/Sensor_Task
    USER/Task/Watchdog_Task
    USER/PageManagement
    USER/GUI_page
    USER/GUI_page/Home
    USER/GUI_page/Menu
    USER/GUI_page/Common
    USER/GUI_page/Tools
    USER/GUI_page/Status
    USER/GUI_page/Diagnostics
    USER/FONTS
    USER/FreeRTOS_Debug
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    BSP/LCD
    BSP/Key
    BSP/Power
    BSP/IIC
    BSP/Watchdog
    BSP/PROM
    BSP/BlueTooth
    BSP/AHT21
    BSP/LSM303DLHC
    BSP/MPU6050
    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    "${FREERTOS_GCC_PORT_DIR}"
    Middlewares/Third_Party/lvgl_minimal_runtime
    Middlewares/Third_Party/lvgl_minimal_runtime/lvgl
    Middlewares/Third_Party/lvgl_minimal_runtime/lvgl/include
    Middlewares/Third_Party/lvgl_minimal_runtime/port
    Middlewares/Third_Party/lvgl_minimal_runtime/lvgl/include/lvgl/draw
)

# STM32F411CEU6 为 Cortex-M4F，使用单精度 FPU 和 hard-float ABI。
set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# 编译选项以嵌入式固件常用配置为主：
# -ffunction-sections/-fdata-sections 配合链接阶段 --gc-sections 裁剪未使用代码。
target_compile_options(${TARGET_NAME} PRIVATE
    ${CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:C>:-Wno-missing-field-initializers>
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Release>:-Os>
    $<$<CONFIG:Release>:-g0>
)

# 链接脚本描述 STM32F411 的 Flash/RAM 布局；nano/nosys specs 适合无操作系统文件系统的裸机/RTOS 固件。
target_link_options(${TARGET_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -specs=nano.specs
    -specs=nosys.specs
)

# 构建完成后生成烧录常用的 Intel HEX 和 raw BIN，同时打印镜像尺寸。
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generating HEX/BIN and printing image size"
)
