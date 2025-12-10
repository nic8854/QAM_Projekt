#include "QamDemodulator.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "eduboard2.h"
#include "math.h"
#include <stdbool.h>
#include <stdint.h>

#define TAG "QAM"

#define PI          3.14
#define sqrt18      4.24264
#define Ampl_max    1862    // 1.5V
//#define Offset      1241    // => 1V
//#define Offset      1200    // => 1V
#define Offset      1900    // => 1.65V
#define Invert      1
//#define Invert      -1


#define Input_Pin   AN1
#define Anz_Bits        64       // Anzahl Bits

#define ADC_BUFFER_SIZE 256      // über 2 Wellen messen bei 50uS Sample Rate
#define ADC_Read_BUFFER_SIZE 16*256 // Komplette Zeichenfolge
#define ADC_resolution 12

#define Sin_Cosin_Buffer_Size 256 // über 2 Wellen rechnen  

#define Start_Offset   10


typedef enum {
    Idle,
    Read
} State;
State mode = Idle;

bool QAM_receive = false;

static int16_t sinetable[Sin_Cosin_Buffer_Size];
static int16_t cosinetable[Sin_Cosin_Buffer_Size];

static int16_t sync_Amplitude = 1862;      // 1.5Vpp muss immer auf die Amplitude des entferntesten Musters aufgerechnet werden
static uint8_t max_steilheit =  100;//(((Ampl_max/1862)/*sync_Amplitude*/) * 60) + 20;
static uint8_t zero_pos;
static uint8_t readb_buff_pos;
static bool Start = false;
static bool Buffer_compl = false;
static uint8_t buffer_cnt = 0;
// static uint8_t Phase = 0;
// static uint8_t sync_Phase = 0;

static uint64_t Data = 0;


static int16_t adcQAMBuffer[ADC_BUFFER_SIZE];
static int16_t adcReadBuffer[ADC_Read_BUFFER_SIZE];

void adc_callback_function(){
    float scale = 1;//(float)Ampl_max / (float)sync_Amplitude;
    adc_get_QAM_buffer(Invert,scale,Offset,Input_Pin, adcQAMBuffer);
    
    if (!Start){
        for (int i = 0; i < ADC_BUFFER_SIZE-1; i++)
        {
            if (abs(adcQAMBuffer[i+1] - adcQAMBuffer[i]) > max_steilheit)
            {
                buffer_cnt = 0;
                Buffer_compl = false;
                zero_pos = i;
                readb_buff_pos = i;
                Start = true;
                led_toggle(LED7);
                break;
                // Fehler: zu schnelle Änderung
            }
        }
    }    
    if (Start)
    {     
        if (buffer_cnt < 17)
        {
            for (int i = zero_pos; i < ADC_BUFFER_SIZE; i++)
            {
                adcReadBuffer[i-readb_buff_pos + (buffer_cnt*ADC_BUFFER_SIZE)] = adcQAMBuffer[i];
            }
            zero_pos = 0;
        }
        else
        {
            Start = false;
            Buffer_compl = true;
        }
        buffer_cnt++;
    }
    //Phase++;
    //QamDemodulator();
}

// void sync_Phase(uint8_t Winkel){
//     uint8_t Zero = array_zero_pos(adcQAMBuffer,ADC_BUFFER_SIZE); 

// }

void InitQamDemodulator(){
    // Sinus und Cosiunustabelle erzeugen
    for (int i = 0; i < Sin_Cosin_Buffer_Size; i++) {
        float val = sinf(2.0f * M_PI * i / (Sin_Cosin_Buffer_Size));    // -1..1
        sinetable[i] = (int16_t)(val * Ampl_max/sqrt18);                       

        val = cosf(2.0f * M_PI * i / Sin_Cosin_Buffer_Size);            // -1..1
        cosinetable[i] = (int16_t)(val * Ampl_max/sqrt18);                     
    }

    // ADC init
    memset(adcQAMBuffer, 0x00, sizeof(adcQAMBuffer));
    adc_set_stream_callback(&adc_callback_function);


    xTaskCreate(QamDemodulator,   //Subroutine
                "QAMDemodulator",  //Name
                2*2048,         //Stacksize
                NULL,           //Parameters
                5,             //Priority
                NULL);          //Taskhandle
}


float average_filter(int32_t *buffer, int length) {
    float sum = 0.0f;

    for (int i = 0; i < length; i++) {
        sum += buffer[i];
    }

    return sum / length;
}

int32_t array_max(int32_t *arr, int length) {
    int16_t max = arr[0];

    for (int i = 1; i < length; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

uint32_t array_zero_pos(int32_t *arr, int length) {
    
    uint8_t zero_pos = 0;

    for(int i = 0; i < length-1; i++) {
        // Bedingung für positiven Nulldurchgang: negativ -> positiv
        if(arr[i] < 0 && arr[i+1] > 0) {
            zero_pos = i;
        }
    }
    return zero_pos;
}

void QamDemodulator(){
    while(1){
    vTaskDelay(200);
    if (Buffer_compl)
    {
        for (size_t i = 0; i < ADC_Read_BUFFER_SIZE; i++)
        {
            printf("%d\n", adcReadBuffer[i]);
        }
    Buffer_compl = false;
    }
}
}

// void QamDemodulator() {
   
//     int32_t I_Buffer[Sin_Cosin_Buffer_Size];
//     int32_t Q_Buffer[Sin_Cosin_Buffer_Size];
//     int16_t avarage_Value;
//     int16_t Ampl_Value;

//     float map_Value = 0;
//     int8_t I_Bit = 0;
//     int8_t Q_Bit = 0;


//     int32_t adc_Buffer[Sin_Cosin_Buffer_Size];

    

        
//     switch (mode) {
//         case Idle:led_toggle(LED5);
//             //avarage_Value = average_filter (adc_Buffer , Sin_Cosin_Buffer_Size);
//             Ampl_Value = array_max (adc_Buffer , Sin_Cosin_Buffer_Size);
//             if (abs(Ampl_Value) > Start_Offset )
//             //if (abs(avarage_Value) > Start_Offset )
//             {led_toggle(LED6);
//                 sync_Amplitude = abs (array_max (adc_Buffer , Sin_Cosin_Buffer_Size));
//                 sync_Phase = array_zero_pos (adc_Buffer , Sin_Cosin_Buffer_Size);
//                 mode = Read;
                
//             }
//         break;

//         case Read: 
//             for (int i = 0; i < 18; i++) {
//                 for(int w = 0; w < Sin_Cosin_Buffer_Size; w++) {
//                     I_Buffer[w] = adc_Buffer[w] * sinetable[w];
//                     Q_Buffer[w] = adc_Buffer[w] * cosinetable[w];
//                 }

//                 map_Value = average_filter (I_Buffer , Sin_Cosin_Buffer_Size);
//                     if (map_Value >= -4 && map_Value <= -2) {
//                         I_Bit = 0b00;
//                     } else if (map_Value >= -2 && map_Value <= 0) {
//                         I_Bit = 0b01;
//                     } else if (map_Value >= 0 && map_Value <= 2) {
//                         I_Bit = 0b11;
//                     } else if (map_Value >= 2 && map_Value <= 4) {
//                         I_Bit = 0b10;
//                     }

//                 map_Value = average_filter (Q_Buffer , Sin_Cosin_Buffer_Size);
//                     if (map_Value >= -4 && map_Value <= -2) {
//                         Q_Bit = 0b00;
//                     } else if (map_Value >= -2 && map_Value <= 0) {
//                         Q_Bit = 0b01;
//                     } else if (map_Value >= 0 && map_Value <= 2) {
//                         Q_Bit = 0b11;
//                     } else if (map_Value >= 2 && map_Value <= 4) {
//                         Q_Bit = 0b10;
//                     }

//                 QAM_receive = true;
//                 mode = Idle;
                

//             uint8_t Symbol = 0x0f & ((I_Bit << 2) | Q_Bit);
//             uint8_t shift = (4 * i);
//             Data |= Symbol << shift;
//             }
//             led_toggle(LED7);
//         break;              
//     }
    

    // while (QAM_receive)
    // {
    //     vTaskDelay(1);
    // }
//}

uint64_t QAM_get_Data()
{   
    uint64_t Data_out;
    led_toggle(LED6);
    QAM_receive = false;
    Data_out = Data;
    Data = 0xFFFF;
    return Data_out;
}