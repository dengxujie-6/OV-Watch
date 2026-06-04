# AGENTS.md

## 1. 项目说明

本仓库是基于 STM32F411CEU6、CMSIS-OS2、FreeRTOS 和 LVGL 9 的智能手表嵌入式 UI 项目。
当前目标是完成任务调度、UI 页面、页面栈管理、LVGL 移植、板级驱动和硬件访问接口，并保持分层清晰。

当前不考虑 Boot 或 Bootloader。
除非用户明确要求，否则不要新增固件升级、Flash 分区、镜像校验、应用跳转或启动模式相关代码。

## 2. 工作方式

修改代码前，先阅读与任务直接相关的源文件、头文件、配置文件和调用链。
不要猜测硬件连接、GPIO 有效电平、设备地址、任务同步关系、API 语义或用户未说明的需求。
存在多种合理实现时，说明关键差异，并选择满足当前需求的最简单方案。

只做完成任务所必需的修改：

- 不顺便重构无关模块。
- 不批量重命名或重新格式化未涉及的代码。
- 不增加用户未要求的抽象层、配置项、回调、注册表或扩展接口。
- 不因为代码看起来未使用就擅自删除。
- 不执行 Git push、merge、rebase、reset、commit，也不修改 `.git/`，除非用户明确要求。

完成任务前，必须检查修改范围，并说明已经验证的内容、无法验证的内容和剩余风险。
没有完成编译或实机验证时，不得声称功能已经正常工作。

## 3. 软件架构与依赖方向

```text
Application
├── Tasks
├── UI App
└── PageManager

Middleware
├── CMSIS-OS2
├── FreeRTOS
└── LVGL

Driver
├── HWAccess
├── BSP
└── STM32 HAL

Hardware
├── STM32F411CEU6
├── ST7789V LCD
├── CST816T Touch
└── Sensors / Board Devices
```

依赖应从上层流向下层：

```text
UI App  ──────────────> LVGL / PageManager / HWAccess / BoardHW
Tasks   ──────────────> CMSIS-OS2 / FreeRTOS / HWAccess
GUI Task ─────────────> LVGL / PageManager
PageManager ──────────> GUI_Page_t create() / destroy()
HWAccess ─────────────> BSP
BSP ──────────────────> STM32 HAL / Core peripheral handles
STM32 HAL ────────────> Hardware
LVGL Port ────────────> LVGL + BSP / HWAccess
```

分层约束：

- Application 层不得直接访问寄存器。
- UI App 不得直接依赖 STM32 HAL、BSP 设备驱动或 FreeRTOS 任务实现。
- PageManager 不得依赖具体页面实现，不得管理具体 LVGL 对象。
- BSP、HWAccess、LVGL Port 和中间件不得依赖 `USER/GUI_page/`。
- 新代码不得扩大 Application 层对 HAL 或 BSP 的直接依赖。
- `USER/Task/Key_Task/Key_task.c` 和 `USER/Task/GUI_Task/LVGL_Task.c` 当前存在直接硬件依赖，属于待逐步收敛的历史实现。除非任务明确要求分层重构，否则不要为了顺便清理而扩大修改范围。

## 4. 目录职责

### `Core/` — CubeMX 生成与系统集成

- `Core/Src/main.c`：系统启动、时钟、外设初始化和 RTOS 启动。
- `Core/Src/freertos.c`：CMSIS-OS2 / FreeRTOS 任务对象的集中创建位置。
- `Core/Src/gpio.c`、`dma.c`、`rtc.c`、`spi.c`、`tim.c`：CubeMX 外设初始化。
- `Core/Src/stm32f4xx_it.c`：中断入口。
- `Core/Inc/FreeRTOSConfig.h`：FreeRTOS 配置。

规则：

- 只在 `USER CODE BEGIN` 与 `USER CODE END` 区域内增加手写代码。
- 外设配置以 `test.ioc` 为源头；改变引脚、时钟、DMA、NVIC 或外设模式时，优先修改或说明 CubeMX 配置。
- 不要把业务逻辑、页面逻辑或设备驱动实现放入 `Core/`。
- 新任务的创建放在 `Core/Src/freertos.c` 的用户代码区域；任务入口实现放在 `USER/Task/`。

### `USER/Task/` — Application / Tasks

当前任务包括：

- `GUI_Task/LVGL_Task.c`：初始化 LVGL、显示输入移植层、初始页面，并周期调用 `lv_timer_handler()`。
- `HardWare_init_task/HardWare_Init_Task.c`：执行板级硬件启动顺序。
- `Key_Task/Key_task.c`：按键扫描、消抖和事件投递。

规则：

- 任务负责调度、等待、事件处理和模块协作，不负责具体器件驱动。
- 不要在普通任务中创建复杂页面对象树；页面 UI 放在 `USER/GUI_page/`。
- 只有 GUI / LVGL 任务上下文可以调用 LVGL 页面和对象 API。
- 非 GUI 任务更新界面时，使用事件、队列、线程标志或缓存数据通知 GUI 任务。
- 有启动顺序依赖时使用明确同步机制，不要只依赖任务优先级或延时。
- 应用任务优先使用 CMSIS-OS2 API；仅在 CMSIS-OS2 无法满足需求、需要 FreeRTOS 诊断能力或必须使用原生 ISR API 时，才使用原生 FreeRTOS API，并说明原因。
- 新增任务时，说明任务用途、优先级、栈大小、阻塞方式和通信方式，并考虑注册到 `USER/FreeRTOS_Debug/` 的栈监控模块。

### `USER/GUI_page/` — Application / UI App

当前页面包括菜单、计算器、日历、充电、秒表和通用标题页。

规则：

- 每个页面只管理自己的 LVGL 对象、LVGL 定时器和私有状态。
- 页面根 screen 删除后，其子对象指针全部失效，不得继续访问。
- 页面销毁时，先删除仍可能回调页面状态的 LVGL 定时器或异步资源，再删除根对象。
- 页面不得直接创建、销毁或操作其他页面。
- 页面跳转和返回必须通过 `PageManager_Push()` 与 `PageManager_Pop()`。
- 不要在页面代码中散布 `lv_screen_load()` 以实现跨页面跳转。
- 页面不得直接调用 HAL、GPIO、SPI、DMA、BSP 驱动或任务入口。
- 页面需要显示硬件数据时，使用 `BoardHW`、`HwAccess` 或后续定义的应用数据接口。
- 页面事件回调应短小，不执行阻塞延时、长时间计算或硬件轮询。
- 不要为每个页面创建一个 FreeRTOS 任务。
- 不要引入频繁的原始 `malloc()` / `free()`。

### `USER/PageManagement/` — Application / PageManager

当前设计：

- `GUI_Page_t` 只包含 `create()` 和 `destroy()`。
- 页面栈只保存 `const GUI_Page_t *` 页面描述符指针。
- 页面栈由指针数组和栈顶索引组成。
- `PageManager_Push(page)` 销毁当前页面、将新页面入栈并调用新页面 `create()`。
- `PageManager_Pop()` 销毁当前页面、出栈并重新调用上一个页面 `create()`。

PageManager 的公共导航接口只保留：

```c
int PageManager_Push(const GUI_Page_t * page);
int PageManager_Pop(void);
```

规则：

- 不要新增 Root Page、页面根对象、页面注册表、页面 ID、历史记录对象或全局页面容器。
- 不要新增 `Init`、`LoadRoot`、`Replace`、`Clear`、`BackToRoot`、动画切换或路由接口，除非用户明确要求。
- PageManager 只负责页面描述符栈和 `create()` / `destroy()` 调用，不管理页面内部 LVGL 对象。
- PageManager 不应包含具体页面头文件，不应知道 `MenuPage`、`CalculatorPage` 等具体页面。
- 修改 PageManager 时，检查栈空、栈满、空指针、无效函数指针和索引越界。

### `USER/HwAccess/` — Driver / HWAccess

- `hwaccess.h` / `hwaccess.c`：向 Application 层提供 LCD 等硬件操作表，并绑定到具体 BSP 实现。
- `board_hw.h` / `board_hw.c`：向 UI 页面提供板级状态或缓存数据，例如电池电量百分比。

规则：

- Application 层控制硬件时，优先通过 HWAccess，而不是直接包含 BSP 或 HAL 头文件。
- HWAccess 接口应表达应用需要的能力，不暴露底层 SPI、GPIO、DMA 细节或 HAL 句柄。
- `BoardHW` 适合提供 UI 展示所需的状态、缓存值或业务化数据，不要在 UI 刷新路径中执行慢速硬件采样。
- HWAccess 可以依赖 BSP，但不得依赖 GUI 页面、PageManager 或具体任务实现。

### `BSP/` — Driver / BSP

当前实现包括：

- `BSP/LCD/st7789v.*`：ST7789V LCD 驱动。
- `BSP/LCD/CST816T.*`：CST816T 触摸驱动。
- `BSP/I2CVirtual.*`：软件 I2C / 虚拟 I2C 支持。

规则：

- BSP 负责器件寄存器、总线传输、板级引脚和设备初始化，不负责页面或业务逻辑。
- 实现驱动前，从原理图、数据手册、`test.ioc` 和现有代码确认引脚、有效电平、总线模式、地址和时序。
- 不要猜测传感器型号、I2C 地址、分压比例、按键电平或 LCD 偏移。
- BSP 可以调用 HAL 和使用 CubeMX 生成的外设句柄，但不得依赖 UI 页面、PageManager 或应用任务。
- 中断和 DMA 完成回调应尽量只记录状态、释放同步对象或通知任务，不执行耗时逻辑。

### `Middlewares/` — Middleware

- `Middlewares/Third_Party/FreeRTOS/` 和 `Middlewares/Third_Party/lvgl_minimal_runtime/lvgl/` 属于第三方源码，除非用户明确要求，否则不要修改。
- `Middlewares/Third_Party/lvgl_minimal_runtime/port/` 属于项目移植层，可以在显示和输入适配任务中修改。
- LVGL Port 可以依赖 BSP / HWAccess，但不得依赖具体页面或 PageManager。
- 中断上下文不得直接调用 LVGL API。LCD DMA 完成中断只通知 LVGL 任务，保持当前线程标志通知模型。
- 修改显示缓冲区、缓冲行数、颜色格式、Y 偏移或 DMA 刷新流程时，评估 RAM 占用、像素字节数和异步刷新完成时序。

### `Drivers/` 与 `MDK-ARM/`

- `Drivers/` 中的 CMSIS 和 STM32 HAL 驱动属于官方或第三方代码，除非用户明确要求，否则不要修改。
- 不要删除看起来未使用的 HAL 驱动文件来优化体积；是否参与最终链接由工程配置和链接器决定。
- 主 Keil 工程文件是 `MDK-ARM/test.uvprojx`，主目标名称是 `test`。
- 不要手动修改 `MDK-ARM/test/` 下的 `.o`、`.d`、`.crf`、`.axf`、`.map` 等构建产物。
- 新增 `.c` 文件后，确认其已加入 Keil 工程；新增头文件目录后，确认 Include Path 已配置。
- 不要因为新增文件而批量重排或重写整个 `.uvprojx`。

## 5. STM32、FreeRTOS 与 LVGL 约束

当前目标平台配置以 `test.ioc`、`Core/` 和 BSP 实现为准。已知配置包括：

- STM32F411CEU6，SYSCLK 100 MHz。
- RTC 使用 LSE。
- LCD 为 ST7789V，触摸为 CST816T。
- LVGL 逻辑显示尺寸由 `USER/HwAccess/hwaccess.h` 中的 `LCD_WIDTH` 和 `LCD_HEIGHT` 定义。
- LCD 物理显示区域的 Y 偏移由 LVGL 显示移植层处理。

通用约束：

- 修改时钟、DMA、SPI、PWM、RTC、GPIO 或中断配置前，先检查 `test.ioc` 和生成代码。
- 不要在任务或中断中定义大局部数组；新增全局缓冲区、帧缓冲区或任务栈时，评估 SRAM 占用。
- 使用 DMA 时，分析缓冲区生命周期、传输完成通知、重复启动、超时和并发访问。
- 中断中禁止调用阻塞 API，只能调用明确支持 ISR 上下文的 API。
- 使用原生 FreeRTOS ISR API 时，使用 `FromISR` 版本，并正确处理是否需要切换任务。
- `volatile` 不能替代任务同步、临界区、互斥量、队列或事件通知。
- 任务循环必须有明确阻塞点，不要创建无意义忙轮询。
- 保留并正确使用 `configCHECK_FOR_STACK_OVERFLOW`、`vApplicationStackOverflowHook()` 和 `uxTaskGetStackHighWaterMark()` 调试能力。
- 修改任务栈后，检查 `g_freertos_debug_tasks[]` 中的历史最小剩余栈空间。
- 所有 LVGL API 默认只允许在 GUI / LVGL 任务上下文调用。
- 删除父对象后，不要再次删除或访问已经由父对象自动删除的子对象。
- LVGL 定时器、事件回调和用户数据指针不得引用已销毁的页面状态。
- 显示刷新完成必须与 `lv_display_flush_ready()` 严格对应。
- 修改 RGB565、字节交换、刷新区域、DMA 或双缓冲逻辑时，确认像素格式和缓冲区大小一致。

## 6. C 语言与注释规范

- 使用完整、可编译的函数和逻辑块，不留下无意义占位代码或大量省略内容。
- 新增模块、结构体、公共函数和关键生命周期逻辑时，使用 `/** ... */` 文档注释。
- 函数内部的步骤说明使用 `//` 注释。
- 不要为了统一注释风格而修改 CubeMX、HAL、FreeRTOS 或 LVGL 第三方代码中的原始注释。
- 注释应解释设计原因、所有权、生命周期、线程上下文、硬件假设和边界条件。
- 指针参数应说明是否允许为 `NULL`、由谁拥有、有效期多长。
- 涉及数据宽度、协议、寄存器、像素或硬件接口时，优先使用固定宽度整数类型。
- 对无符号常量使用合适的 `U` / `UL` 后缀，避免隐式有符号与无符号转换。
- 私有函数和仅文件内使用的变量使用 `static`。
- 宏应避免隐藏副作用；能用函数或常量表达时，不创建复杂宏。
- 不要引入原始 `malloc()` / `free()`，除非任务明确需要并已分析失败处理与生命周期。
- 保持文件为 UTF-8，避免引入乱码或破坏现有中文注释。
- 匹配当前模块已有的命名和排版风格，不进行无关的全局格式化。

## 7. 新功能放置决策

| 需求 | 推荐位置 |
| --- | --- |
| 新增页面、页面控件、页面内事件 | `USER/GUI_page/` |
| 页面跳转、返回、页面栈行为 | `USER/PageManagement/` |
| 新增应用任务或任务级调度 | `USER/Task/` |
| 向应用暴露硬件能力 | `USER/HwAccess/` |
| 新器件驱动、寄存器读写、板级引脚 | `BSP/` |
| LVGL 显示或输入适配 | `Middlewares/Third_Party/lvgl_minimal_runtime/port/` |
| 外设引脚、DMA、时钟、NVIC 配置 | `test.ioc` + CubeMX 生成代码 |
| FreeRTOS 配置 | `Core/Inc/FreeRTOSConfig.h`，并确认生成影响 |

不要把功能放到“最容易写”的文件中，应放到职责正确的层。如果文件夹下面有AGENTS.md 需要先确认，作为规范后操作。

## 8. 验证与完成标准

每次修改后，至少执行以下检查：

1. 使用 `git status --short` 确认修改文件符合任务范围。
2. 使用 `git diff --check` 检查空白错误。
3. 阅读 `git diff`，确认每一处修改都能追溯到用户需求。
4. 确认没有修改 `.git/`、第三方库、CubeMX 非用户区域或构建产物。
5. 确认新增 `.c` 文件已加入 `MDK-ARM/test.uvprojx`，新增头文件路径已配置。
6. 在可用环境中构建 Keil 工程 `MDK-ARM/test.uvprojx` 的 `test` 目标。
7. 构建环境不可用时，明确说明未执行编译，不得声称编译通过。

当 `UV4.exe` 可用时，可以使用类似以下命令进行命令行构建：

```text
UV4.exe -b MDK-ARM/test.uvprojx -t test
```

涉及硬件行为时，还应给出对应实机验证项，例如 LCD 刷新、触摸坐标、按键电平、DMA 完成、页面反复 Push / Pop 和任务栈高水位。

任务只有在实现范围明确、修改最小、代码已审查、可执行验证已完成或未完成原因已说明时，才算完成。
