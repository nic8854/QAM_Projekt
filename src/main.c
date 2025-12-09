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
#include "eduboard2.h"
#include "memon.h"

#include "math.h"

#include "AdcDataRelay.h"
#include "DacDataRelay.h"
#include "DataProvider.h"
#include "GuiDriver.h"
#include "PacketDecoder.h"
#include "PacketEncoder.h"
#include "QamDemodulator.h"
#include "QamModulator.h"

#define TAG "QAM"

#define UPDATETIME_MS 100

void templateTask(void* param) {
    //Init stuff here
    vTaskDelay(10);
    for(;;) {
        vTaskDelay(10);
        // task main loop
        if(button_get_state(SW0, true) == SHORT_PRESSED) {
            Qam_Burst(0xF123456789ABCD0);
        }
        led_toggle(LED7);
        // delay
        vTaskDelay(UPDATETIME_MS/portTICK_PERIOD_MS);
    }
}

void app_main()
{
    //Initialize Eduboard2 BSP
    eduboard2_init();

    //Initialize QAM components
    InitAdcDataRelay();
    InitDacDataRelay();
    InitDataProvider();
    InitGuiDriver();
    PacketDecoder_init();
    InitPacketEncoder();
    InitQamDemodulator();
    InitQamModulator();
    
    //Create templateTask
    xTaskCreate(templateTask,   //Subroutine
                "testTask",     //Name
                2*2048,         //Stacksize
                NULL,           //Parameters
                10,             //Priority
                NULL);          //Taskhandle
    for(;;) {
        vTaskDelay(2000/portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Hello Eduboard");
    }
}