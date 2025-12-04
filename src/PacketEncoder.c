#include "PacketEncoder.h"

#include "math.h"
#include "eduboard2.h"

#define TAG "PacketEncoder"

// Queue für Daten von DataProvider
#define PACKETENCODER_INPUT_QUEUE_LEN 16

QueueHandle_t s_packetEncoderInputQueue = NULL;

uint16_t PacketEncoder_calcChecksum16(uint32_t payload)
{
    uint16_t checksum = 0;
    // rechnen
    return (checksum);
}

// Frame: [63:55] syncByte, [47:40] commandByte, [39:8] Payload, [7:0] checksumByte
uint64_t PacketEncoder_buildFrame(uint32_t payload)
{
    const uint8_t syncByte = 0xFF;
    const uint8_t commandByte = 0x0A;
    const uint8_t paramByte = 0x00;
    const uint8_t payload = 0x00;
    const uint16_t checksumByte = PacketEncoder_calcChecksum16(payload);

    uint64_t frame = 0;
    frame |= ((uint64_t)syncByte << 48);
    frame |= ((uint64_t)commandByte << 40);
    frame |= ((uint64_t)payload << 8);
    frame |= ((uint64_t)checksumByte);

    return frame;
}

// -----------------------------------------------------------------------------
// PacketEncoder API
// -----------------------------------------------------------------------------

bool PacketEncoder_receiveData(uint32_t data)
{
    if (s_packetEncoderInputQueue == NULL)
    {
        // noch nicht initialisiert
        return false;
    }

    BaseType_t ok = xQueueSend(s_packetEncoderInputQueue, &data, 0); // keine Wartezeit
    return (ok == pdPASS);
}

// -----------------------------------------------------------------------------
// PacketEncoder Task
// -----------------------------------------------------------------------------

void PacketEncoderTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t rxWord = 0;

    for (;;)
    {
        // blockierend auf neue Daten warten
        if (xQueueReceive(s_packetEncoderInputQueue,
                          &rxWord,
                          portMAX_DELAY) == pdPASS)
        {
            uint64_t frame = PacketEncoder_buildFrame(rxWord);

            // Frame an QAM-Modulator weitergeben
            //QAMModulator_receiveData(frame)

            ESP_LOGI(TAG,
                     "Encoded frame: 0x%016llX (from 0x%08X)",
                     (unsigned long long)frame,
                     (unsigned int)rxWord);
        }
    }
}

void InitPacketEncoder(void)
{
    s_packetEncoderInputQueue = xQueueCreate(
        PACKETENCODER_INPUT_QUEUE_LEN,
        sizeof(uint32_t)
    );

    if (s_packetEncoderInputQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create PacketEncoder input queue");
        return;
    }

    xTaskCreate(PacketEncoderTask,  //Subroutine
        "PacketEncoder",            //Name
        2*2048,                     //Stacksize
        NULL,                       //Parameters
        6,                          //Priority
        NULL);                      //Taskhandle
}