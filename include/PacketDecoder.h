#pragma once
#include "eduboard2.h"
#include <stdbool.h>
#include <stdint.h>

#if defined(QAM_RX_MODE) || defined(QAM_TRX_MODE)

void PacketDecoder_init();

bool PacketDecoder_receivePacket(uint64_t packet);

#endif