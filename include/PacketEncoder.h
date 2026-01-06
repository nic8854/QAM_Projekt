#pragma once

#include <stdint.h>
#include <stdbool.h>

void PacketEncoder_init(void);

bool PacketEncoder_receiveTemperature(uint32_t data);

bool PacketEncoder_receiveTextByte(uint8_t byte);