#include "PacketDecoder.h"

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t packetQueue = NULL;


void PacketDecoder_init() {
    packetQueue = xQueueCreate(100, sizeof(uint64_t));
}

bool PacketDecoder_receivePacket(uint64_t packet) {
    if (packetQueue == NULL) {
        return false;
    }
    
    bool result = xQueueSend(packetQueue, &packet, 0);
    return result;
}