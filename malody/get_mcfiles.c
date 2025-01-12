#include "tools.h"

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
                char *file_name = _strdup(find_data.cFileName);
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
