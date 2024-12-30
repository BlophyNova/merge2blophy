#include "../thirdparty/cJSON/cJSON.h"
#include "../thirdparty/miniz/miniz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>    // 包含 _mkdir
#include <io.h>        // 包含文件操作函数
#include <sys/stat.h>  // 包含 _stat 结构体的定义
typedef struct _stat stat;  // 在Windows中使用 _stat 来兼容 POSIX 的 stat
#define PATH_SEPARATOR '\\'
#define MKDIR(path) _mkdir(path)  // 使用 _mkdir 来创建目录
#define GET_ABS_PATH(path, abs_path) GetFullPathNameA(path, 1024, abs_path, NULL)  // 获取绝对路径
#define REMOVE_FILE(path) DeleteFileA(path)  // 删除文件
#define REMOVE_DIR(path) RemoveDirectoryA(path)  // 删除空目录

// 你可以考虑如下处理路径以防 Unicode 字符问题
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

// ANSI 颜色支持在 Windows 上
#ifdef _WIN32
#include <windows.h>
void enable_ansi_colors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_ansi_colors() {
}
#endif

// 帮助信息
void print_help(const char *program_name) {
    printf("用法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -f <文件路径>       指定输入的 .mc 文件路径\n");
    printf("  -o <输出路径>       指定输出的 Chart.json 文件路径\n");
    printf("  -z                  指定处理 .mcz 文件（将解压并处理其中的 .mc 文件）\n");
    printf("  -h                  显示帮助信息\n");
}

// 读取文件内容
char *read_file(const char *filename) {
    DEBUG_PRINT("读取文件: %s\n", filename);
    FILE *file = fopen(filename, "rb"); // Open in binary mode for cross-platform compatibility
    if (!file) {
        fprintf(stderr, RED "==> 无法打开文件: %s\n" RESET, filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(length + 1);
    if (!content) {
        fprintf(stderr, RED "==> 内存分配失败\n" RESET);
        fclose(file);
        return NULL;
    }

    size_t read_len = fread(content, 1, length, file);
    if (read_len != length) {
        fprintf(stderr, RED "==> 读取文件失败: %s\n" RESET, filename);
        free(content);
        fclose(file);
        return NULL;
    }

    content[length] = '\0';
    fclose(file);

    DEBUG_PRINT("文件读取成功，大小: %ld 字节\n", length);
    return content;
}

// 获取绝对路径
int get_absolute_path(const char *path, char *abs_path) {
    DEBUG_PRINT("获取绝对路径: %s\n", path);
    if (!GET_ABS_PATH(path, abs_path)) {
        fprintf(stderr, RED "==> 无法获取绝对路径: %s\n" RESET, path);
        return 0;
    }
    return 1;
}

// 创建目录
int create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (MKDIR(path) != 0) {
            fprintf(stderr, RED "==> 无法创建目录: %s\n" RESET, path);
            return 0;
        }
        printf(GREEN "==> 创建目录: %s\n" RESET, path);
    } else {
        DEBUG_PRINT("目录已存在: %s\n", path);
    }
    return 1;
}

// 解压 .mcz 文件 using miniz
int unzip_mcz(const char *mcz_path, const char *output_dir) {
    DEBUG_PRINT("解压 .mcz 文件: %s 到目录: %s\n", mcz_path, output_dir);
    char abs_output_dir[1024];
    if (!create_directory_if_not_exists(output_dir)) {
        return 0; // 如果无法创建目录，返回失败
    }

    // 获取解压目录的绝对路径
    if (!get_absolute_path(output_dir, abs_output_dir)) {
        return 0; // 获取绝对路径失败
    }

    // 使用 miniz 解压
    mz_zip_archive zip_archive = {0};

    if (!mz_zip_reader_init_file(&zip_archive, mcz_path, 0)) {
        fprintf(stderr, RED "==> 无法打开 .mcz 文件: %s\n" RESET, mcz_path);
        return 0;
    }

    int num_files = (int) mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            fprintf(stderr, RED "==> 无法获取文件信息: %d\n" RESET, i);
            mz_zip_reader_end(&zip_archive);
            return 0;
        }

        // 构建输出文件路径
        char output_file_path[1024];
        // Replace '/' and '\\' with PATH_SEPARATOR
        char normalized_filename[1024];
        strcpy(normalized_filename, file_stat.m_filename);
        for (char *p = normalized_filename; *p; p++) {
            if (*p == '/' || *p == '\\') {
                *p = PATH_SEPARATOR;
            }
        }
        snprintf(output_file_path, sizeof(output_file_path), "%s%c%s", abs_output_dir, PATH_SEPARATOR,
                 normalized_filename);

        // 检查是否为目录
        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            // 创建目录
            if (!create_directory_if_not_exists(output_file_path)) {
                mz_zip_reader_end(&zip_archive);
                return 0;
            }
            continue;
        }

        // 确保父目录存在
        char parent_dir[1024];
        strcpy(parent_dir, output_file_path);
        char *last_sep = strrchr(parent_dir, PATH_SEPARATOR);
        if (last_sep) {
            *last_sep = '\0';
            if (!create_directory_if_not_exists(parent_dir)) {
                mz_zip_reader_end(&zip_archive);
                return 0;
            }
        }

        // 解压文件
        if (!mz_zip_reader_extract_to_file(&zip_archive, i, output_file_path, 0)) {
            fprintf(stderr, RED "==> 解压文件失败: %s\n" RESET, output_file_path);
            mz_zip_reader_end(&zip_archive);
            return 0;
        }

        DEBUG_PRINT("解压文件: %s\n", output_file_path);
    }

    mz_zip_reader_end(&zip_archive);
    printf(GREEN "==> 解压成功到: %s\n" RESET, abs_output_dir);
    return 1;
}

// 获取目录下所有 .mc 文件，递归查找
int get_mc_files(const char *dir, char ***mc_files, int *mc_file_count) {
    DEBUG_PRINT("获取目录 %s 下所有 .mc 文件\n", dir);
#ifdef _WIN32
        WIN32_FIND_DATAA find_data;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char search_path[1024];
        snprintf(search_path, sizeof(search_path), "%s%c*", dir, PATH_SEPARATOR);

        hFind = FindFirstFileA(search_path, &find_data);
        if (hFind == INVALID_HANDLE_VALUE) {
            fprintf(stderr, RED "==> 无法打开目录: %s\n" RESET, dir);
            return 0;
        }

        int capacity = 10;
        *mc_files = malloc(sizeof(char *) * capacity);
        if (!*mc_files) {
            fprintf(stderr, RED "==> 内存分配失败\n" RESET);
            FindClose(hFind);
            return 0;
        }
        *mc_file_count = 0;

        do {
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                continue;

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // 如果是目录，则递归查找子目录
                char subdir[BUFFER_SIZE];
                snprintf(subdir, sizeof(subdir), "%s%c%s", dir, PATH_SEPARATOR, find_data.cFileName);
                char **subdir_mc_files = NULL;
                int subdir_mc_file_count = 0;
                if (get_mc_files(subdir, &subdir_mc_files, &subdir_mc_file_count)) {
                    for (int i = 0; i < subdir_mc_file_count; i++) {
                        if (*mc_file_count >= capacity) {
                            capacity *= 2;
                            char **temp = realloc(*mc_files, sizeof(char *) * capacity);
                            if (!temp) {
                                fprintf(stderr, RED "==> 内存分配失败\n" RESET);
                                FindClose(hFind);
                                return 0;
                            }
                            *mc_files = temp;
                        }
                        (*mc_files)[*mc_file_count] = subdir_mc_files[i];
                        (*mc_file_count)++;
                    }
                    free(subdir_mc_files);
                }
            } else {
                // 如果是 .mc 文件，加入到文件列表中
                if (strstr(find_data.cFileName, ".mc")) {
                    if (*mc_file_count >= capacity) {
                        capacity *= 2;
                        char **temp = realloc(*mc_files, sizeof(char *) * capacity);
                        if (!temp) {
                            fprintf(stderr, RED "==> 内存分配失败\n" RESET);
                            FindClose(hFind);
                            return 0;
                        }
                        *mc_files = temp;
                    }
                    char *file_name = strdup(find_data.cFileName);
                    if (file_name) {
                        (*mc_files)[*mc_file_count] = file_name;
                        (*mc_file_count)++;
                    }
                }
            }
        } while (FindNextFileA(hFind, &find_data) != 0);

        FindClose(hFind);
#else
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, RED "==> 无法打开目录: %s\n" RESET, dir);
        return 0;
    }

    struct dirent *entry;
    int capacity = 10;
    *mc_files = malloc(sizeof(char *) * capacity);
    if (!*mc_files) {
        fprintf(stderr, RED "==> 内存分配失败\n" RESET);
        closedir(d);
        return 0;
    }
    *mc_file_count = 0;

    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_DIR) {
            // 如果是目录，则递归查找子目录
            char subdir[BUFFER_SIZE];
            snprintf(subdir, sizeof(subdir), "%s%c%s", dir, PATH_SEPARATOR, entry->d_name);
            char **subdir_mc_files = NULL;
            int subdir_mc_file_count = 0;
            if (get_mc_files(subdir, &subdir_mc_files, &subdir_mc_file_count)) {
                for (int i = 0; i < subdir_mc_file_count; i++) {
                    if (*mc_file_count >= capacity) {
                        capacity *= 2;
                        char **temp = realloc(*mc_files, sizeof(char *) * capacity);
                        if (!temp) {
                            fprintf(stderr, RED "==> 内存分配失败\n" RESET);
                            closedir(d);
                            return 0;
                        }
                        *mc_files = temp;
                    }
                    (*mc_files)[*mc_file_count] = subdir_mc_files[i];
                    (*mc_file_count)++;
                }
                free(subdir_mc_files);
            }
        } else {
            // 如果是 .mc 文件，加入到文件列表中
            if (strstr(entry->d_name, ".mc")) {
                if (*mc_file_count >= capacity) {
                    capacity *= 2;
                    char **temp = realloc(*mc_files, sizeof(char *) * capacity);
                    if (!temp) {
                        fprintf(stderr, RED "==> 内存分配失败\n" RESET);
                        closedir(d);
                        return 0;
                    }
                    *mc_files = temp;
                }
                char *file_name = strdup(entry->d_name);
                if (file_name) {
                    (*mc_files)[*mc_file_count] = file_name;
                    (*mc_file_count)++;
                }
            }
        }
    }

    closedir(d);
#endif
    return 1;
}

// 递归查找包含 .mc 文件的目录
int get_unique_subdirectory(const char *dir, char *subdir) {
    DEBUG_PRINT("递归查找目录 %s 下的 .mc 文件\n", dir);
#ifdef _WIN32
        WIN32_FIND_DATAA find_data;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char search_path[1024];
        snprintf(search_path, sizeof(search_path), "%s%c*", dir, PATH_SEPARATOR);

        hFind = FindFirstFileA(search_path, &find_data);
        if (hFind == INVALID_HANDLE_VALUE) {
            fprintf(stderr, RED "==> 无法打开目录: %s\n" RESET, dir);
            return 0;
        }

        do {
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                continue;

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // 如果是目录，则递归查找
                char subdir_path[BUFFER_SIZE];
                snprintf(subdir_path, sizeof(subdir_path), "%s%c%s", dir, PATH_SEPARATOR, find_data.cFileName);
                if (get_unique_subdirectory(subdir_path, subdir)) {
                    FindClose(hFind);
                    return 1; // 找到包含 .mc 文件的目录，返回
                }
            } else {
                if (strstr(find_data.cFileName, ".mc")) {
                    // 如果是 .mc 文件，返回当前目录
                    snprintf(subdir, BUFFER_SIZE, "%s", dir);
                    FindClose(hFind);
                    return 1;
                }
            }
        } while (FindNextFileA(hFind, &find_data) != 0);

        FindClose(hFind);
#else
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, RED "==> 无法打开目录: %s\n" RESET, dir);
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_DIR) {
            // 如果是目录，则递归查找
            char subdir_path[BUFFER_SIZE];
            snprintf(subdir_path, sizeof(subdir_path), "%s%c%s", dir, PATH_SEPARATOR, entry->d_name);
            if (get_unique_subdirectory(subdir_path, subdir)) {
                closedir(d);
                return 1; // 找到包含 .mc 文件的目录，返回
            }
        } else {
            if (strstr(entry->d_name, ".mc")) {
                // 如果是 .mc 文件，返回当前目录
                snprintf(subdir, BUFFER_SIZE, "%s", dir);
                closedir(d);
                return 1;
            }
        }
    }

    closedir(d);
#endif
    return 0; // 如果没有找到 .mc 文件
}

// 让用户选择 .mc 文件
char *choose_mc_file(char **mc_files, int mc_file_count) {
    DEBUG_PRINT("让用户选择 .mc 文件\n");
    if (mc_file_count == 0) {
        fprintf(stderr, RED "==> 没有找到 .mc 文件\n" RESET);
        return NULL;
    }

    if (mc_file_count == 1) {
        printf(GREEN "  => 只有一个 .mc 文件，自动选择: %s\n" RESET, mc_files[0]);
        return mc_files[0];
    }

    printf("  => 请选择一个 .mc 文件:\n");
    for (int i = 0; i < mc_file_count; i++) {
        printf("    %d: %s\n", i + 1, mc_files[i]);
    }

    char *endptr;
    int choice = 0;
    while (1) {
        char input[BUFFER_SIZE];
        printf("  输入文件编号: ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character if present
            char *newline = strchr(input, '\n');
            if (newline) *newline = '\0';

            errno = 0;
            choice = (int) strtol(input, &endptr, 10);
            if (endptr == input || (*endptr != '\0' && *endptr != '\r') || errno != 0 || choice < 1 || choice >
                mc_file_count) {
                fprintf(stderr, RED "  无效的选择，请重新输入\n" RESET);
            } else {
                break;
            }
        }
    }

    return mc_files[choice - 1];
}

// 删除指定路径下的单个文件
int remove_file_custom(const char *filename) {
#ifdef _WIN32
        if (!REMOVE_FILE(filename)) {
            fprintf(stderr, RED "==> Error deleting file: %s\n" RESET, filename);
            return -1;
        }
#else
    if (REMOVE_FILE(filename) != 0) {
        perror("==> Error deleting file");
        return -1;
    }
#endif
    return 0;
}

int delete_directory_contents(const char *dir_path);

// 删除目录
int delete_directory_custom(const char *dir_path) {
    // 删除目录中的所有内容
    if (delete_directory_contents(dir_path) != 0) {
        return -1;
    }

    // 删除空目录
#ifdef _WIN32
        if (!REMOVE_DIR(dir_path)) {
#else
    if (REMOVE_DIR(dir_path) != 0) {
#endif
        perror("RemoveDirectory");
        return -1;
    }
    return 0;
}

int delete_directory_contents(const char *dir_path) {
#ifdef _WIN32
        WIN32_FIND_DATAA find_data;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char search_path[1024];
        snprintf(search_path, sizeof(search_path), "%s%c*", dir_path, PATH_SEPARATOR);

        hFind = FindFirstFileA(search_path, &find_data);
        if (hFind == INVALID_HANDLE_VALUE) {
            perror("FindFirstFileA");
            return -1;
        }

        do {
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                continue;

            char file_path[1024];
            snprintf(file_path, sizeof(file_path), "%s%c%s", dir_path, PATH_SEPARATOR, find_data.cFileName);

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // 如果是子目录，递归删除
                if (delete_directory_custom(file_path) != 0) {
                    FindClose(hFind);
                    return -1;
                }
            } else {
                // 删除文件
                if (!REMOVE_FILE(file_path)) {
                    fprintf(stderr, RED "==> Error deleting file: %s\n" RESET, file_path);
                    FindClose(hFind);
                    return -1;
                }
            }
        } while (FindNextFileA(hFind, &find_data) != 0);

        FindClose(hFind);
#else
    DIR *d = opendir(dir_path);
    if (!d) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s%c%s", dir_path, PATH_SEPARATOR, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // 如果是子目录，递归删除
            if (delete_directory_custom(file_path) != 0) {
                closedir(d);
                return -1;
            }
        } else {
            // 删除文件
            if (REMOVE_FILE(file_path) != 0) {
                perror("remove");
                closedir(d);
                return -1;
            }
        }
    }

    closedir(d);
#endif
    return 0;
}

// 读取标准输入
char *read_stdin_custom() {
    size_t buffer_size = 1024;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, RED "==> 内存分配失败\n" RESET);
        return NULL;
    }

    size_t index = 0;
    int ch;

    // 从标准输入读取字符直到遇到换行符或者缓冲区已满
    while ((ch = getchar()) != EOF && ch != '\n') {
        buffer[index++] = (char) ch;

        // 如果缓冲区满了，扩展缓冲区
        if (index >= buffer_size - 1) {
            size_t new_buffer_size = buffer_size * 2;
            char *new_buffer = realloc(buffer, new_buffer_size);
            if (!new_buffer) {
                fprintf(stderr, RED "==> 内存扩展失败\n" RESET);
                free(buffer); // 释放原来的内存
                return NULL;
            }
            buffer = new_buffer; // 更新指针
            buffer_size = new_buffer_size; // 更新缓冲区大小
        }
    }

    buffer[index] = '\0'; // 添加字符串结束符
    return buffer;
}

// 提取最后的 offset 值并处理
double extract_last_offset(const cJSON *notes) {
    if (!cJSON_IsArray(notes)) {
        fprintf(stderr, RED "==> note 字段不是数组\n" RESET);
        return 0;
    }

    const int note_count = cJSON_GetArraySize(notes);
    if (note_count == 0) {
        fprintf(stderr, RED "==> note 数组为空\n" RESET);
        return 0;
    }

    const cJSON *last_note = cJSON_GetArrayItem(notes, note_count - 1);
    const cJSON *offset = cJSON_GetObjectItem(last_note, "offset");
    if (!cJSON_IsNumber(offset)) {
        fprintf(stderr, RED "==> 未找到有效的 offset 值\n" RESET);
        return 0;
    }

    const double result = offset->valuedouble;

    printf("  => Offset: %lf\n", result);

    return result; // 将 offset 除以 1000
}

// 生成 bpmList
cJSON *create_bpm_list(const cJSON *time) {
    if (!cJSON_IsArray(time)) {
        fprintf(stderr, RED "==> time 字段不是数组\n" RESET);
        return cJSON_CreateArray();
    }

    cJSON *bpm_list = cJSON_CreateArray();
    const int time_count = cJSON_GetArraySize(time);

    for (int i = 0; i < time_count; i++) {
        const cJSON *entry = cJSON_GetArrayItem(time, i);
        const cJSON *beat = cJSON_GetObjectItem(entry, "beat");
        const cJSON *bpm = cJSON_GetObjectItem(entry, "bpm");

        if (!cJSON_IsArray(beat) || !cJSON_IsNumber(bpm)) {
            fprintf(stderr, RED "==> time 数组中的元素格式不正确\n" RESET);
            continue;
        }

        const int a = cJSON_GetArrayItem(beat, 0)->valueint;
        const int b = cJSON_GetArrayItem(beat, 1)->valueint;
        const int c = cJSON_GetArrayItem(beat, 2)->valueint;

        const double e = (c != 0) ? a + (double) b / c : a;

        cJSON *bpm_entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(bpm_entry, "integer", a);
        cJSON_AddNumberToObject(bpm_entry, "molecule", b);
        cJSON_AddNumberToObject(bpm_entry, "denominator", c);
        cJSON_AddNumberToObject(bpm_entry, "currentBPM", bpm->valuedouble);
        cJSON_AddNumberToObject(bpm_entry, "ThisStartBPM", e);

        cJSON_AddItemToArray(bpm_list, bpm_entry);
    }

    printf("  => BPM List解析完成.\n");

    return bpm_list;
}

// 创建 Chart.json 文件
void create_chart_json(const double offset, cJSON *bpm_list, const char *output_path) {
    cJSON *chart = cJSON_CreateObject();

    cJSON_AddNumberToObject(chart, "yScale", 6.0);
    cJSON_AddNumberToObject(chart, "beatSubdivision", 4);
    cJSON_AddNumberToObject(chart, "verticalSubdivision", 16);
    cJSON_AddNumberToObject(chart, "eventVerticalSubdivision", 10);
    cJSON_AddNumberToObject(chart, "playSpeed", 1.0);
    cJSON_AddNumberToObject(chart, "offset", offset);
    cJSON_AddNumberToObject(chart, "musicLength", -1.0);
    cJSON_AddBoolToObject(chart, "loopPlayBack", 1);
    cJSON_AddItemToObject(chart, "bpmList", bpm_list);
    printf(GREEN "  => 文件初始化完成.\n" RESET);

    // 写入文件
    FILE *file = fopen(output_path, "w");
    if (!file) {
        fprintf(stderr, RED "==> 无法创建文件: %s\n" RESET, output_path);
        cJSON_Delete(chart);
        return;
    }

    char *json_string = cJSON_Print(chart);
    if (!json_string) {
        fprintf(stderr, RED "==> JSON 格式化失败\n" RESET);
        fclose(file);
        cJSON_Delete(chart);
        return;
    }

    fprintf(file, "%s\n", json_string);
    fclose(file);

    printf(GREEN "==> 保存成功, 文件位于: %s\n" RESET, output_path);

    // 清理内存
    free(json_string);
    cJSON_Delete(chart);

    printf(GREEN "==> 临时文件已删除\n" RESET);
}

int main(const int argc, char *argv[]) {
    enable_ansi_colors(); // Enable ANSI colors on Windows

    const char *input_path = NULL;
    const char *output_path = "Chart.json";
    int is_mcz = 0;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            input_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "-z") == 0) {
            is_mcz = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, RED "==> 未知选项: %s\n" RESET, argv[i]);
            return EXIT_FAILURE;
        }
    }

    // 检查 output_path 是否是目录路径，如果是目录，则附加文件名 "Chart.json"
    if (output_path) {
#ifdef _WIN32
            DWORD attr = GetFileAttributesA(output_path);
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
#else
        struct stat st;
        if (stat(output_path, &st) == 0 && S_ISDIR(st.st_mode)) {
#endif
            // 如果是目录路径，附加文件名 "Chart.json"
            char temp_path[1024];
            snprintf(temp_path, sizeof(temp_path), "%s%cChart.json", output_path, PATH_SEPARATOR);
            output_path = strdup(temp_path); // 更新 output_path
        }
    } else {
        output_path = "Chart.json"; // 如果没有指定 -o，则默认使用 "Chart.json"
    }

    DEBUG_PRINT("程序启动，输入路径: %s, 输出路径: %s\n", input_path ? input_path : "(未指定)", output_path);

    // 读取输入文件内容
    char *input_content = NULL;
    if (input_path) {
        if (is_mcz) {
            // 处理 .mcz 文件
            char abs_input_path[1024];
            if (!get_absolute_path(input_path, abs_input_path)) {
                return EXIT_FAILURE;
            }

            // 生成解压目录路径
            char unzip_dir[1024];
            snprintf(unzip_dir, sizeof(unzip_dir), "%s_unzip", abs_input_path);

            // 解压文件
            if (!unzip_mcz(abs_input_path, unzip_dir)) {
                return EXIT_FAILURE;
            }

            // 获取唯一的子目录
            char subdir[BUFFER_SIZE];
            if (!get_unique_subdirectory(unzip_dir, subdir)) {
                fprintf(stderr, RED "==> 找到多个子目录或未找到包含 .mc 文件的目录，无法继续\n" RESET);
                // 清理解压目录
                delete_directory_custom(unzip_dir);
                return EXIT_FAILURE;
            }

            // 获取所有 .mc 文件
            char **mc_files = NULL;
            int mc_file_count = 0;
            if (!get_mc_files(subdir, &mc_files, &mc_file_count)) {
                // 清理解压目录
                delete_directory_custom(unzip_dir);
                return EXIT_FAILURE;
            }

            // 让用户选择一个文件
            char *mc_file = choose_mc_file(mc_files, mc_file_count);
            if (!mc_file) {
                // 清理解压目录
                delete_directory_custom(unzip_dir);
                return EXIT_FAILURE;
            }

            // 获取 mc 文件的完整路径并处理
            char mc_file_path[BUFFER_SIZE];
            snprintf(mc_file_path, sizeof(mc_file_path), "%s%c%s", subdir, PATH_SEPARATOR, mc_file);

            printf(GREEN "==> 处理文件: %s\n" RESET, mc_file_path);

            char *mc_content = read_file(mc_file_path);
            if (!mc_content) {
                // 清理解压目录
                delete_directory_custom(unzip_dir);
                return EXIT_FAILURE;
            }

            cJSON *json = cJSON_Parse(mc_content);
            free(mc_content);
            if (!json) {
                fprintf(stderr, RED "==> JSON 解析失败\n" RESET);
                // 清理解压目录
                delete_directory_custom(unzip_dir);
                return EXIT_FAILURE;
            }

            // 提取数据并生成 Chart.json
            const cJSON *notes = cJSON_GetObjectItem(json, "note");
            const cJSON *time = cJSON_GetObjectItem(json, "time");

            const double offset = extract_last_offset(notes);
            cJSON *bpm_list = create_bpm_list(time);

            DEBUG_PRINT("OUTPUT_PATH: %s", output_path);

            create_chart_json(offset, bpm_list, output_path);

            // 删除解压目录
            if (delete_directory_custom(unzip_dir) != 0) {
                fprintf(stderr, RED "==> 无法删除解压目录: %s\n" RESET, unzip_dir);
                cJSON_Delete(json);
                return EXIT_FAILURE;
            }

            // 清理 JSON
            cJSON_Delete(json);
        } else {
            // 直接处理 .mc 文件
            input_content = read_file(input_path);
            if (input_content == NULL) {
                return EXIT_FAILURE;
            }

            // 解析 JSON 数据
            cJSON *json = cJSON_Parse(input_content);
            free(input_content);
            if (json == NULL) {
                fprintf(stderr, RED "==> 无法解析 JSON 数据\n" RESET);
                return EXIT_FAILURE;
            }

            // 提取数据并生成 Chart.json
            const cJSON *notes = cJSON_GetObjectItem(json, "note");
            const cJSON *time = cJSON_GetObjectItem(json, "time");

            const double offset = extract_last_offset(notes);
            cJSON *bpm_list = create_bpm_list(time);

            create_chart_json(offset, bpm_list, output_path);

            // 清理内存
            cJSON_Delete(json);
        }
    } else {
        // 从标准输入读取
        input_content = read_stdin_custom();
        if (input_content == NULL) {
            return EXIT_FAILURE;
        }

        // 解析 JSON 数据
        cJSON *json = cJSON_Parse(input_content);
        free(input_content);
        if (json == NULL) {
            fprintf(stderr, RED "==> 无法解析 JSON 数据\n" RESET);
            return EXIT_FAILURE;
        }

        // 提取数据并生成 Chart.json
        const cJSON *notes = cJSON_GetObjectItem(json, "note");
        const cJSON *time = cJSON_GetObjectItem(json, "time");

        const double offset = extract_last_offset(notes);
        cJSON *bpm_list = create_bpm_list(time);

        create_chart_json(offset, bpm_list, output_path);

        // 清理内存
        cJSON_Delete(json);
    }

    return 0;
}
