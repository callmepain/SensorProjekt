#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SDCardLogger.h"

class Battery {
private:
    const char* filePath;
    static const int SAMPLE_SIZE = 5;  
    float voltageSamples[SAMPLE_SIZE];
    float currentSamples[SAMPLE_SIZE];
    int voltageSampleIndex = 0; // Index für voltageSamples
    int currentSampleIndex = 0; // Index für currentSamples
    int sampleIndex;
    float totalEnergy;     
    int batteryCapacity;
    float internalResistance; // in Ohm

    // Task-Variablen
    TaskHandle_t taskHandle = nullptr;
    uint32_t taskDelayMs;

    SDCardLogger& sdLogger;  // Neue Referenz

    void addSample(float *samples, int &sampleIndex, float newSample);
    void saveToJson(float energy);

    // **Task-Funktion als static** (damit xTaskCreate darauf zugreifen kann)
    static void updateTask(void *param);

public:
    // Konstruktor
    Battery(int capacity, const char* path, SDCardLogger& logger, float resistance = 0.03);
    void initialize();

    // Methoden
    float calculateChargingTime(float current_mA);
    void initializeSamples(float initialVoltage, float initialCurrent);
    void update(float voltage, float current_mA, float energy_mWh);
    int calculateSOC(float voltage);
    float calculateRemainingTime();
    float getTotalEnergy() const;
    unsigned long getCount();
    float getSmoothedVoltage();
    float getSmoothedCurrent();
    void loadFromJson();

    // **Neue Methode zum Starten des Tasks**
    void startUpdateTask(uint32_t intervalMs = 1000, 
                         UBaseType_t priority = 1, 
                         BaseType_t coreID = 1);

    // Neue Funktionen
    float getCurrent();        // Aktuellen Strom in mA
    float getCurrentSigned();  // Strom mit Vorzeichen (+ = Entladung, - = Ladung)
    float getPower();         // Aktuelle Leistung in mW
};

#endif
