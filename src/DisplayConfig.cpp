#include "DisplayConfig.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Interner Zustand des Displays
static bool displayOn = true;

void setDisplayState(bool state) {
    displayOn = state;
    if (displayOn) {
        display.ssd1306_command(SSD1306_DISPLAYON); // Display einschalten
    } else {
        display.ssd1306_command(SSD1306_DISPLAYOFF); // Display ausschalten
    }
}

bool isDisplayOn() {
    return displayOn;
}