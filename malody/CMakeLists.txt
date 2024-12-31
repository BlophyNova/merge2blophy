cmake_minimum_required(VERSION 3.30)

# 项目信息
project(mtbc VERSION 1.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(WIN32)
    add_definitions(-DWIN32)
endif()

if(MSVC)
    remove_definitions(/Za)
endif()

# 指定静态链接版本的 cJSON 和 miniz
add_subdirectory(../thirdparty/cJSON ${CMAKE_BINARY_DIR}/cJSON)
add_subdirectory(../thirdparty/miniz ${CMAKE_BINARY_DIR}/miniz)

# 添加可执行文件
add_executable(mtbc convert.c)

# 指定包含目录
target_include_directories(mtbc PRIVATE ../thirdparty/cJSON)
target_include_directories(mtbc PRIVATE ../thirdparty/miniz)
include_directories(${CMAKE_BINARY_DIR}/miniz)
include_directories(${CMAKE_BINARY_DIR}/cJSON)

# 确保静态链接 cJSON 和 miniz 库
target_link_libraries(mtbc PRIVATE cjson miniz)
