#pragma once
#include "create_bpmlist.h"

// 用于获取最简分数的最大公约数
int gcd(int a, int b) {
    while (b != 0) {
        const int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// 将 double 转换为近似的分数形式
void double_to_fraction(const double value, int *main, int *molecule, int *denominator) {
    // 获取整数部分
    *main = (int) value;

    // 获取小数部分
    const double fractional_part = value - *main;

    // 假设我们将小数部分转换为分母为 1000 的分数
    const int precision = 1000;
    const int num = (int) (fractional_part * precision); // 小数部分乘以1000
    *denominator = precision;

    // 最简分数
    const int common_divisor = gcd(num, *denominator);
    *molecule = num / common_divisor;
    *denominator = *denominator / common_divisor;
}

// 生成 bpmList
cJSON *create_bpm_list(const cJSON *bpm) {
    if (!cJSON_IsArray(bpm)) {
        fprintf(stderr, RED "==> time 字段不是数组\n" RESET);
        return cJSON_CreateArray();
    }

    cJSON *bpm_list = cJSON_CreateArray();
    const int count = cJSON_GetArraySize(bpm);

    for (int i = 0; i < count; i++) {
        if (!cJSON_IsArray(bpm)) {
            fprintf(stderr, RED "==> Bpm 数组中的元素格式不正确\n" RESET);
            continue;
        }
        const cJSON *entry = cJSON_GetArrayItem(bpm, i);
        const cJSON *beat = cJSON_GetObjectItem(entry, "Timing");
        const cJSON *bpm_value = cJSON_GetObjectItem(entry, "Bpm");

        int a, b, c;
        if (beat->valuedouble < 0) {
            a = 0;
            b = 0;
            c = 0;
        } else {
            double_to_fraction(beat->valuedouble, &a, &b, &c);
        }

        cJSON *bpm_entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(bpm_entry, "integer", a);
        cJSON_AddNumberToObject(bpm_entry, "molecule", b);
        cJSON_AddNumberToObject(bpm_entry, "denominator", c);
        cJSON_AddNumberToObject(bpm_entry, "currentBPM", bpm_value->valuedouble);
        cJSON_AddNumberToObject(bpm_entry, "ThisStartBPM", bpm_value->valuedouble);

        cJSON_AddItemToArray(bpm_list, bpm_entry);
    }

    printf(GREEN "==> BPM List解析完成.\n");

    return bpm_list;
}
