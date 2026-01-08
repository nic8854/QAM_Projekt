#pragma once
#include <stdbool.h>
#include "eduboard2.h"

#if defined(QAM_TX_MODE) || (defined(QAM_TRX_MODE) && defined(TRX_ROUTE_PACKET))

void GuiDriver_init(void);
bool GuiDriver_receiveTemperature(float temperature);
bool GuiDriver_receiveText(char text[4]);

#endif