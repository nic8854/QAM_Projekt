#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>



extern bool QAM_receive;

void InitQamDemodulator();
void QamDemodulator();

uint64_t QAM_get_Data();



void moving_median(int16_t *input,
                   int16_t *output,
                   size_t length,
                   size_t window);

float average_filter(float *buffer, int length);


int16_t array_max(int16_t *arr, int length);
uint32_t array_zero_pos(int32_t *arr, int length);

void sync_Ampl(int16_t Ampl_meas);


bool PacketDecoder_receivePacket(uint64_t packet);




