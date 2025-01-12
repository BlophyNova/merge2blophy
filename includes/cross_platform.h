#pragma once

#ifdef _WIN32
#include <windows.h>
#include <direct.h>    // 包含 _mkdir
#include <sys/stat.h>  // 包含 _stat 结构体的定义
#define PATH_SEPARATOR '\\'
#define MKDIR(path) _mkdir(path)  // 使用 _mkdir 来创建目录
#define GET_ABS_PATH(path, abs_path) GetFullPathNameA(path, 1024, abs_path, NULL)  // 获取绝对路径
#define REMOVE_FILE(path) DeleteFileA(path)  // 删除文件
#define REMOVE_DIR(path) RemoveDirectoryA(path)  // 删除空目录
#define REMOVE_FILE_UTF16(path) DeleteFileW(path)  // 支持 UTF-16 编码路径
#define REMOVE_DIR_UTF16(path) RemoveDirectoryW(path)  // 支持 UTF-16 编码路径
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#define MKDIR(path) mkdir(path, 0700)
#define GET_ABS_PATH(path, abs_path) realpath(path, abs_path)
#define REMOVE_FILE(path) remove(path)
#define REMOVE_DIR(path) rmdir(path)
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "miniz.h"
#include <errno.h>

// 定义缓冲区大小
#define BUFFER_SIZE 8192

// 调试输出宏
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "DEBUG: " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"
