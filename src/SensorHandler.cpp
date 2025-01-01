#include "SensorHandler.h"
#include "MinMaxStorage.h"

SensorHandler::SensorHandler(Adafruit_BME280& bme)
    : bme(bme), temperature(NAN), humidity(NAN), pressure(NAN),
      bmeInitialized(false) {}

void SensorHandler::init() {
    // BME280 initialisieren
    if (bme.begin(0x76)) {
        bmeInitialized = true;
        Serial.println("BME280 erfolgreich initialisiert.");
    } else {
        bmeInitialized = false;
        Serial.println("Fehler: BME280 konnte nicht initialisiert werden!");
    }

    if (!bmeInitialized) {
        Serial.println("Keine Sensoren verfügbar. Fortsetzung ohne Sensordaten.");
    }Serial.println("Sensoren erfolgreich initialisiert.");
}

void SensorHandler::updateSensors() {
    if (bmeInitialized) {
        float newTemp = bme.readTemperature();
        float newHum = bme.readHumidity();
        float newPres = bme.readPressure() / 100.0F;

        // Plausibilitätsprüfung für Temperatur, Luftfeuchtigkeit und Druck
        if (newTemp >= -40.0 && newTemp <= 85.0) { // Gültiger Bereich für den BME280
            temperature = newTemp;

            // Min/Max-Temperatur aktualisieren
            bool updated = false;
            if (isnan(minTemp) || newTemp < minTemp) {
                minTemp = newTemp;
                updated = true;
            }
            if (isnan(maxTemp) || newTemp > maxTemp) {
                maxTemp = newTemp;
                updated = true;
            }
            if (updated) {
                saveMinMaxToJson(); // Min/Max-Werte speichern
                Serial.println("Min/Max-Temperaturen aktualisiert und gespeichert.");
            }
        } else {
            Serial.println("Warnung: Ungültiger Temperaturwert gelesen. Wert wird ignoriert.");
        }

        if (newHum >= 0.0 && newHum <= 100.0) { // Gültiger Bereich für Luftfeuchtigkeit
            humidity = newHum;
        } else {
            Serial.println("Warnung: Ungültiger Luftfeuchtigkeitswert gelesen. Wert wird ignoriert.");
        }

        if (newPres >= 300.0 && newPres <= 1100.0) { // Gültiger Bereich für Luftdruck in hPa
            pressure = newPres;
        } else {
            Serial.println("Warnung: Ungültiger Druckwert gelesen. Wert wird ignoriert.");
        }
    } else {
        Serial.println("BME280 nicht verfügbar, keine Aktualisierung der Umweltdaten.");
    }
}

void SensorHandler::checkSensors() {
    bool bmeStatus = testBME();

    if (bmeStatus != bmeInitialized) {
        bmeInitialized = bmeStatus;
        if (bmeInitialized) {
            Serial.println("BME280 erfolgreich erneut initialisiert.");
        } else {
            Serial.println("BME280 während der Laufzeit verloren.");
        }
    }
}

// Testet den BME280-Sensor
bool SensorHandler::testBME() {
    // Überprüfen, ob die Messdaten plausibel sind
    float temp = bme.readTemperature();
    return (temp > -40.0 && temp < 85.0); // Gültiger Temperaturbereich des BME280
}

float SensorHandler::getTemperature() const {
    return bmeInitialized ? temperature : NAN;
}

float SensorHandler::getHumidity() const {
    return bmeInitialized ? humidity : NAN;
}

float SensorHandler::getPressure() const {
    return bmeInitialized ? pressure : NAN;
}

bool SensorHandler::isBMEInitialized() const {
    return bmeInitialized;
}