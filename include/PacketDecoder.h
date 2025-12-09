#pragma once
#include <stdbool.h>
#include <stdint.h>

void PacketDecoder_init();

bool PacketDecoder_receivePacket(uint64_t packet);