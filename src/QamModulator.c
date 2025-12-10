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


#define Ampl_factor 0.0885f         // entspricht 1.5Vpp

// Samplerate 100 uS
#define QAM_TABLE_SIZE  128       // 127 Samples pro Sinusperiode => 78.74 Hz

#define Anz_Bits        64       // Anzahl Bits
#define Offset          104       // 104 => 1.65V

static int8_t sinetable[QAM_TABLE_SIZE];
static int8_t cosinetable[QAM_TABLE_SIZE];

static uint8_t Stream_Data[QAM_TABLE_SIZE];


void dacCallbackFunction() {
    
}


void InitQamModulator(){

    void eduboard_init_dac();

    // Sinus und Cosiunustabelle erzeugen
    for (int i = 0; i < QAM_TABLE_SIZE; i++) {
        float val = sinf(2.0f * M_PI * i / QAM_TABLE_SIZE); // -1..1
        sinetable[i] = (int8_t)(val * 127);                 // -127..127

        val = cosf(2.0f * M_PI * i / QAM_TABLE_SIZE);       // -1..1
        cosinetable[i] = (int8_t)(val * 127);               // -127..127
    }

    // 2️⃣ DAC konfigurieren
    dac_set_config(DAC_A, DAC_GAIN_2, true);
    dac_set_config(DAC_B, DAC_GAIN_2, true);
    dac_update();

    dac_set_stream_callback(&dacCallbackFunction); 

    // Starte Stream
    Qam_DAC_Stream (1,1);
}


void Qam_DAC_Stream(int8_t I, int8_t Q)
{
        for (int i = 0; i < QAM_TABLE_SIZE; i++) {
            int val = (int)(Ampl_factor * (sinetable[i] * I + cosinetable[i] * Q));
            Stream_Data[i] = Offset + val;  // Offset für DAC
        }

        for (int i = 0; i < 2; i++)
        {
            dac_load_stream_data(Stream_Data, Stream_Data);
        }  
}

bool Qam_Burst(uint64_t Data){

    // Sartsequenz => Muster für 0
    //Qam_DAC_Stream (-3,-3);
    
    for (int w = 0; w < (Anz_Bits/4); w++)
    {
        int8_t map[4] = { -3, -1, +3, +1 };

        uint8_t shift =  (Anz_Bits-4) - (4 * w);    // erst vorderes Bitmuster
        uint8_t symbol = (Data >> shift) & 0x0F;    // 4 Bits pro Symbol
        uint8_t i_bits = (symbol >> 2) & 0x03;      // obere 2 Bits
        uint8_t q_bits = symbol & 0x03;             // untere 2 Bits
            
        int8_t I = map[i_bits];
        int8_t Q = map[q_bits];
        
        Qam_DAC_Stream (Q, I);
    }

    // Stoppsequenz => Muster für F
    Qam_DAC_Stream (1,1);

    return true;
}