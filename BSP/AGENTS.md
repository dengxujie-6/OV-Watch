# BSP Development Rules

## 文件归类

新增或调整 BSP 驱动文件时，必须按硬件模块分类放置，禁止把新驱动文件直接散放在 `BSP/` 根目录。

- 按键驱动放在 `BSP/Key/`，例如 `BSP/Key/bsp_key.c`、`BSP/Key/bsp_key.h`。
- LCD、触摸、显示相关总线放在 `BSP/LCD/`。
- 蓝牙驱动放在 `BSP/BlueTooth/`。
- 传感器驱动按器件或模块放入对应目录，例如 `BSP/AHT21/`。

## 职责边界

BSP 层负责具体硬件细节，包括：

- GPIO 端口、引脚、上下拉和有效电平。
- HAL GPIO、SPI、I2C、ADC 等底层调用。
- 器件初始化、寄存器读写和总线传输。

BSP 层不得依赖 `USER/Task/`、`USER/GUI_page/`、`USER/PageManagement/` 等上层模块。

Application、Task、UI 页面不得直接 include BSP 头文件，也不得直接调用 HAL。上层访问硬件必须统一通过 `USER/HwAccess` 暴露的接口。

## 硬件接口收口

工程只保留一个面向上层的硬件访问入口：`HwAccess`。

不要新增类似 `BoardHW`、`DeviceAccess`、`PlatformHW` 等并行硬件入口。新硬件能力应作为模块操作表挂到 `HwAccess` 中，例如：

```c
HwAccess.key.init();
HwAccess.key.is_pressed(HWACCESS_KEY_BACK);
HwAccess.battery.get_percent();
```

推荐调用链：

```text
Application / Task / UI
    -> USER/HwAccess
        -> BSP/<Module>
            -> STM32 HAL / Hardware
```

禁止出现以下越层调用：

```text
Task/UI -> BSP
Task/UI -> HAL
UI -> GPIO/SPI/I2C/ADC
```

## Keil 工程同步

新增 `.c` 文件后，必须同步加入 `MDK-ARM/test.uvprojx`。

新增 BSP 子目录后，必须确认 Keil IncludePath 包含该目录。

include 写法必须和 IncludePath 配套：

- 如果 IncludePath 已包含 `..\BSP\Key`，代码中使用：

```c
#include "bsp_key.h"
```

- 不要写：

```c
#include "Key/bsp_key.h"
```

除非 IncludePath 只包含 `..\BSP`。
