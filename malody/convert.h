#pragma once
#include "../includes/cross_platform.h"
#include "tools.h"
void print_help(const char *program_name);
char *read_file(const char *filename);
int get_absolute_path(const char *path, char *abs_path);
int create_directory_if_not_exists(const char *path);
int unzip_mcz(const char *mcz_path, const char *output_dir);
char *choose_mc_file(char **mc_files, int mc_file_count);
char *read_stdin_custom();
double extract_last_offset(const cJSON *notes);
cJSON *create_bpm_list(const cJSON *bpm);
cJSON *createBeatObject(double startBPM, double endBPM);
void add_boxes_to_chart(cJSON *chart);
void create_chart_json(double offset, cJSON *bpm_list, const char *output_path);
