#include "DataProvider.h"
#include "PacketEncoder.h"

#include "math.h"
#include "eduboard2.h"

#define TAG "DataProvider"

// Wie oft Daten gesendet werden [ms]
#define DATAPROVIDER_PERIOD_MS       1000
#define DATAPROVIDER_TEXT_PERIOD_MS  30000

char s_exampleText[] = "Hallo von QAM, das ist ein Test.\0";

void DataProvider_sendExampleText(void)
{
    for (size_t i = 0;; i++)
    {
        const uint8_t text = (uint8_t)s_exampleText[i];

        if (!PacketEncoder_receiveTextByte(text))
        {
            ESP_LOGW(TAG, "Text-Queue voll, Text abgebrochen (Byte 0x%02X)", text);
            return;
        }

        if (text == '\0')
            return;
    }
}

// -----------------------------------------------------------------------------
// DataProvider task
// -----------------------------------------------------------------------------
void DataProvider_task(void *pvParameters)
{
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();
    TickType_t lastTextTime = lastWakeTime;

    for (;;)
    {
        // --- Temperatur senden ---
        float temperature = tmp112_get_value();

        uint32_t payload;
        memcpy(&payload, &temperature, sizeof(float));

        if (!PacketEncoder_receiveTemperature(payload))
            ESP_LOGW(TAG, "PacketEncoder payload-queue full, dropping sample");


        // --- Beispieltext senden ---
        TickType_t now = xTaskGetTickCount();
        if ((now - lastTextTime) >= pdMS_TO_TICKS(DATAPROVIDER_TEXT_PERIOD_MS))
        {
            DataProvider_sendExampleText();
            lastTextTime = now;
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DATAPROVIDER_PERIOD_MS));
    }
}

void DataProvider_init(void)
{
    xTaskCreate(DataProvider_task,
                "DataProvider",
                2*2048,
                NULL,
                5,
                NULL);
}
