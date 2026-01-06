#pragma once
#include <stdbool.h>


void GuiDriver_init(void);
bool GuiDriver_receiveTemperature(float temperature);
bool GuiDriver_receiveText(char text[4]);