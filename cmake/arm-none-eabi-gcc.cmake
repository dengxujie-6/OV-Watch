set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

find_program(CMAKE_C_COMPILER NAMES ${TOOLCHAIN_PREFIX}gcc HINTS "$ENV{ARM_NONE_EABI_PATH}/bin")
find_program(CMAKE_ASM_COMPILER NAMES ${TOOLCHAIN_PREFIX}gcc HINTS "$ENV{ARM_NONE_EABI_PATH}/bin")
find_program(CMAKE_OBJCOPY NAMES ${TOOLCHAIN_PREFIX}objcopy HINTS "$ENV{ARM_NONE_EABI_PATH}/bin")
find_program(CMAKE_SIZE NAMES ${TOOLCHAIN_PREFIX}size HINTS "$ENV{ARM_NONE_EABI_PATH}/bin")

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR
        "arm-none-eabi-gcc not found. Install Arm GNU Toolchain and either add its bin directory to PATH "
        "or set ARM_NONE_EABI_PATH to the toolchain root.")
endif()

if(NOT CMAKE_OBJCOPY)
    message(FATAL_ERROR "arm-none-eabi-objcopy not found.")
endif()

if(NOT CMAKE_SIZE)
    message(FATAL_ERROR "arm-none-eabi-size not found.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
