#include <cjson/cJSON.h> // 需要使用 cJSON 库解析 JSON
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 8192 // 定义缓冲区大小

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"

// 帮助信息
void print_help(const char *program_name) {
    printf("用法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -f <文件路径>       指定输入的 .mc 文件路径\n");
    printf("  -o <输出路径>       指定输出的 Chart.json 文件路径\n");
    printf("  -h                  显示帮助信息\n");
    printf("如果未指定 -f，则从标准输入读取 JSON 数据。\n");
    printf("如果未指定 -o，则输出到当前目录的 Chart.json 文件。\n");
}

// 读取文件内容
char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, RED "==> 无法打开文件: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(length + 1);
    if (!content) {
        fprintf(stderr, RED "==> 内存分配失败\n");
        fclose(file);
        return NULL;
    }

    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);

    return content;
}

// 从标准输入读取内容
char *read_stdin() {
    char *content = malloc(BUFFER_SIZE);
    if (!content) {
        fprintf(stderr, RED "==> 内存分配失败\n");
        return NULL;
    }

    size_t total_read = 0;
    size_t read_size;

    while ((read_size = fread(content + total_read, 1, BUFFER_SIZE - total_read, stdin)) > 0) {
        total_read += read_size;
        if (total_read >= BUFFER_SIZE) {
            fprintf(stderr, RED "==> 输入数据超过缓冲区大小\n");
            free(content);
            return NULL;
        }
    }

    content[total_read] = '\0';
    return content; // 返回分配的内存
}

// 提取最后的 offset 值并处理
double extract_last_offset(const cJSON *notes) {
    if (!cJSON_IsArray(notes)) {
        fprintf(stderr, RED "==> note 字段不是数组\n");
        return 0;
    }

    const int note_count = cJSON_GetArraySize(notes);
    if (note_count == 0) {
        fprintf(stderr, RED "==> note 数组为空\n");
        return 0;
    }

    const cJSON *last_note = cJSON_GetArrayItem(notes, note_count - 1);
    const cJSON *offset = cJSON_GetObjectItem(last_note, "offset");
    if (!cJSON_IsNumber(offset)) {
        fprintf(stderr, RED "==> 未找到有效的 offset 值\n");
        return 0;
    }

    const double result = offset->valuedouble;

    printf("  => Offset: %lf\n", result);

    return result; // 将 offset 除以 1000
}

// 生成 bpmList
cJSON *create_bpm_list(const cJSON *time) {
    if (!cJSON_IsArray(time)) {
        fprintf(stderr, RED "==> time 字段不是数组\n");
        return cJSON_CreateArray();
    }

    cJSON *bpm_list = cJSON_CreateArray();
    const int time_count = cJSON_GetArraySize(time);

    for (int i = 0; i < time_count; i++) {
        const cJSON *entry = cJSON_GetArrayItem(time, i);
        const cJSON *beat = cJSON_GetObjectItem(entry, "beat");
        const cJSON *bpm = cJSON_GetObjectItem(entry, "bpm");

        if (!cJSON_IsArray(beat) || !cJSON_IsNumber(bpm)) {
            fprintf(stderr, RED "==> time 数组中的元素格式不正确\n");
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

    printf("  => BPM List解析完成.");

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
    printf(GREEN "  => 文件初始化完成.\n");

    // 写入文件
    FILE *file = fopen(output_path, "w");
    if (!file) {
        fprintf(stderr, RED "==> 无法创建文件: %s\n", output_path);
        cJSON_Delete(chart);
        return;
    }

    char *json_string = cJSON_Print(chart);
    fprintf(file, "%s\n", json_string);
    fclose(file);

    printf(GREEN "==> 保存成功, 文件位于: %s\n", output_path);

    // 清理内存
    free(json_string);
    cJSON_Delete(chart);
}

int main(const int argc, char *argv[]) {
    char *input = NULL;
    const char *input_path = NULL;
    const char *output_path = "Chart.json";

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            input_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, RED "==> 未知选项: %s\n", argv[i]);
            print_help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // 读取输入数据
    if (input_path) {
        input = read_file(input_path);
    } else {
        printf(GREEN "==> 正在尝试从STDIN读取数据\n");
        input = read_stdin();
    }

    if (!input) {
        fprintf(stderr, RED"==> 未能读取输入数据\n");
        return EXIT_FAILURE;
    }

    // 解析 JSON 数据
    cJSON *json = cJSON_Parse(input);
    free(input); // 释放动态分配的内存

    if (!json) {
        fprintf(stderr,RED "==> JSON 解析失败\n");
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
    return EXIT_SUCCESS;
}
