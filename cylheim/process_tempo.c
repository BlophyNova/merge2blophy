#include "process_tempo.h"

void calculate_real_time_and_bpm(const int timeBase, const double tempo, const int tick, double *timing, double *bpm) {
    // 计算每个 Tick 的时间长度 (以微秒为单位)
    const double tick_duration_us = (tempo / timeBase) * tick;
    // 转换为毫秒
    const double tick_duration_ms = tick_duration_us / 1000.0;
    *timing = tick_duration_ms;
    *bpm = 60000000 / tempo;
}

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