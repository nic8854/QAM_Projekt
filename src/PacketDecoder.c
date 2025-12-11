#include "PacketDecoder.h"

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "PacketDecoder";

static QueueHandle_t packetQueue = NULL;
static TaskHandle_t PacketDecoder_taskHandle = NULL;

uint8_t PacketDecoder_getCmd__(uint64_t packet);
uint8_t PacketDecoder_getParam__(uint64_t packet);
uint32_t PacketDecoder_getData__(uint64_t packet);

static void PacketDecoder_task(void *pvParameters) {
    uint64_t packet;
    
    while (1) {
        if (xQueueReceive(packetQueue, &packet, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received packet: 0x%llX", packet);
            switch(PacketDecoder_getCmd__(packet)) {
                case 0x10: {
                    uint32_t data = PacketDecoder_getData__(packet);
                    float temperature;
                    memcpy(&temperature, &data, sizeof(float));
                    ESP_LOGI(TAG, "Temperature command received: %.2fC", temperature);
                    break;
                }
                default:
                    ESP_LOGW(TAG, "Unknown command: 0x%02X", PacketDecoder_getCmd__(packet));
                    break;
            }
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

uint8_t PacketDecoder_getCmd__(uint64_t packet) {
    return (uint8_t)((packet & 0x00FF000000000000) >> 48);
}

uint8_t PacketDecoder_getParam__(uint64_t packet) {
    return (uint8_t)((packet & 0x0000FF0000000000) >> 40);
}

uint32_t PacketDecoder_getData__(uint64_t packet) {
    return (uint32_t)(packet & 0x00000000FFFFFFFF);
}