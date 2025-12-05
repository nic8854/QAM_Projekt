#include "QamDemodulator.h"

#include "esp_rom_sys.h"
#include "eduboard2.h"
#include "math.h"
#include <stdbool.h>
#include <stdint.h>

#define Offset      127
#define Invert      1
//#define Invert      -1

#define ADC_BUFFER_SIZE 256
#define ADC_resolution 12

#define Start_Offset   10


typedef enum {
    Idle,
    Read
} State;
State mode = Idle;

bool QAM_receive = false;

static int8_t sinetable[ADC_BUFFER_SIZE];
static int8_t cosinetable[ADC_BUFFER_SIZE];

static int16_t sync_Amplitude = 0;
static int16_t sync_Phase = 0;

static uint64_t Data = 0xFFFF;


void InitQamDemodulator(){

    // Sinus und Cosiunustabelle erzeugen
    for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
        float val = sinf(2.0f * M_PI * i / (ADC_BUFFER_SIZE/2)); // -1..1
        sinetable[i] = (int8_t)(val * 127);                // -127..127

        val = cosf(2.0f * M_PI * i / ADC_BUFFER_SIZE/2);      // -1..1
        cosinetable[i] = (int8_t)(val * 127);             // -127..127
    }
}


float average_filter(int16_t *buffer, int length) {
    float sum = 0.0f;

    for (int i = 0; i < length; i++) {
        sum += buffer[i];
    }

    return sum / length;
}

int16_t array_max(int16_t *arr, int length) {
    int16_t max = arr[0];

    for (int i = 1; i < length; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

int16_t array_zero_pos(int16_t *arr, int length) {
    
    uint8_t zero_pos = 0;

    for(int i = 0; i < length-1; i++) {
        // Bedingung für positiven Nulldurchgang: negativ -> positiv
        if(arr[i] < 0 && arr[i+1] > 0) {
            zero_pos = i;
        }
    }
    return zero_pos;
}


void QamDemodulator() {
   
    int16_t adcRawBuffer[ADC_BUFFER_SIZE];
    int16_t I_Buffer[ADC_BUFFER_SIZE];
    int16_t Q_Buffer[ADC_BUFFER_SIZE];
    int16_t adcValue;
    int16_t avarage_Value;

    float map_Value = 0;
    int8_t I_Bit = 0;
    int8_t Q_Bit = 0;

    for (int i = 0; i < 16; i++) {
        
        for (int i = 0 + sync_Phase; i < ADC_BUFFER_SIZE; i++) {
        esp_rom_delay_us(50);   // 50 µs warten

            for(int i = ADC_BUFFER_SIZE-1; i > 0; i--) {
                adcRawBuffer[i] = adcRawBuffer[i-1];
            }

            adcValue = adc_get_raw(AN0) >> (ADC_resolution - 8);
            adcValue = (adcValue - Offset) * Invert;
            adcRawBuffer[0] = adcValue;
        }


        switch (mode) {
            case Idle:
                avarage_Value = average_filter (adcRawBuffer , ADC_BUFFER_SIZE);
                if (abs(avarage_Value) > Start_Offset )
                {
                    sync_Amplitude = abs (array_max (adcRawBuffer , ADC_BUFFER_SIZE));
                    sync_Phase = array_zero_pos (adcRawBuffer , ADC_BUFFER_SIZE);
                    mode = Read;
                }
                break;

            case Read: 
                for(int i = 0; i < ADC_BUFFER_SIZE; i++) {
                    I_Buffer[i] = adcRawBuffer[i] * sinetable[i];
                    Q_Buffer[i] = adcRawBuffer[i] * cosinetable[i];
                }

                map_Value = average_filter (I_Buffer , ADC_BUFFER_SIZE);
                    if (map_Value >= -4 && map_Value <= -2) {
                        I_Bit = 0b00;
                    } else if (map_Value >= -2 && map_Value <= 0) {
                        I_Bit = 0b01;
                    } else if (map_Value >= 0 && map_Value <= 2) {
                        I_Bit = 0b11;
                    } else if (map_Value >= 2 && map_Value <= 4) {
                        I_Bit = 0b10;
                    }

                    map_Value = average_filter (Q_Buffer , ADC_BUFFER_SIZE);
                    if (map_Value >= -4 && map_Value <= -2) {
                        Q_Bit = 0b00;
                    } else if (map_Value >= -2 && map_Value <= 0) {
                        Q_Bit = 0b01;
                    } else if (map_Value >= 0 && map_Value <= 2) {
                        Q_Bit = 0b11;
                    } else if (map_Value >= 2 && map_Value <= 4) {
                        Q_Bit = 0b10;
                    }
            break;            
        }  
        

        uint8_t Symbol = (I_Bit << 2) | Q_Bit;
        uint8_t shift = 12 - (4 * i);  // MSB zuerst
        Data |= ((uint16_t)Symbol << shift);
    }
    QAM_receive = true;

    while (QAM_receive)
    {
        vTaskDelay(1);
    }
}

uint64_t QAM_get_Data()
{   
    uint64_t Data_out;

    QAM_receive = false;
    Data_out = Data;
    Data = 0xFFFF;
    return Data_out;
}