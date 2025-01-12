#include "tools.h"

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