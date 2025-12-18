/********************************************************************************************* */
//    Projekt:  QAM
//    Author:   Michael Mächler
//              Ignacio Neuhaus
//              Remo Bachmann
//              Sandro Alder
//    Dozent:   Martin Burger
//    Juventus Technikerschule - Embedded Systems
//    Version: 1.0.0
//    
//    *insert projekt description here*
//    
//    
/********************************************************************************************* */

// ------------------------------ PROGRAM CONFIG ------------------------------ //

// Mode select

//#define QAM_TX_MODE             // Transmitter
//#define QAM_RX_MODE             // Receiver
#define QAM_TRX_MODE            // Transceiver

// Route select
#if defined(QAM_TRX_MODE)

  //#define TRX_ROUTE_PACKET      // PacketEncoder -> PacketDecoder (without Modem)
  //#define TRX_ROUTE_MODEM       // Modulator -> Demodulator (digital loopback)
  #define TRX_ROUTE_FULL        // Modulator -> DAC -> ADC -> Demodulator (full)

#endif
// ---------------------------------------------------------------------------- //

// --------------------------------- INCLUDES --------------------------------- //

#include "eduboard2.h"
#include "esp_log.h"
#include "memon.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "math.h"
#include <stdio.h>

// Header files
#include "AdcDataRelay.h"
#include "DacDataRelay.h"
#include "DataProvider.h"
#include "GuiDriver.h"
#include "PacketDecoder.h"
#include "PacketEncoder.h"
#include "QamDemodulator.h"
#include "QamModulator.h"

// ---------------------------------------------------------------------------- //

// ------------------------------ INIT / START -------------------------------- //
void app_init(void)
{
#if defined(QAM_TX_MODE)

    DataProvider_init();
    PacketEncoder_init();
    InitQamModulator();
    InitDacDataRelay();

#elif defined(QAM_RX_MODE)

    InitAdcDataRelay();
    InitQamDemodulator();
    PacketDecoder_init();
    InitGuiDriver();

#elif defined(QAM_TRX_MODE)

  #if defined(TRX_ROUTE_PACKET)

    DataProvider_init();
    PacketEncoder_init();
    PacketDecoder_init();

  #elif defined(TRX_ROUTE_MODEM)

    DataProvider_init();
    PacketEncoder_init();
    InitQamModulator();
    InitQamDemodulator();
    PacketDecoder_init();

  #elif defined(TRX_ROUTE_FULL)

    DataProvider_init();
    PacketEncoder_init();
    InitQamModulator();
    InitDacDataRelay();
    InitAdcDataRelay();
    InitQamDemodulator();
    PacketDecoder_init();

  #endif

#endif
}
// ---------------------------------------------------------------------------- //

#define TAG "QAM"

#define UPDATETIME_MS 100

void templateTask(void* param) {
    //Init stuff here
    vTaskDelay(10);
    for(;;) {
        vTaskDelay(10);
        // task main loop
        if(button_get_state(SW0, true) == SHORT_PRESSED) {
            Qam_Burst(0x0F0F0F0F0F0F0F0F);
        }
        //led_toggle(LED7);
        // delay
        vTaskDelay(UPDATETIME_MS/portTICK_PERIOD_MS);
    }
}


void QAM_receive_Data(void* param) {
    //Init stuff here
    ESP_LOGI(TAG, "Task started!");
    for(;;) {
        vTaskDelay(100);

        //uint16_t adcBuffer[256];
        
        if(QAM_receive)        // ein Datenblock wurde komplett empfangen
        {
            //led_toggle(LED1);
            uint64_t d = QAM_get_Data();
            ESP_LOGI(TAG, "QAM Data = 0x%016" PRIx64, d);
        }
    }
}




void app_main()
{
    //Initialize Eduboard2 BSP
    eduboard2_init();
    app_init();

    // Log Zeug deaktivieren
    esp_log_level_set("*", ESP_LOG_INFO);

    
    xTaskCreate(templateTask,   //Subroutine
                "testTask",     //Name
                2*2048,         //Stacksize
                NULL,           //Parameters
                10,             //Priority
                NULL);          //Taskhandle


    // Create templateTask
    // xTaskCreate(QAM_receive_Data,   //Subroutine
    //             "QAM_receive", //Name
    //             2*2048,         //Stacksize
    //             NULL,           //Parameters
    //             5,             //Priority
    //             NULL);          //Taskhandle

    for(;;) {
        vTaskDelay(2000/portTICK_PERIOD_MS);
        //ESP_LOGI(TAG, "Hello Eduboard");
    }
}