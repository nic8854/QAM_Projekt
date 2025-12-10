#pragma once

#include <stdint.h>
#include <stdbool.h>

void PacketEncoder_init(void);

bool PacketEncoder_receiveData(uint32_t data);