#ifndef LEDCONTROL_H
#define LEDCONTROL_H

#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct Transition {
    float progress;     // Fortschritt des Übergangs (0.0 bis 1.0)
    CRGB startColor; // Startfarbe
    CRGB endColor;   // Ziel-Farbe
    bool active;         // Gibt an, ob der Übergang aktiv ist

    Transition() : progress(0.0f), startColor(0), endColor(0), active(false) {}
};

class LEDControl {
public:
    LEDControl();

    void init();
    void update(float temperature);
    void setBrightness(int brightness);
    void runTask();
    void adjustBrightness(int potValue);
    void fadeBrightness(int targetBrightness, int duration);
    void turnOn();
    void turnOff();
    bool isOn() const;

private:
    Transition currentTransition;
    SemaphoreHandle_t ledMutex;
    float transitionSpeed;
    CRGB currentColor;
    bool lightState;
    int currentBrightness;
    int lastPotValue;
    int desiredFPS = 45;

    void startTransition(CRGB targetColor);
    CRGB getColorForTemperature(float temp);
    float easeInOutQuad(float t);
    int readPotValueWithTolerance(int currentValue, int tolerance = 100);
};

#endif // LEDCONTROL_H
