# GCC / CMake 构建说明

这套构建文件和 Keil 工程并行存在，不修改 `MDK-ARM/test.uvprojx`，输出目录默认放在 `build/gcc/`。

## 1. 需要安装的工具

先安装以下工具，并把对应 `bin` 目录加入 `PATH`：

- Arm GNU Toolchain：提供 `arm-none-eabi-gcc`
- Ninja，或继续使用当前机器已有的 `mingw32-make`
- STM32CubeProgrammer：用于命令行下载，可选

当前工程已经有 CMake：

```powershell
cmake --version
```

安装好 Arm GNU Toolchain 后，确认：

```powershell
arm-none-eabi-gcc --version
arm-none-eabi-objcopy --version
arm-none-eabi-size --version
```

如果不想改系统 `PATH`，也可以设置工具链根目录：

```powershell
$env:ARM_NONE_EABI_PATH="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.3 rel1"
```

## 2. 补 FreeRTOS GCC port

当前 Keil 工程只带了 ARMCC/RVDS 版本的 FreeRTOS port：

```text
Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM4F/
```

GCC 构建需要补官方 FreeRTOS V10.3.1 的目录：

```text
Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/
```

至少应包含：

```text
port.c
portmacro.h
```

不要把 RVDS 目录直接拿给 GCC 用；里面有 ARMCC 专用内联汇编语法。

## 3. 配置与编译

推荐使用 Ninja：

```powershell
cmake -S . -B build/gcc -G Ninja "-DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gcc -j
```

如果还没有 Ninja，可以用当前机器已有的 `mingw32-make`：

```powershell
cmake -S . -B build/gcc -G "MinGW Makefiles" "-DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake" "-DCMAKE_MAKE_PROGRAM=E:/CodeBlocks/MinGW/bin/mingw32-make.exe" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gcc -j
```

成功后会生成：

```text
build/gcc/test.elf
build/gcc/test.hex
build/gcc/test.bin
build/gcc/test.map
```

## 4. 下载到板子

使用 `hex` 下载：

```powershell
STM32_Programmer_CLI.exe -c port=SWD -w build/gcc/test.hex -v -rst
```

使用 `bin` 下载时需要指定 Flash 起始地址：

```powershell
STM32_Programmer_CLI.exe -c port=SWD -w build/gcc/test.bin 0x08000000 -v -rst
```

## 5. 和 Keil 的关系

这套 GCC 构建不会影响 Keil：

```text
Keil 输出：MDK-ARM/test/test.axf
GCC 输出：build/gcc/test.elf / test.hex / test.bin
```

如果要回 Keil 下载，需要在 Keil 里重新 Build/Download，Keil 默认不会自动下载 `build/gcc/` 里的文件。
