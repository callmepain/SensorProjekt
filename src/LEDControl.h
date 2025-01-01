#ifndef LEDCONTROL_H
#define LEDCONTROL_H

#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class LEDControl {
public:
    LEDControl(Adafruit_NeoPixel& strip);

    void init();
    void update(float temperature);
    void setProgress(int progress, int totalSteps);
    void fadeOut(unsigned long duration, int targetFPS);
    void setBrightness(int brightness);
    void setTransitionSpeed(float speed);
    void adjustBrightness(int potValue);
    void turnOn();
    void turnOff();
    bool isOn() const;

private:
    Adafruit_NeoPixel& strip;
    SemaphoreHandle_t ledMutex;
    float transitionSpeed;
    uint32_t currentColor;
    bool lightState;
    int currentBrightness;
    int lastPotValue;

    uint32_t getColorForTemperature(float temp);
    uint32_t interpolateColor(uint32_t color1, uint32_t color2, float ratio);
    float easeInOutQuad(float t);
    int readPotValueWithTolerance(int currentValue, int tolerance = 100);
};

#endif // LEDCONTROL_H
