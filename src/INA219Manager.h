#ifndef INA219_MANAGER_H
#define INA219_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <INA219_WE.h>

class INA219Manager {
public:
    // Konstruktor mit optionaler I2C-Adresse
    INA219Manager(uint8_t i2cAddress = 0x40, TwoWire* theWire = &Wire)
      : ina(i2cAddress), wire(theWire) {}

    // Initialisieren des INA219
    bool begin() {
        if (!ina.init()) {
            return false;
        }
        return true;
    }

    // ADC-Modus konfigurieren
    void setADCMode(INA219_ADC_MODE adcMode) {
        ina.setADCMode(adcMode);
    }

    // PGain konfigurieren
    void setPGain(INA219_PGAIN pGain) {
        ina.setPGain(pGain);
    }

    // Busspannungsbereich konfigurieren
    void setBusRange(INA219_BUS_RANGE range) {
        ina.setBusRange(range);
    }

    // Korrekturfaktor setzen
    void setCorrectionFactor(float factor) {
        ina.setCorrectionFactor(factor);
    }

    // Offset für Shunt-Spannung setzen
    void setShuntVoltOffset(float offset_mV) {
        ina.setShuntVoltOffset_mV(offset_mV);
    }

    // Beispieldaten abrufen
    float getBusVoltage_V() {
        return ina.getBusVoltage_V();
    }

    float getShuntVoltage_mV() {
        return ina.getShuntVoltage_mV();
    }

    float getCurrent_mA() {
        return ina.getCurrent_mA();
    }

    float getBusPower_mW() {
        return ina.getBusPower();
    }

    float getLoadVoltage_V() {
        return getBusVoltage_V() + (getShuntVoltage_mV() / 1000.0);
    }

    bool hasOverflow() {
        return ina.getOverflow();
    }

    // Energieverbrauch aktualisieren und abrufen
    float updateEnergy() {
        unsigned long currentTime = millis();
        if (lastUpdateTime > 0) { // Zeitdifferenz berechnen
            unsigned long deltaTime = currentTime - lastUpdateTime;
            float deltaTime_h = deltaTime / 3600000.0;    // Zeit in Stunden
            totalEnergy_mWh += getBusPower_mW() * deltaTime_h; // Energie in mWh
        }
        lastUpdateTime = currentTime;
        return totalEnergy_mWh;
    }

private:
    INA219_WE ina;
    TwoWire* wire;
    float totalEnergy_mWh = 0.0;  // Gesamtenergie in mWh
    unsigned long lastUpdateTime = 0; // Letzter Zeitstempel
};

#endif // INA219_MANAGER_H