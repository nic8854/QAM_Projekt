#pragma once
#include <stdbool.h>
#include <stdint.h>

extern bool QAM_receive;

void InitQamDemodulator();
void QamDemodulator();

uint64_t QAM_get_Data();