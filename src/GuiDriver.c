/********************************************************************************************* */
//    GuiDriver
//    Author: Remo Bachmann
//    Juventus Technikerschule
//    Version: 1.0.0
/********************************************************************************************* */


#include "eduboard2.h"
#include "GuiDriver.h"


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// -------------------- Settings --------------------
#define GUI_PERIOD_MS          100      // Display refresh
#define GUI_QUEUE_LEN          32

#define HISTORY_LEN            300      // 300 samples (z.B. 5min @1Hz)

#define PLOT_X                 10
#define PLOT_Y                 120  // 80 alter wert
#define PLOT_W                 410
#define PLOT_H                 120

#define HEADER_X               10
#define HEADER_Y               30

// Wenn du lieber fixen Bereich willst:
//#define USE_AUTOSCALE          1
#define TEMP_MIN_FIXED         15.0f
#define TEMP_MAX_FIXED         35.0f
// --------------------------------------------------


#undef  PLOT_W
#define PLOT_W                 HISTORY_LEN   // CHANGE: vorher 410

typedef struct {
    float tempC;
} GuiSample_t;

// Separate Message-Struktur für Text-Queue (4 Zeichen)
// WICHTIG: nicht null-terminiert in der Queue, wird erst für Anzeige terminiert
typedef struct {
    char text[4];
} GuiText_t; 

static QueueHandle_t s_guiQ = NULL;

// zweite Queue für Text 
static QueueHandle_t s_textQ = NULL; 

static float  s_hist[HISTORY_LEN];
static uint16_t s_head = 0;
static uint16_t s_count = 0;
static float s_latest = NAN;

//  zuletzt empfangener Text (als C-String für lcdDrawString)
static char s_latestText[5] = "----"; // 4 Zeichen + '\0'

//  Statistik: wie viele Werte beim Senden verworfen wurden (Queue voll)
static uint32_t s_dropTemp = 0; 
static uint32_t s_dropText = 0; 

//  NEU: zusammengeführte Textzeile (mindestens 48 Zeichen) für die Anzeige
//  - Wir speichern IMMER exakt die letzten 48 Zeichen (Rolling Window)
//  - Neue 4-Zeichen-Blöcke werden rechts angehängt, links fällt entsprechend weg
//  - Für lcdDrawString terminieren wir mit '\0'
#define GUI_TEXT_LINE_LEN      48
static char s_textLine[GUI_TEXT_LINE_LEN + 1]; // 48 Zeichen + '\0'

static void hist_push(float v)
{
    s_hist[s_head] = v;
    s_head = (uint16_t)((s_head + 1) % HISTORY_LEN);
    if (s_count < HISTORY_LEN) s_count++;
}

static float hist_get_oldest(uint16_t idx_from_oldest)
{
    // idx_from_oldest: 0..s_count-1
    uint16_t oldest = (uint16_t)((s_head + HISTORY_LEN - s_count) % HISTORY_LEN);
    uint16_t idx = (uint16_t)((oldest + idx_from_oldest) % HISTORY_LEN);
    return s_hist[idx];
}

static void hist_minmax(float *outMin, float *outMax)
{
#if USE_AUTOSCALE
    if (s_count == 0) { *outMin = TEMP_MIN_FIXED; *outMax = TEMP_MAX_FIXED; return; }

    float mn =  1e9f;
    float mx = -1e9f;
    for (uint16_t i = 0; i < s_count; i++) {
        float v = hist_get_oldest(i);
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }

    // Padding
    if (mx - mn < 0.2f) { mx += 0.1f; mn -= 0.1f; }
    *outMin = mn - 0.5f;
    *outMax = mx + 0.5f;
#else
    *outMin = TEMP_MIN_FIXED;
    *outMax = TEMP_MAX_FIXED;
#endif
}

static uint8_t temp_to_u8(float t, float tMin, float tMax)
{
    if (tMax <= tMin) return 0;
    float n = (t - tMin) / (tMax - tMin); // 0..1
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    // lcdDrawDataUInt8 erwartet Werte im Bereich [yMin..yMax]
    return (uint8_t)(n * 255.0f);
}

//  NEU: 4 Zeichen in die 48-Zeichen-Anzeigezeile einfügen (Rolling Window)
//  - schiebt die bestehende Zeile um 4 nach links
//  - hängt die neuen 4 Zeichen rechts an
/*static void textline_push4(const char in4[4])
{
    memmove(&s_textLine[0], &s_textLine[4], GUI_TEXT_LINE_LEN - 4);
    memcpy(&s_textLine[GUI_TEXT_LINE_LEN - 4], in4, 4);
    s_textLine[GUI_TEXT_LINE_LEN] = '\0';
}
*/

static void textline_push4(const char in4[4])
{
    uint16_t len = (uint16_t)strlen(s_textLine);

    // Wenn voll: Rolling Window (links 4 weg)
    if (len >= GUI_TEXT_LINE_LEN) {
        memmove(&s_textLine[0], &s_textLine[4], GUI_TEXT_LINE_LEN - 4);
        len = (uint16_t)(GUI_TEXT_LINE_LEN - 4);
        s_textLine[len] = '\0';
    }

    // Falls weniger als 4 Platz: so kürzen, dass 4 reinpassen
    if (len > (GUI_TEXT_LINE_LEN - 4)) {
        uint16_t overflow = (uint16_t)(len - (GUI_TEXT_LINE_LEN - 4));
        memmove(&s_textLine[0], &s_textLine[overflow], (size_t)(len - overflow));
        len = (uint16_t)(len - overflow);
        s_textLine[len] = '\0';
    }

    // 4 Zeichen anhängen
    memcpy(&s_textLine[len], in4, 4);
    s_textLine[len + 4] = '\0';
}

static void draw_screen(void)
{
   lcdFillScreen(BLACK);

    // Titel
    lcdDrawString(fx32M, 10, HEADER_Y, "Temperature", GREEN);

    // Aktueller Wert
    char line[64];
    if (!isnan(s_latest)) {
        snprintf(line, sizeof(line), "Current: %.2f deg C", s_latest);
    } else {
        snprintf(line, sizeof(line), "Current: --.-- deg C");
    }
    lcdDrawString(fx24M, 10, 60, line, WHITE);



    //  Anzeige vom letzten Text (4 Zeichen)
    
    //  NEU: zusammengeführte Textzeile (48 Zeichen) unter "Current" ausgeben
    //  - Die Zeile ist IMMER mindestens 48 Zeichen lang (intern 48 Zeichen + '\0')
    //  - Auf dem Display zeigen wir exakt 48 Zeichen an (ggf. mit Leerzeichen aufgefüllt)
    //char textLine[64]; 
    //snprintf(textLine, sizeof(textLine), "Text: %.48s", s_textLine); 
    //lcdDrawString(fx24M, 10, 85, textLine, WHITE); //  unter Current (Position ggf. anpassen)

    //  NEU: führende Leerzeichen für die Anzeige überspringen
//  - Intern bleibt s_textLine weiterhin exakt 48 Zeichen lang
//  - Auf dem Display wird links kein "leerer Abstand" angezeigt
/*const char *p = s_textLine;
while (*p == ' ' && *(p + 1) != '\0') {
    p++;   // führende Leerzeichen überspringen
}
*/
char textLine[64];
snprintf(textLine, sizeof(textLine), "Text: %s", s_textLine);
lcdDrawString(fx24M, 10, 85, textLine, WHITE); // unter "Current"

/*
char textLine[64];
snprintf(textLine, sizeof(textLine), "Text: %s", p);
lcdDrawString(fx24M, 10, 85, textLine, WHITE); // unter "Current"
*/

    // Plot-Rahmen
    lcdDrawRect(PLOT_X, PLOT_Y, PLOT_X + PLOT_W, PLOT_Y + PLOT_H, BLUE);

    // Skala bestimmen
    float tMin, tMax;
    hist_minmax(&tMin, &tMax);

    // Skala anzeigen
    char scaleTxt[64];
    snprintf(scaleTxt, sizeof(scaleTxt), "min %.1fC / max %.1fC", tMin, tMax);
    lcdDrawString(fx16M, PLOT_X + 10, PLOT_Y + 10, scaleTxt, WHITE);

    //  Statistik anzeigen (wie gut das System performt)
    //  Drop-Counter (Queue voll -> Werte verworfen)
    char statTxt[64]; 
    snprintf(statTxt, sizeof(statTxt), "drops T:%lu X:%lu",
             (unsigned long)s_dropTemp, (unsigned long)s_dropText); 
    lcdDrawString(fx16M, PLOT_X + 10, PLOT_Y + 28, statTxt, WHITE); 

    // Verlauf in uint8 umrechnen für lcdDrawDataUInt8
    static uint8_t plotBuf[HISTORY_LEN];
    memset(plotBuf, 0, sizeof(plotBuf));

    // Wir zeichnen HISTORY_LEN Punkte, aber falls weniger da sind: links leer lassen
    uint16_t n = s_count;
    if (n > HISTORY_LEN) n = HISTORY_LEN;

    // plotBuf ist “zeitlich”: links alt -> rechts neu (wie im ADC-Test)
    // lcdDrawDataUInt8 zeichnet die Daten wie geliefert über "length" Samples.
    // Wir füllen ein Fenster von HISTORY_LEN, die letzten n Samples rechts.
    uint16_t start = (uint16_t)(HISTORY_LEN - n);
    for (uint16_t i = 0; i < n; i++) {
        float v = hist_get_oldest(i);
        plotBuf[start + i] = temp_to_u8(v, tMin, tMax);
    }

    // Plot (yMin=0..255)
    // (x,y,width,height,min,max,???, data, color)
    lcdDrawDataUInt8(PLOT_X, PLOT_Y, HISTORY_LEN, PLOT_H, 0, 255, false, plotBuf, YELLOW);



    lcdUpdateVScreen();
}

static void GuiDriver_task(void *pv)
{
    (void)pv;
    TickType_t last = xTaskGetTickCount();


    for (;;) {
        // neue Samples abholen (alle, die anstehen)
        GuiSample_t s;
        if (xQueueReceive(s_guiQ, &s, 0) == pdTRUE) {
            s_latest = s.tempC;
            hist_push(s_latest);
        }

        // Text-Queue ebenfalls leeren, aktuellsten Text behalten
        GuiText_t t; 
        //  NEU: alle anstehenden 4-Zeichen-Blöcke abholen und in die 48-Zeichen-Zeile einfügen
        if (xQueueReceive(s_textQ, &t, 0) == pdTRUE) { 
            memcpy(s_latestText, t.text, 4); // exakt 4 chars
            s_latestText[4] = '\0';          //  für Anzeige terminieren

            //  NEU: 4 Zeichen an die Laufzeile anhängen (Rolling Window)
            textline_push4(t.text);
        }

        draw_screen();

        vTaskDelayUntil(&last, pdMS_TO_TICKS(GUI_PERIOD_MS));
    }
}


void GuiDriver_init(void) 
{
    lcdFillScreen(BLACK);
    // Queue
    s_guiQ = xQueueCreate(GUI_QUEUE_LEN, sizeof(GuiSample_t));

    //Text-Queue erzeugen
    s_textQ = xQueueCreate(GUI_QUEUE_LEN, sizeof(GuiText_t)); 

    // History reset
    memset(s_hist, 0, sizeof(s_hist));
    s_head = 0;
    s_count = 0;
    s_latest = NAN;

    //  Text + Stats reset
    memcpy(s_latestText, "----", 5);
    s_dropTemp = 0;                 
    s_dropText = 0;                 

    // Task starten
    xTaskCreate(
        GuiDriver_task,
        "GuiDriver",
        3 * 2048,
        NULL,
        4,
        NULL
    );
}


bool GuiDriver_receiveTemperature(float tempC) 
{
    if (s_guiQ == NULL) return false;
    GuiSample_t s = {.tempC = tempC};

    // CHANGE: Drop zählen, falls Queue voll ist (statt still fehlschlagen)
    if (xQueueSend(s_guiQ, &s, 0) != pdTRUE) { // CHANGE
        s_dropTemp++;                          
        return false;
    }
    return true;
}

// Text-Empfangsfunktion laut API (4 Zeichen pro Element)
bool GuiDriver_receiveText(char text[4]) 
{
    if (s_textQ == NULL) return false;

    GuiText_t t;                 
    memcpy(t.text, text, 4);     //  genau 4 Zeichen übernehmen

    //  Drop zählen, falls Queue voll ist
    if (xQueueSend(s_textQ, &t, 0) != pdTRUE) { 
        s_dropText++;                            
        return false;
    }
    return true;
}
#endif