#include "convert.h"

// 帮助信息
void print_help(const char *program_name) {
    printf("用法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -f <文件路径>       指定输入的chart_*.txt 文件路径\n");
    printf("  -o <输出路径>       指定输出的 Chart.json 文件路径\n");
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

    const size_t read_len = fread(content, 1, length, file);
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
    cJSON_AddNumberToObject(chart, "offset", offset);
    cJSON_AddNumberToObject(chart, "musicLength", -1.0);
    cJSON_AddBoolToObject(chart, "loopPlayBack", 1);
    cJSON_AddItemToObject(chart, "bpmList", bpm_list);

    add_boxes_to_chart(chart);

    printf(GREEN "==> Json数据初始化完成.\n" RESET);

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

    const char *input_path = "cylheim.json";
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
            fprintf(stderr, RED "==> 未知选项: %s\n" RESET, argv[i]);
            return EXIT_FAILURE;
        }
    }

    char *input_content = read_file(input_path);
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
    const double offset = 0;
    cJSON *bpm_list = create_bpm_list(json);
    printf(GREEN "==> Offset: %f\n" RESET, offset);

    create_chart_json(offset, bpm_list, output_path);

    // 清理内存
    cJSON_Delete(json);
    return 0;
}
