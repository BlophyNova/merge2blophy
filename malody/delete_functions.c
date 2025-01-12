#include "tools.h"

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
