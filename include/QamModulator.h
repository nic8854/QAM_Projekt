#pragma once

void InitQamModulator();

void Qam_DAC_Stream(int8_t I, int8_t Q);

bool Qam_Burst(uint64_t Data);

bool PacketDecoder_receivePacket(uint64_t packet);