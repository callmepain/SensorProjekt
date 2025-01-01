#ifndef LEDCONTROL_H
#define LEDCONTROL_H

#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct Transition {
    float progress;     // Fortschritt des Übergangs (0.0 bis 1.0)
    uint32_t startColor; // Startfarbe
    uint32_t endColor;   // Ziel-Farbe
    bool active;         // Gibt an, ob der Übergang aktiv ist

    Transition() : progress(0.0f), startColor(0), endColor(0), active(false) {}
};

class LEDControl {
public:
    LEDControl(Adafruit_NeoPixel& strip);

    void init();
    void update(float temperature);
    void setProgress(int progress, int totalSteps);
    void fadeOut(unsigned long duration, int targetFPS);
    void setBrightness(int brightness);
    void runTask();
    void setTransitionSpeed(float speed);
    void adjustBrightness(int potValue);
    void turnOn();
    void turnOff();
    bool isOn() const;

private:
    Transition currentTransition;
    Adafruit_NeoPixel& strip;
    SemaphoreHandle_t ledMutex;
    float transitionSpeed;
    uint32_t currentColor;
    bool lightState;
    int currentBrightness;
    int lastPotValue;

    void startTransition(uint32_t targetColor);
    uint32_t getColorForTemperature(float temp);
    uint32_t interpolateColor(uint32_t color1, uint32_t color2, float ratio);
    float easeInOutQuad(float t);
    int readPotValueWithTolerance(int currentValue, int tolerance = 100);
};

#endif // LEDCONTROL_H
