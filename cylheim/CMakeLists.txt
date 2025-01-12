cmake_minimum_required(VERSION 3.5)

# 项目信息
project(cytbc VERSION 1.3 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if (WIN32)
    add_definitions(-DWIN32)
endif ()

set(BUILD_SHARED_LIBS OFF)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS OFF)
set(BUILD_HEADER_ONLY OFF)

# 添加子模块
add_subdirectory(../thirdparty/cJSON ${CMAKE_BINARY_DIR}/cJSON)
add_subdirectory(../thirdparty/miniz ${CMAKE_BINARY_DIR}/miniz)

# 添加可执行文件
add_executable(cytbc
        ../includes/cross_platform.h
        convert.c
        convert.h
        process_tempo.c
        process_tempo.h
        create_bpmlist.c
        create_bpmlist.h
)

target_compile_definitions(cytbc PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# 设置 include 目录
target_include_directories(cytbc PRIVATE ../thirdparty/cJSON ../thirdparty/miniz)
include_directories(${CMAKE_BINARY_DIR}/miniz)
include_directories(${CMAKE_BINARY_DIR}/cJSON)

# 链接库
target_link_libraries(cytbc PRIVATE cjson miniz)

# 设置 RPATH
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# 强制在构建时设置 RPATH
set(CMAKE_BUILD_RPATH "$ORIGIN")
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
