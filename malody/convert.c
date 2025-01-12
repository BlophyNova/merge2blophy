#include "../includes/cross_platform.h"
#include "convert.h"

// 帮助信息
void print_help(const char *program_name) {
    printf("用法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -f <文件路径>       指定输入的 .mc 文件路径\n");
    printf("  -o <输出路径>       指定输出的 Chart.json 文件路径\n");
    printf("  -z                  指定处理 .mcz 文件（将解压并处理其中的 .mc 文件）\n");
    printf("  -h                  显示帮助信息\n");
}

// ANSI 颜色支持在 Windows 上
#ifdef _WIN32
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

// 解压 .mcz 文件
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

    const unsigned int num_files = mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            fprintf(stderr, RED "==> 无法获取文件信息: %d\n" RESET, i);
            mz_zip_reader_end(&zip_archive);
            return 0;
        }

        // 构建输出文件路径
        char output_file_path[1024];
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

// 让用户选择 .mc 文件
char *choose_mc_file(char **mc_files, int mc_file_count) {
    DEBUG_PRINT("让用户选择 .mc 文件\n");
    if (mc_file_count == 0) {
        fprintf(stderr, RED "==> 没有找到 .mc 文件\n" RESET);
        return NULL;
    }

    if (mc_file_count == 1) {
        printf(BLUE " -> 只有一个 .mc 文件，自动选择: %s\n" RESET, mc_files[0]);
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

    printf(BLUE "  -> Offset: %lf\n", result);

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

    printf(BLUE "  -> BPM List解析完成.\n");

    return bpm_list;
}

cJSON *createBeatObject(const double startBPM, const double endBPM) {
    cJSON *beats = cJSON_CreateObject();
    cJSON_AddNumberToObject(beats, "integer", 0);
    cJSON_AddNumberToObject(beats, "molecule", 0);
    cJSON_AddNumberToObject(beats, "denominator", 1);
    cJSON_AddNumberToObject(beats, "currentBPM", startBPM);
    cJSON_AddNumberToObject(beats, "ThisStartBPM", endBPM);
    return beats;
}

void add_boxes_to_chart(cJSON *chart) {
    // 创建 "boxes" 数组
    cJSON *boxes = cJSON_CreateArray();

    // 创建 box
    cJSON *box = cJSON_CreateObject();
    cJSON *boxEvents = cJSON_CreateObject();

    // 添加 boxEvents 的各个子项
    cJSON *speed = cJSON_CreateArray();
    cJSON *moveX = cJSON_CreateArray();
    cJSON *moveY = cJSON_CreateArray();
    cJSON *rotate = cJSON_CreateArray();
    cJSON *alpha = cJSON_CreateArray();
    cJSON *scaleX = cJSON_CreateArray();
    cJSON *scaleY = cJSON_CreateArray();
    cJSON *centerX = cJSON_CreateArray();
    cJSON *centerY = cJSON_CreateArray();
    cJSON *lineAlpha = cJSON_CreateArray();

    // 添加 "speed" 数组的示例项
    cJSON *speedObject = cJSON_CreateObject();
    cJSON_AddItemToObject(speedObject, "startBeats", createBeatObject(0.0, 0.0));
    cJSON_AddItemToObject(speedObject, "endBeats", createBeatObject(1.0, 1.0));
    cJSON_AddNumberToObject(speedObject, "startValue", 3.0);
    cJSON_AddNumberToObject(speedObject, "endValue", 3.0);
    cJSON_AddNumberToObject(speedObject, "curveIndex", 0);
    cJSON_AddBoolToObject(speedObject, "IsSelected", 0);
    cJSON_AddItemToArray(speed, speedObject);

    // 其它数组（moveX、moveY、rotate 等）可以使用类似的方法添加数据
    cJSON_AddItemToObject(boxEvents, "speed", speed);
    cJSON_AddItemToObject(boxEvents, "moveX", moveX);
    cJSON_AddItemToObject(boxEvents, "moveY", moveY);
    cJSON_AddItemToObject(boxEvents, "rotate", rotate);
    cJSON_AddItemToObject(boxEvents, "alpha", alpha);
    cJSON_AddItemToObject(boxEvents, "scaleX", scaleX);
    cJSON_AddItemToObject(boxEvents, "scaleY", scaleY);
    cJSON_AddItemToObject(boxEvents, "centerX", centerX);
    cJSON_AddItemToObject(boxEvents, "centerY", centerY);
    cJSON_AddItemToObject(boxEvents, "lineAlpha", lineAlpha);

    // 添加其它字段（LengthSpeed 等）
    cJSON_AddNumberToObject(boxEvents, "LengthSpeed", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthMoveX", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthMoveY", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthRotate", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthAlpha", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthScaleX", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthScaleY", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthCenterX", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthCenterY", 1);
    cJSON_AddNumberToObject(boxEvents, "LengthLineAlpha", 1);

    cJSON_AddItemToObject(box, "boxEvents", boxEvents);

    // 添加 "lines" 数组
    cJSON *lines = cJSON_CreateArray();
    for (int i = 0; i < 5; i++) {
        cJSON *line = cJSON_CreateObject();
        cJSON_AddItemToObject(line, "onlineNotes", cJSON_CreateArray());
        cJSON_AddItemToObject(line, "onlineNotesLength", cJSON_CreateNumber(0));
        cJSON_AddItemToObject(line, "offlineNotes", cJSON_CreateArray());
        cJSON_AddItemToObject(line, "offlineNotesLength", cJSON_CreateNumber(0));
        cJSON_AddItemToArray(lines, line);
    }

    cJSON_AddItemToObject(box, "lines", lines);

    // 将 box 添加到 boxes 数组中
    cJSON_AddItemToArray(boxes, box);

    // 将 boxes 添加到 chart 对象中
    cJSON_AddItemToObject(chart, "boxes", boxes);
}

void create_chart_json(const double offset, cJSON *bpm_list, const char *output_path) {
    cJSON *chart = cJSON_CreateObject();

    cJSON_AddNumberToObject(chart, "yScale", 6.0);
    cJSON_AddNumberToObject(chart, "beatSubdivision", 4);
    cJSON_AddNumberToObject(chart, "verticalSubdivision", 16);
    cJSON_AddNumberToObject(chart, "eventVerticalSubdivision", 10);
    cJSON_AddNumberToObject(chart, "playSpeed", 1.0);
    cJSON_AddNumberToObject(chart, "offset", offset / 1000);
    cJSON_AddNumberToObject(chart, "musicLength", -1.0);
    cJSON_AddBoolToObject(chart, "loopPlayBack", 1);
    cJSON_AddItemToObject(chart, "bpmList", bpm_list);

    add_boxes_to_chart(chart);

    printf(BLUE " -> 文件初始化完成.\n" RESET);

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
        const DWORD attr = GetFileAttributesA(output_path);
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
    if (!input_path) {
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
    } else {
        if (!is_mcz) {
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
        } else {
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
        }
    }

    return 0;
}
