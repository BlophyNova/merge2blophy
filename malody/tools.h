#pragma once
#include "../includes/cross_platform.h"
// 获取目录下所有 .mc 文件，递归查找
int get_unique_subdirectory(const char *dir, char *subdir);
// 删除指定路径下的单个文件
int remove_file_custom(const char *filename);
// 删除目录
int delete_directory_custom(const char *dir_path);
int delete_directory_contents(const char *dir_path);
int get_mc_files(const char *dir, char ***mc_files, int *mc_file_count);