#ifndef VL53L1X_MANAGER_H
#define VL53L1X_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

class VL53L1XManager {
public:
    // Konstruktor mit optionaler I2C-Adresse und Verweis auf den I2C-Port
    VL53L1XManager(uint8_t i2cAddress = 0x29, TwoWire &i2cPort = Wire)
        : address(i2cAddress), wire(&i2cPort) {}

    bool begin() {
        if (!sensor.init(true)) {
            return false;
        }
        // Optional: I2C-Adresse setzen, falls mehrere Sensoren
        sensor.setAddress(address);
        
        // Beispiel: kontinuierlichen Modus starten
        sensor.setDistanceMode(VL53L1X::Long); // Kurz, Mittel oder Lang
        sensor.setMeasurementTimingBudget(50000); // in µs, 50ms
        sensor.startContinuous(50); // alle 50ms messen
        return true;
    }

    // Gibt die Distanz in Millimetern zurück
    uint16_t readDistance() {
        // sensor.read() aktualisiert den internen Messwert.
        sensor.read(false);
        return sensor.ranging_data.range_mm;
    }

    // Beispiel: Messstatus prüfen
    bool isRangeValid() {
        // 0 = gültig; andere Werte = Fehler / Warnungen
        return (sensor.ranging_data.range_status == 0);
    }

    // Sensor in den Stromsparmodus versetzen
    void enterLowPowerMode() {
        sensor.stopContinuous(); // Stoppt den kontinuierlichen Modus
        delay(1); // Kurze Pause, um sicherzustellen, dass der Sensor den Modus verlässt
    }

    // Sensor wieder aktivieren
    void exitLowPowerMode() {
        sensor.startContinuous(50); // Startet den kontinuierlichen Modus erneut
        delay(1); // Kurze Pause, um den Sensor zu stabilisieren
    }

private:
    VL53L1X sensor;
    TwoWire *wire;
    uint8_t address;
};

#endif // VL53L1X_MANAGER_H
