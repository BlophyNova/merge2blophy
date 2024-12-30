cmake_minimum_required(VERSION 3.30)

# 项目信息
project(mtbc VERSION 1.0 LANGUAGES C CXX)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# 尝试查找 cJSON 库
find_package(cJSON CONFIG)

# 如果未找到 cJSON，则通过 FetchContent 下载并引入
if (NOT cJSON_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            cjson
            GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(cjson)
endif()

# 添加可执行文件
add_executable(mtbc convert.c)

# 将 cJSON 的包含目录添加到项目中
target_include_directories(mtbc PRIVATE ${cJSON_INCLUDE_DIRS})

# 链接 cJSON 库到可执行文件
target_link_libraries(mtbc PRIVATE cjson)
