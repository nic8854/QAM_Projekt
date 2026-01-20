#include "PacketDecoder.h"
#include "GuiDriver.h"

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

#if defined(QAM_RX_MODE) || defined(QAM_TRX_MODE)

static const char *TAG = "PacketDecoder";

static QueueHandle_t packetQueue = NULL;
static TaskHandle_t PacketDecoder_taskHandle = NULL;
static uint32_t droppedPackets = 0;

uint8_t PacketDecoder_getCmd__(uint64_t packet);
uint8_t PacketDecoder_getParam__(uint64_t packet);
uint32_t PacketDecoder_getData__(uint64_t packet);
uint8_t PacketDecoder_getSyncByte__(uint64_t packet);
uint8_t PacketDecoder_getChecksumByte__(uint64_t packet);
uint8_t PacketDecoder_calcChecksum__(uint64_t packet);
bool PacketDecoder_verifyChecksum__(uint64_t packet);

static void PacketDecoder_task(void *pvParameters) {
    uint64_t packet;

    while (1) {
        if (xQueueReceive(packetQueue, &packet, pdMS_TO_TICKS(50)) == pdTRUE) {
            ESP_LOGI(TAG, "Received packet: 0x%llX", packet);

            // Verify checksum
            if (!PacketDecoder_verifyChecksum__(packet)) {
                ESP_LOGW(TAG, "Checksum verification failed! Expected: 0x%02X, Got: 0x%02X", 
                         PacketDecoder_calcChecksum__(packet), PacketDecoder_getChecksumByte__(packet));
                droppedPackets++;
                ESP_LOGE(TAG, "Dropping packet. Total dropped: %d", droppedPackets);
                #if defined(QAM_RX_MODE) || (defined(QAM_TRX_MODE) && defined(TRX_ROUTE_PACKET))
                    GuiDriver_receiveDroppedPackets(droppedPackets);
                #endif
                continue;
            }

            switch(PacketDecoder_getCmd__(packet)) {
                case 0x10: {
                    uint32_t data = PacketDecoder_getData__(packet);
                    float temperature;
                    memcpy(&temperature, &data, sizeof(float));
                    ESP_LOGI(TAG, "Temperature command received: %.2fC", temperature);
                    #if defined(QAM_RX_MODE) || (defined(QAM_TRX_MODE) && defined(TRX_ROUTE_PACKET))
                        GuiDriver_receiveTemperature(temperature);
                    #endif
                    break;
                }
                case 0x20: {
                    uint32_t data = PacketDecoder_getData__(packet);
                    char text[4];
                    text[0] = (char)((data >> 24) & 0xFF);
                    text[1] = (char)((data >> 16) & 0xFF);
                    text[2] = (char)((data >> 8) & 0xFF);
                    text[3] = (char)(data & 0xFF);
                    ESP_LOGI(TAG, "Text command received: %c%c%c%c", text[0], text[1], text[2], text[3]);
                    #if defined(QAM_RX_MODE) || (defined(QAM_TRX_MODE) && defined(TRX_ROUTE_PACKET))
                        GuiDriver_receiveText(text);
                    #endif
                    break;
                }
                default:
                    ESP_LOGW(TAG, "Unknown command: 0x%02X", PacketDecoder_getCmd__(packet));
                    droppedPackets++;
                    ESP_LOGE(TAG, "Dropping packet. Total dropped: %d", droppedPackets);
                    #if defined(QAM_RX_MODE) || (defined(QAM_TRX_MODE) && defined(TRX_ROUTE_PACKET))
                        GuiDriver_receiveDroppedPackets(droppedPackets);
                    #endif
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
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
    return (uint32_t)((packet & 0x000000FFFFFFFF00) >> 8);
}

uint8_t PacketDecoder_getSyncByte__(uint64_t packet) {
    return (uint8_t)((packet & 0xFF00000000000000) >> 56);
}

uint8_t PacketDecoder_getChecksumByte__(uint64_t packet) {
    return (uint8_t)(packet & 0x00000000000000FF);
}

uint8_t PacketDecoder_calcChecksum__(uint64_t packet) {
    uint8_t sum = 0;
    uint8_t syncByte = PacketDecoder_getSyncByte__(packet);
    uint8_t cmdByte = PacketDecoder_getCmd__(packet);
    uint8_t paramByte = PacketDecoder_getParam__(packet);
    uint32_t payload = PacketDecoder_getData__(packet);

    sum += syncByte;
    sum += cmdByte;
    sum += paramByte;
    sum += (uint8_t)((payload >> 24) & 0xFF);
    sum += (uint8_t)((payload >> 16) & 0xFF);
    sum += (uint8_t)((payload >> 8)  & 0xFF);
    sum += (uint8_t)( payload        & 0xFF);

    return sum;
}

bool PacketDecoder_verifyChecksum__(uint64_t packet) {
    uint8_t calculatedChecksum = PacketDecoder_calcChecksum__(packet);
    uint8_t receivedChecksum = PacketDecoder_getChecksumByte__(packet);
    return (calculatedChecksum == receivedChecksum);
}

#endif