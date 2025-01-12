# 设置交叉编译工具链前缀
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 指定交叉编译工具的路径
set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)

# 设置交叉编译时的相关路径
set(CMAKE_FIND_ROOT_PATH arm-toolchain)

# 设置编译器选项，确保使用正确的架构
set(CMAKE_C_FLAGS "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard")

# 查找库时使用交叉编译根路径
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# 启用强制交叉编译的标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__ARM__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__ARM__")

# 防止使用系统自带的 CMake 工具链
set(CMAKE_SYSTEM_NAME Linux)
