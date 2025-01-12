#pragma once
#include "../includes/cross_platform.h"
#include "create_bpmlist.h"

void print_help(const char *program_name);

char *read_file(const char *filename);

cJSON *createBeatObject(double startBPM, double endBPM);

void add_boxes_to_chart(cJSON *chart);

void create_chart_json(double offset, cJSON *bpm_list, const char *output_path);
