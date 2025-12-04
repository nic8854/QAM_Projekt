#include "PacketEncoder.h"

#include "math.h"
#include "eduboard2.h"

#define TAG "PacketEncoder"

// Eingangsqueue DataProvider
#define PACKETENCODER_INPUT_QUEUE_LEN 16

QueueHandle_t s_packetEncoderInputQueue = NULL;

// -----------------------------------------------------------------------------
// Checksumme: einfacher 8-Bit-Summen-Checksum über die ersten 7 Bytes
// -----------------------------------------------------------------------------

uint8_t PacketEncoder_calcChecksum8(const uint8_t *bytes, size_t len)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; ++i)
    {
        sum += bytes[i];
    }
    return (uint8_t)(sum & 0xFFu);
}

// -----------------------------------------------------------------------------
// Frameaufbau gemäss Protokoll (8 Bytes → 64 Bit)
// -----------------------------------------------------------------------------

uint64_t PacketEncoder_buildFrame(uint32_t payload)
{
    uint8_t syncByte   = 0xFF;
    uint8_t commandByte = 0x0A; // "Temperatur" -> später mit enum erweitern
    uint8_t paramByte   = 0x00;

    // Payload als 4 Datenbytes (MSB zuerst)
    uint8_t data1 = (uint8_t)((payload >> 24) & 0xFFu); // MSB
    uint8_t data2 = (uint8_t)((payload >> 16) & 0xFFu);
    uint8_t data3 = (uint8_t)((payload >> 8)  & 0xFFu);
    uint8_t data4 = (uint8_t)(payload & 0xFFu);         // LSB

    uint8_t bytes[7] = {
        syncByte,
        commandByte,
        paramByte,
        data1,
        data2,
        data3,
        data4
    };

    uint8_t checksumByte = PacketEncoder_calcChecksum8(bytes, 7);

    // Bytes in 64-Bit-Frame packen (Byte 1 = MSB)
    uint64_t frame = 0;
    frame |= ((uint64_t)syncByte    << 56);
    frame |= ((uint64_t)commandByte << 48);
    frame |= ((uint64_t)paramByte   << 40);
    frame |= ((uint64_t)data1       << 32);
    frame |= ((uint64_t)data2       << 24);
    frame |= ((uint64_t)data3       << 16);
    frame |= ((uint64_t)data4       << 8);
    frame |= ((uint64_t)checksumByte);

    return frame;
}

// -----------------------------------------------------------------------------
// Public API: Queue-Zugriff
// -----------------------------------------------------------------------------

bool PacketEncoder_receiveData(uint32_t data)
{
    if (s_packetEncoderInputQueue == NULL)
    {
        // InitPacketEncoder() noch nicht aufgerufen
        return false;
    }

    // stark vereinfachtes Handling: non-blocking, Resultat direkt zurückgeben
    return (xQueueSend(s_packetEncoderInputQueue, &data, 0) == pdPASS);
}

// -----------------------------------------------------------------------------
// Task
// -----------------------------------------------------------------------------

static void PacketEncoderTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t rxWord;

    for (;;)
    {
        // blockierend auf neue Daten warten
        if (xQueueReceive(s_packetEncoderInputQueue,
                          &rxWord,
                          portMAX_DELAY) == pdPASS)
        {
            uint64_t frame = PacketEncoder_buildFrame(rxWord);

            // TODO: an QAMModulator weitergeben
            // QAMModulator_receiveData(frame);

            ESP_LOGI(TAG,
                     "Frame: 0x%016llX (from 0x%08X)",
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

    xTaskCreate(
        PacketEncoderTask,  // Taskfunktion
        "PacketEncoder",    // Name
        2 * 2048,           // Stack
        NULL,               // Parameter
        6,                  // Priorität
        NULL                // Handle
    );
}
