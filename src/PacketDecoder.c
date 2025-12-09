#include "PacketDecoder.h"

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "PacketDecoder";

static QueueHandle_t packetQueue = NULL;
static TaskHandle_t PacketDecoder_taskHandle = NULL;

static void PacketDecoder_task(void *pvParameters) {
    uint64_t packet;
    
    while (1) {
        if (xQueueReceive(packetQueue, &packet, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received packet: 0x%llX", packet);
        }
    }
}


void PacketDecoder_init() {
    packetQueue = xQueueCreate(100, sizeof(uint64_t));
    
    xTaskCreate(
        PacketDecoder_task,
        "PacketDecoder",
        4096,
        NULL,
        5,
        &PacketDecoder_taskHandle
    );
}

bool PacketDecoder_receivePacket(uint64_t packet) {
    if (packetQueue == NULL) {
        return false;
    }
    
    bool result = xQueueSend(packetQueue, &packet, 0);
    return result;
}