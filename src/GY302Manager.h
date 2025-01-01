#ifndef GY302_MANAGER_H
#define GY302_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <vector>

class GY302Manager {
public:
    // Konstruktor: i2cAddress kann z.B. 0x23 oder 0x5C sein
    GY302Manager(uint8_t i2cAddress = 0x23, size_t maxSamples = 20)
        : address(i2cAddress), maxSamples(maxSamples) {}

    // Initialisierung
    bool begin(TwoWire &i2cPort = Wire) {
        // BH1750::CONTINUOUS_HIGH_RES_MODE: kontinuierliche Messung, hohe Auflösung
        return bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, address, &i2cPort);
    }

    // Starte Sampling in einem eigenen Task
    void startSampling(unsigned long intervalMs) {
        this->intervalMs = intervalMs;
        xTaskCreatePinnedToCore(
            samplingTask,          // Task-Funktion
            "GY302_Sampling",      // Name des Tasks
            2048,                  // Stack-Größe
            this,                  // Übergabeparameter (this)
            1,                     // Task-Priorität
            &taskHandle,           // Handle für den Task
            1                      // Core (1 für Core 1)
        );
    }

    // Durchschnitt der letzten Werte abrufen
    float getAverageLux() {
        xSemaphoreTake(mutex, portMAX_DELAY); // Mutex sperren
        float avg = 0;
        if (!samples.empty()) {
            for (float sample : samples) {
                avg += sample;
            }
            avg /= samples.size();
        }
        xSemaphoreGive(mutex); // Mutex freigeben
        return avg;
    }

    // Lux-Wert abrufen
    float readLux() {
        return bh1750.readLightLevel();
    }

private:
    BH1750 bh1750;
    uint8_t address;
    size_t maxSamples;
    std::vector<float> samples;
    unsigned long intervalMs = 1000;
    TaskHandle_t taskHandle = nullptr;
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

    // Task-Funktion
    static void samplingTask(void* parameter) {
        GY302Manager* instance = static_cast<GY302Manager*>(parameter);
        unsigned long lastSampleTime = millis();
        while (true) {
            if (millis() - lastSampleTime >= instance->intervalMs) {
                lastSampleTime = millis();
                float lux = instance->bh1750.readLightLevel();
                instance->addSample(lux);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms Pause
        }
    }

    // Füge neuen Sample hinzu und entferne ältesten, wenn nötig
    void addSample(float lux) {
        const float MAX_DEVIATION = 10000.0; // Maximal zulässige Abweichung vom Durchschnitt

        float avg = getAverageLux();
        if (!samples.empty() && fabs(lux - avg) > MAX_DEVIATION) {
            return; // Wert ignorieren, da er zu stark abweicht
        }

        xSemaphoreTake(mutex, portMAX_DELAY); // Mutex sperren
        if (samples.size() >= maxSamples) {
            samples.erase(samples.begin()); // Ältesten Wert entfernen
        }
        samples.push_back(lux);
        xSemaphoreGive(mutex); // Mutex freigeben
    }
};

#endif // GY302_MANAGER_H
