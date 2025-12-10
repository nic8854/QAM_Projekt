#include "PacketEncoder.h"
#include "PacketDecoder.h"
#include "QamModulator.h"

#include "math.h"
#include "eduboard2.h"

#define TAG "PacketEncoder"

#define PACKETENCODER_INPUT_QUEUE_LEN 16

QueueHandle_t s_packetEncoderInputQueue = NULL;

// Checksumme über Header + Payload
uint8_t PacketEncoder_calcChecksum( uint8_t syncByte,
                                    uint8_t cmdByte,
                                    uint8_t paramByte,
                                    uint32_t payload)
{
    uint8_t sum = 0;

    sum += syncByte;
    sum += cmdByte;
    sum += paramByte;
    sum += (uint8_t)((payload >> 24) & 0xFF);
    sum += (uint8_t)((payload >> 16) & 0xFF);
    sum += (uint8_t)((payload >> 8)  & 0xFF);
    sum += (uint8_t)( payload        & 0xFF);

    return sum;
}

// Frameaufbau:
// byte1: SYNC
// byte2: CMD
// byte3: PARAM
// byte4..7: DATA (uint32_t payload, MSB zuerst)
// byte8: CHECKSUM

uint64_t PacketEncoder_buildFrame(uint32_t payload)
{
    uint8_t syncByte  = 0xFF;   // Sync
    uint8_t cmdByte   = 0x10;   // Temperatur
    uint8_t paramByte = 0x00;
    uint8_t checksumByte = PacketEncoder_calcChecksum(syncByte, cmdByte, paramByte, payload);

    uint8_t b1 = syncByte;
    uint8_t b2 = cmdByte;
    uint8_t b3 = paramByte;
    uint8_t b4 = (uint8_t)((payload >> 24) & 0xFF);
    uint8_t b5 = (uint8_t)((payload >> 16) & 0xFF);
    uint8_t b6 = (uint8_t)((payload >> 8)  & 0xFF);
    uint8_t b7 = (uint8_t)( payload        & 0xFF);
    uint8_t b8 = checksumByte;

    uint64_t frame = 0;
    frame |= ((uint64_t)b1 << 56);
    frame |= ((uint64_t)b2 << 48);
    frame |= ((uint64_t)b3 << 40);
    frame |= ((uint64_t)b4 << 32);
    frame |= ((uint64_t)b5 << 24);
    frame |= ((uint64_t)b6 << 16);
    frame |= ((uint64_t)b7 <<  8);
    frame |= ((uint64_t)b8      );

    return frame;
}

bool PacketEncoder_receiveData(uint32_t data)
{
    if (s_packetEncoderInputQueue == NULL) return false;

    BaseType_t result = xQueueSend(s_packetEncoderInputQueue, &data, 0);
    return (result);
}


void PacketEncoder_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t payload = 0;

    for (;;)
    {
        // warten auf Payload vom DataProvider
        if (xQueueReceive(s_packetEncoderInputQueue, &payload, portMAX_DELAY))
        {
            uint64_t frame = PacketEncoder_buildFrame(payload);
            // Debug
            //ESP_LOGW(TAG, "Transmitted Frame: 0x%016llX", (unsigned long long)frame);

            if(!PacketDecoder_receivePacket(frame))
                ESP_LOGW(TAG, "PacketDecoder queue full, dropping frame");

            //if (!QamModulator_receivePacket(frame)) 
            //    ESP_LOGW(TAG, "QamModulator queue full, dropping frame");
        }
    }
}

void PacketEncoder_init(void)
{
    s_packetEncoderInputQueue = xQueueCreate(PACKETENCODER_INPUT_QUEUE_LEN, sizeof(uint32_t));

    xTaskCreate(
        PacketEncoder_task, // Taskfunktion
        "PacketEncoder",    // Name
        2 * 2048,           // Stack
        NULL,               // Parameter
        6,                  // Priorität
        NULL                // Handle
    );
}
