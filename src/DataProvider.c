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

void DataProvider_task(void *pvParameters)
{
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();
    uint16_t sampleCounter  = 0; // Debug

    for (;;)
    {
        // Temperatur aus Sensor lesen [°C]
        float temperature = tmp112_get_value();

        // 32-Bit Payload
        uint32_t payload = (uint32_t) temperature;

        // in PacketEncoder schieben
        if (!PacketEncoder_receiveData(payload))
        {
            ESP_LOGW(TAG,
                     "PacketEncoder queue full, dropping sample %u",
                     (unsigned int)sampleCounter);
        }

        sampleCounter++;

        // Debug
        //ESP_LOGI(TAG, "Temp=%.4f°C -> payload=0x%04X", temperature, payload);

        // auf nächste Periode warten
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DATAPROVIDER_PERIOD_MS));
    }
}

void DataProvider_init(void)
{
    xTaskCreate(DataProvider_task,  //Subroutine
        "DataProvider",             //Name
        2*2048,                     //Stacksize
        NULL,                       //Parameters
        5,                          //Priority
        NULL);                      //Taskhandle
}