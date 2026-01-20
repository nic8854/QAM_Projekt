#include "eduboard2.h"
#include "PacketEncoder.h"
#include "PacketDecoder.h"
#include "QamModulator.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#define TAG "PacketEncoder"

// Queue-Längen
#define PACKETENCODER_TEMP_QUEUE_LEN  16
#define PACKETENCODER_TEXT_QUEUE_LEN  64   // Text-Bytes, inkl. '\0'

// Commands
#define CMD_Temp  0x10
#define CMD_Text  0x20

// Queues
QueueHandle_t s_packetEncoderTempQueue = NULL;
QueueHandle_t s_packetEncoderTextQueue = NULL;

// ------------------------- Checksum & Frame ------------------------- //

// Checksumme über Header + Payload (8-bit Summe)
uint8_t PacketEncoder_calcChecksum(uint8_t syncByte,
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
uint64_t PacketEncoder_buildFrame(uint32_t payload, uint8_t cmd, uint8_t param)
{
    uint8_t syncByte  = 0xFF;
    uint8_t cmdByte   = cmd;
    uint8_t paramByte = param;

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
    frame |= ((uint64_t)b8);

    return frame;
}

void PacketEncoder_forwardFrame(uint64_t frame)
{
    #if defined(QAM_TRX_MODE) && defined(TRX_ROUTE_PACKET)
        if (!PacketDecoder_receivePacket(frame))
            ESP_LOGW(TAG, "PacketDecoder queue full, dropping frame");
    
    #else
        if (!QamModulator_receivePacket(frame))
            ESP_LOGW(TAG, "QamModulator queue full, dropping frame");
    #endif
}

bool PacketEncoder_receiveTemperature(uint32_t data)
{
    if (s_packetEncoderTempQueue == NULL) return false;
    return xQueueSend(s_packetEncoderTempQueue, &data, 0) == pdTRUE;
}

bool PacketEncoder_receiveTextByte(uint8_t byte)
{
    if (s_packetEncoderTextQueue == NULL) return false;
    return xQueueSend(s_packetEncoderTextQueue, &byte, 0) == pdTRUE;
}

// ------------------------- Text packing ------------------------- //
// 4 Bytes sammeln und als uint32_t schicken (MSB zuerst).

uint32_t s_textWord  = 0;
uint8_t  s_textCount = 0;   // 0..4
uint8_t  s_textParam = 0;   // 0..255

void Text_sendPackedWord(bool messageEnded)
{
    uint64_t frame = PacketEncoder_buildFrame(s_textWord, CMD_Text, s_textParam);
    PacketEncoder_forwardFrame(frame);

    s_textWord  = 0;
    s_textCount = 0;

    if (messageEnded)
        s_textParam = 0;
    else
        s_textParam++;
}


void PacketEncoder_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t payload = 0;
    uint64_t frame = 0;
    uint8_t ch = 0;
    uint8_t shift = 0;
    bool ended = false;

    for (;;)
    {
        if (xQueueReceive(s_packetEncoderTempQueue, &payload, 0) == pdTRUE)
        {
            frame = PacketEncoder_buildFrame(payload, CMD_Temp, 0);
            PacketEncoder_forwardFrame(frame);
        }
        if (xQueueReceive(s_packetEncoderTextQueue, &ch, 0) == pdTRUE)
        {
            // Packen: 1. Byte -> [31:24], 2. -> [23:16], 3. -> [15:8], 4. -> [7:0]
            shift = (uint8_t)(24 - (8U * s_textCount));
            s_textWord |= ((uint32_t)ch) << shift;
            s_textCount++;

            ended = (ch == '\0');

            // Frame senden bei 4 Bytes ODER sobald Terminator drin ist
            if (s_textCount >= 4 || ended) Text_sendPackedWord(ended);
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
}


void PacketEncoder_init(void)
{
    s_packetEncoderTempQueue = xQueueCreate(PACKETENCODER_TEMP_QUEUE_LEN, sizeof(uint32_t));
    s_packetEncoderTextQueue = xQueueCreate(PACKETENCODER_TEXT_QUEUE_LEN, sizeof(uint8_t));

    xTaskCreate(
        PacketEncoder_task,
        "PacketEncoder",
        2 * 2048,
        NULL,
        6,
        NULL
    );
}
