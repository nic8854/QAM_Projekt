#include "DataProvider.h"
#include "PacketEncoder.h"

#include "math.h"
#include "eduboard2.h"

#define TAG "DataProvider"

// Wie oft die Temperatur gelesen wird [ms]
#define DATAPROVIDER_PERIOD_MS  1000

// -----------------------------------------------------------------------------
// DataProvider task
// -----------------------------------------------------------------------------

static void DataProviderTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();
    uint16_t sampleCounter  = 0; // Debug

    for (;;)
    {
        // Temperatur aus Sensor lesen [°C]
        float temperature = tmp112_get_value();

        // 32-Bit Payload
        uint32_t payload = (int16_t)roundf(temperature * 100.0f);

        // in PacketEncoder schieben
        if (!PacketEncoder_receiveData(payload))
        {
            ESP_LOGW(TAG,
                     "PacketEncoder queue full, dropping sample %u",
                     (unsigned int)sampleCounter);
        }

        sampleCounter++;

        // Debug
        ESP_LOGI(TAG,
                 "Temp=%.2f°C -> payload=0x%08X",
                 temperature,
                 (unsigned int)payload);

        // auf nächste Periode warten
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DATAPROVIDER_PERIOD_MS));
    }
}

void InitDataProvider(void)
{
    xTaskCreate(DataProviderTask,   //Subroutine
        "DataProvider",             //Name
        2*2048,                     //Stacksize
        NULL,                       //Parameters
        5,                          //Priority
        NULL);                      //Taskhandle
}