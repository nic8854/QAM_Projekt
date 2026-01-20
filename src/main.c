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
//    This program uses QAM (Quadrant Amplitude Modulation) for communicating wirelessly
//    between a sender and a receiver device. The code can be use for both devices and can be
//    configured in the File:
//    QAM_PROJECT > eduboard2 > eduboard2_config.h
//    There the program can be set to either sender, receiver or transceiver. When in mode
//    transceiver, there are a multiple of route options to choose (please see documentation).
//
//    At this point the sender-program sends data as the temperature value or an example text
//    encoded in a custom protocol via QAM to an RF-Module. The receiver-program receives the
//    data, decodes and displays it on the LCD-Screen.
//    
/********************************************************************************************* */

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
#include "DataProvider.h"
#include "PacketDecoder.h"
#include "PacketEncoder.h"
#include "QamDemodulator.h"
#include "QamModulator.h"
#include "GuiDriver.h"

// ---------------------------------------------------------------------------- //

// ------------------------------ INIT / START -------------------------------- //
void app_init(void)
{
#if defined(QAM_TX_MODE)

    DataProvider_init();
    PacketEncoder_init();
    QamModulator_init();

#elif defined(QAM_RX_MODE)

    QamDemodulator_init();
    PacketDecoder_init();
    GuiDriver_init();

#elif defined(QAM_TRX_MODE)

  #if defined(TRX_ROUTE_PACKET)

    DataProvider_init();
    PacketEncoder_init();
    PacketDecoder_init();
    GuiDriver_init();

  #elif defined(TRX_ROUTE_MODEM)

    DataProvider_init();
    PacketEncoder_init();
    QamModulator_init();
    QamDemodulator_init();
    PacketDecoder_init();

  #elif defined(TRX_ROUTE_FULL)

    DataProvider_init();
    PacketEncoder_init();
    QamModulator_init();
    QamDemodulator_init();
    PacketDecoder_init();

  #endif

#endif
}
// ---------------------------------------------------------------------------- //

#define TAG "QAM"

#define UPDATETIME_MS 100


void app_main()
{
    //Initialize Eduboard2 BSP
    eduboard2_init();
    app_init();

    // Log Zeug deaktivieren
    //esp_log_level_set("*", ESP_LOG_INFO);


    // Create templateTask
    // xTaskCreate(QAM_receive_Data,   //Subroutine
    //             "QAM_receive", //Name
    //             2*2048,         //Stacksize
    //             NULL,           //Parameters
    //             5,             //Priority
    //             NULL);          //Taskhandle

    for(;;) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        //ESP_LOGI(TAG, "Hello Eduboard");
       // Qam_Burst(0x0F0F0F0F0F0F0F0F);
        //ESP_LOGI(TAG, "DATA SENT");
    }
}