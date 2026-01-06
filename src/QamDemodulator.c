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
#define sqrt2       1.412135

#define Ampl_Start  1820    // 1.5V
#define Ampl_max    2500    
#define Ampl_min    2100   
//#define Offset      1935    // => 1.65V
#define Offset      1642    // => 1.4V
//c#define Invert      1
#define Invert      -1


#define Input_Pin       AN1
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

static float sinetable[Sin_Cosin_Buffer_Size];
static float cosinetable[Sin_Cosin_Buffer_Size];

static int16_t sync_Amplitude = 1828;      // 1.5Vpp muss immer auf die Amplitude des entferntesten Musters aufgerechnet werden
static uint8_t max_steilheit =  100;       // (((Ampl_max/1862)/*sync_Amplitude*/) * 60) + 20;
static uint8_t zero_pos;
static uint8_t readb_buff_pos;
static bool Start = false;
static bool Buffer_compl = false;
static uint8_t buffer_cnt = 0;


static uint64_t Data = 0;

static int16_t adcQAMBuffer_raw[ADC_BUFFER_SIZE];
static int16_t adcQAMBuffer[ADC_BUFFER_SIZE];
static int16_t adcReadBuffer[ADC_Read_BUFFER_SIZE];

void adc_callback_function(){
    float scale = 1;
    adc_get_QAM_buffer(Invert,scale,Offset,Input_Pin, adcQAMBuffer_raw);
    moving_median(adcQAMBuffer_raw,adcQAMBuffer,ADC_BUFFER_SIZE,5);
    
    if (!Start){

        //sync Amplitude
        int16_t Ampl_meas = (float) 2*sqrt18*(array_max(adcQAMBuffer,ADC_BUFFER_SIZE));    //sync Amplitude
        sync_Ampl (Ampl_meas);

        for (int i = 0; i < ADC_BUFFER_SIZE-1; i++)
        {
            max_steilheit = (float)((sync_Amplitude * PI) / (Sin_Cosin_Buffer_Size/2) + 20);

            if (abs(adcQAMBuffer[i+1] - adcQAMBuffer[i]) > max_steilheit)
            {
                //sync Amplitude
                int16_t Ampl_meas = (float) 2*(array_max(adcQAMBuffer,ADC_BUFFER_SIZE));    //sync Amplitude
                sync_Ampl (Ampl_meas);
                
                buffer_cnt = 0;
                Buffer_compl = false;
                zero_pos = i+1;
                readb_buff_pos = zero_pos;
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
                if ((i-readb_buff_pos + buffer_cnt*ADC_BUFFER_SIZE) >= ADC_Read_BUFFER_SIZE)
                    break;
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
}



int cmp(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}
void moving_median(int16_t *input, int16_t *output,
                   size_t length, size_t window)
{
    size_t half = window / 2;
    int buffer[window];   // C99: variable length array

    for (size_t i = 0; i < length; i++) {
        size_t k = 0;

        for (int j = (int)i - (int)half;
                 j <= (int)i + (int)half; j++) {

            int idx = j;
            if (idx < 0) idx = 0;
            if (idx >= (int)length) idx = length - 1;

            buffer[k++] = input[idx];
        }

        qsort(buffer, window, sizeof(int), cmp);
        output[i] = buffer[half];
    }
}

float average_filter(float *buffer, int length) {
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

void sync_Ampl(int16_t Ampl_meas)
{
    if (Ampl_meas > sync_Amplitude && sync_Amplitude < Ampl_max)
        sync_Amplitude += 5;
    if (Ampl_meas < sync_Amplitude && sync_Amplitude > Ampl_min)
        sync_Amplitude -= 5;
}


uint64_t map_QAM_Buffer(int16_t *adcBuf)
{
    float norm_factor = (float)18/sync_Amplitude;   // Signal normalisieren
    float sym_arr[256];
    uint8_t symbol;
    uint64_t Data = 0;
    for (int s = 0; s < 16; s++) {
        
        float I = 0.0f;
        float Q = 0.0f;

        for (int n = 0; n < 256; n++) {
           sym_arr[n] = adcBuf[n+(s*256)] * sinetable[n] * norm_factor;
        }
        Q = average_filter(sym_arr,256);   //integral über 2 Wellen
        
        for (int n = 0; n < 256; n++) {
           sym_arr[n] = adcBuf[n+(s*256)] * cosinetable[n] * norm_factor;
        }
        I = average_filter(sym_arr,256);   //integral über 2 Wellen
        
        //for (int n = 0; n < 256; n++)
            //printf("%f\n", sym_arr[n]);
        //printf("%f\n", I);
        //printf("%f\n", Q);
        // Decision Mapping
        int I_idx = (I < -2) ? 0 :
                    (I <  0) ? 1 :
                    (I <  2) ? 3 : 2;

        int Q_idx = (Q < -2) ? 0 :
                    (Q <  0) ? 1 :
                    (Q <  2) ? 3 : 2;

        // 4-bit Symbol
        symbol = (I_idx << 2) | (Q_idx);
        Data |= ((uint64_t)symbol << (4 * (15 - s)));
    }
    return Data;
}




uint64_t QAM_get_Data()
{   
    uint64_t Data_out;
    led_toggle(LED6);
    QAM_receive = false;
    Data_out = Data;
    Data = 0xFFFF;
    return Data_out;
}


void QamDemodulator(){
    while(1){
    vTaskDelay(200);
    if (Buffer_compl)
    {   
        for (size_t i = 0; i < ADC_Read_BUFFER_SIZE; i++)
        {
            //printf("%d\n", adcReadBuffer[i]);
        }
        Data = map_QAM_Buffer(adcReadBuffer);
    Buffer_compl = false;
    printf("QAM Packet = 0x%016llX\n", (unsigned long long)Data);
    }
}
}

void InitQamDemodulator(){

    sync_Amplitude = Ampl_Start;
    // Sinus und Cosiunustabelle erzeugen
    for (int i = 0; i < Sin_Cosin_Buffer_Size; i++) {
        float val = sinf(2.0f * M_PI * i / ((Sin_Cosin_Buffer_Size/2)));    // -1..1
        sinetable[i] = (float)(val);                       

        val = cosf(2.0f * M_PI * i / (Sin_Cosin_Buffer_Size/2));            // -1..1
        cosinetable[i] = (float)(val);                     
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