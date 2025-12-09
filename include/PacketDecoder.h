#pragma once
#include <stdbool.h>

void PacketDecoder_init();

bool PacketDecoder_receivePacket(uint64_t packet);