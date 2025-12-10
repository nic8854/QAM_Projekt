#pragma once
#include <stdbool.h>
#include <stdint.h>

extern bool QAM_receive;

void InitQamDemodulator();
void QamDemodulator();

uint64_t QAM_get_Data();






float average_filter(float *buffer, int length);
int32_t array_max(int32_t *arr, int length);
uint32_t array_zero_pos(int32_t *arr, int length);









