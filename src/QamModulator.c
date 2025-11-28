/********************************************************************************************* */
//    QamModulator
//    Author: Sandro Alder
//    Juventus Technikerschule
//    Version: 1.0.0
/********************************************************************************************* */

#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#include "eduboard2.h"
#include "QamModulator.h"


#define Ampl_factor 0.177f

// 1kHz Frequenz
#define QAM_TABLE_SIZE 127       // Anzahl Samples pro Sinusperiode

#define Nr_Waves        127      //2*Anzahl Bits


static int8_t sinetable[QAM_TABLE_SIZE];
static int8_t cosinetable[QAM_TABLE_SIZE];

static uint8_t Stream_Data[QAM_TABLE_SIZE];


void dacCallbackFunction() {
    dac_load_stream_data(Stream_Data, Stream_Data);
}


void InitQamModulator(){

    void eduboard_init_dac();


    // Sinus und Cosiunustabelle erzeugen
    for (int i = 0; i < QAM_TABLE_SIZE; i++) {
        float val = sinf(2.0f * M_PI * i / QAM_TABLE_SIZE); // -1..1
        sinetable[i] = (int8_t)(val * 127);                // -127..127

        val = cosf(2.0f * M_PI * i / QAM_TABLE_SIZE);      // -1..1
        cosinetable[i] = (int8_t)(val * 127);             // -127..127
    }

    // 2️⃣ DAC konfigurieren
    dac_set_config(DAC_A, DAC_GAIN_1, true);
    dac_set_config(DAC_B, DAC_GAIN_1, true);
    dac_update();

    dac_set_stream_callback(&dacCallbackFunction);   
}


bool Qam_Burst(uint64_t Data){

    int8_t map[4] = { -3, -1, +3, +1 };

    for (int w = 0; w < Nr_Waves; w++)
    {
        uint8_t symbol = (Data >> (4 * w)) & 0x0F; // 4 Bits pro Symbol
        uint8_t i_bits = (symbol >> 2) & 0x03; // obere 2 Bits
        uint8_t q_bits = symbol & 0x03;        // untere 2 Bits
            
        int8_t I = map[i_bits];
        int8_t Q = map[q_bits];
        

        for (int i = 0; i < QAM_TABLE_SIZE; i++) {
            int val = (int)(Ampl_factor * (sinetable[i] * I + cosinetable[i] * Q));
            Stream_Data[i] = 127 + val;  // Offset für DAC 0..255
        }

        dac_set_stream_callback(&dacCallbackFunction);
        dac_load_stream_data(Stream_Data, Stream_Data);
    }

    return true;
}