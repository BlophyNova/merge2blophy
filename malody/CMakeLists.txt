cmake_minimum_required(VERSION 3.5)

# 项目信息
project(mtbc VERSION 1.3 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if (WIN32)
    add_definitions(-DWIN32)
endif ()

set(BUILD_SHARED_LIBS OFF)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS OFF)
set(BUILD_HEADER_ONLY OFF)

if (ARM_BUILD)
    message(STATUS "Cross-compiling for ARM architecture")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/arm-toolchain.cmake)
endif ()

# 添加子模块
add_subdirectory(../thirdparty/cJSON ${CMAKE_BINARY_DIR}/cJSON)
add_subdirectory(../thirdparty/miniz ${CMAKE_BINARY_DIR}/miniz)

# 添加可执行文件
add_executable(mtbc
        ../includes/cross_platform.h
        convert.h
        tools.h
        convert.c
        delete_functions.c
        get_subdirectory.c
        get_mcfiles.c
)

target_compile_definitions(mtbc PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# 设置 include 目录
target_include_directories(mtbc PRIVATE ../thirdparty/cJSON ../thirdparty/miniz)
include_directories(${CMAKE_BINARY_DIR}/miniz)
include_directories(${CMAKE_BINARY_DIR}/cJSON)

# 链接库
target_link_libraries(mtbc PRIVATE cjson miniz)

# 设置 RPATH
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# 强制在构建时设置 RPATH
set(CMAKE_BUILD_RPATH "$ORIGIN")
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
