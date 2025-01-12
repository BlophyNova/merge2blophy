#include "create_bpmlist.h"

#include "process_tempo.h"

// 生成 bpmList
cJSON *create_bpm_list(const cJSON *entry) {
    cJSON *bpm_list = cJSON_CreateArray();
    const cJSON *tempo_entry = cJSON_GetObjectItem(entry, "tempo_list");
    const int count = cJSON_GetArraySize(tempo_entry);

    for (int i = 0; i < count; i++) {
        if (!cJSON_IsArray(tempo_entry)) {
            fprintf(stderr, RED "==> Bpm 数组中的元素格式不正确\n" RESET);
            continue;
        }

        const cJSON *tempo = cJSON_GetArrayItem(tempo_entry, i);

        double timing, bpm;

        calculate_real_time_and_bpm(cJSON_GetObjectItem(entry, "time_base")->valueint,
                                    cJSON_GetObjectItem(tempo, "tempo")->valuedouble,
                                    cJSON_GetObjectItem(tempo, "start_tick")->valueint, &timing, &bpm);

        int a, b, c;

        double_to_fraction(timing, &a, &b, &c);


        cJSON *bpm_entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(bpm_entry, "integer", a);
        cJSON_AddNumberToObject(bpm_entry, "molecule", b);
        cJSON_AddNumberToObject(bpm_entry, "denominator", c);
        cJSON_AddNumberToObject(bpm_entry, "currentBPM", bpm);
        cJSON_AddNumberToObject(bpm_entry, "ThisStartBPM", bpm);

        cJSON_AddItemToArray(bpm_list, bpm_entry);
    }

    printf(GREEN "==> BPM List解析完成.\n");

    return bpm_list;
}
