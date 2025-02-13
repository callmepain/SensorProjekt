#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <Adafruit_SSD1306.h>

// Display-Einstellungen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern Adafruit_SSD1306 display;

// Prototypen für Display-Funktionen
void setDisplayState(bool state);
bool isDisplayOn();
bool getDisplayState();

#endif