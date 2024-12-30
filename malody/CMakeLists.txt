cmake_minimum_required(VERSION 3.30)

# 项目信息
project(mtbc VERSION 1.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(MSVC)
    remove_definitions(/Za)
endif()

add_subdirectory(../thirdparty/cJSON ${CMAKE_BINARY_DIR}/cJSON)

# 添加可执行文件
add_executable(mtbc convert.c)

# 链接 cJSON 库到可执行文件
target_link_libraries(mtbc PRIVATE cjson)

# 指定包含目录
target_include_directories(mtbc PRIVATE ../thirdparty/cJSON)
