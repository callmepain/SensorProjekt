#include "Battery.h"
#include "INA219Manager.h"
#include "TelnetLogger.h"

extern TelnetLogger logger;
extern INA219Manager ina2;


// Konstruktor
Battery::Battery(int capacity, const char* path) 
    : batteryCapacity(capacity), filePath(path), sampleIndex(0)
{
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        voltageSamples[i] = 0;
        currentSamples[i] = 0;
    }
}

void Battery::initialize() {
    // JSON-Daten laden, nachdem das Dateisystem initialisiert wurde
    loadFromJson();
    Serial.println("Battery erfolgreich initialisiert.");
}

// Initialisierungsmethode für Samples
void Battery::initializeSamples(float initialVoltage, float initialCurrent) {
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        voltageSamples[i] = initialVoltage;
        currentSamples[i] = initialCurrent;
    }
}

// Durchschnitt berechnen und neuen Sample hinzufügen
void Battery::addSample(float *samples, int &sampleIndex, float newSample) {
    float positiveSample = fabs(newSample);

    samples[sampleIndex] = positiveSample;
    sampleIndex = (sampleIndex + 1) % SAMPLE_SIZE;
}

// JSON-Datei speichern
void Battery::saveToJson(float energy) {
    JsonDocument doc;  // oder größer je nach Bedarf
    doc["totalEnergy"] = energy;

    File file = SPIFFS.open(filePath, FILE_WRITE);
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Schreiben");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Fehler beim Schreiben der JSON-Daten");
    } else {
        //Serial.println("Energie Daten erfolgreich gespeichert.");
    }
    file.close();
}

// JSON-Datei laden
void Battery::loadFromJson() {
    File file = SPIFFS.open(filePath, FILE_READ);
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Lesen");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Fehler beim Lesen der JSON-Daten: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc["totalEnergy"].is<float>()) {
        totalEnergy = doc["totalEnergy"].as<float>();
    } else {
        totalEnergy = 0; // Standardwert
    }

    file.close();
    Serial.println("Energie Daten erfolgreich geladen.");
}

// Ladezeit berechnen
float Battery::calculateChargingTime(float current_mA) {
    float avgVoltage = getSmoothedVoltage();
    int soc = calculateSOC(avgVoltage);
    float remainingCapacity_mAh = batteryCapacity * (100 - soc) / 100.0;

    if (current_mA == 0) {
        return -1;  // kein Ladestrom
    }

    // Ladestrom positiv machen
    float positiveCurrent_mA = fabs(current_mA);

    float chargingTime = remainingCapacity_mAh / positiveCurrent_mA;
    float efficiency = 0.9; 
    chargingTime /= efficiency;

    return chargingTime;  // Stunden
}

// Update-Daten
void Battery::update(float voltage, float current_mA, float energy_mWh) {
    totalEnergy += energy_mWh / 1000.0;  

    // Spannung aktualisieren
    addSample(voltageSamples, voltageSampleIndex, voltage);

    // Strom aktualisieren
    addSample(currentSamples, currentSampleIndex, current_mA);

    // Daten in JSON speichern
    saveToJson(energy_mWh);
}

// State of Charge berechnen
int Battery::calculateSOC(float voltage) {
    if (voltage >= 4.2) return 100;
    if (voltage <= 3.3) return 0;
    return (int)((voltage - 3.3) / (4.2 - 3.3) * 100);
}

// Verbleibende Zeit berechnen
float Battery::calculateRemainingTime() {
    float avgVoltage = 0;
    float avgCurrent = 0;

    for (int i = 0; i < SAMPLE_SIZE; i++) {
        avgVoltage += voltageSamples[i];
        avgCurrent += currentSamples[i];
    }
    avgVoltage /= SAMPLE_SIZE;
    avgCurrent /= SAMPLE_SIZE;

    if (avgCurrent < 1.0) {
        return -1; // zu niedriger Verbrauch
    }

    int soc = calculateSOC(avgVoltage);
    float remainingCapacity = (batteryCapacity * soc) / 100.0;
    return remainingCapacity / avgCurrent;
}

// Gesamtenergie
float Battery::getTotalEnergy() const {
    return totalEnergy;
}

// Geglättete Spannung
float Battery::getSmoothedVoltage() {
    float sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        sum += voltageSamples[i];
    }
    return sum / SAMPLE_SIZE;
}

// Geglätteter Strom
float Battery::getSmoothedCurrent() {
    float sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        sum += currentSamples[i];
    }
    return sum / SAMPLE_SIZE;
}

// **Hier wird der Task erzeugt**
void Battery::startUpdateTask(uint32_t intervalMs, UBaseType_t priority, BaseType_t coreID) {
    if (taskHandle == nullptr) {
        // Merke dir das Intervall
        taskDelayMs = intervalMs;
        
        // Erstelle den Task, übergebe this als Parameter
        xTaskCreatePinnedToCore(
            updateTask,
            "BatteryUpdate",
            4096,
            this, 
            priority,
            &taskHandle,
            coreID
        );
    }
}

// **Statische Task-Funktion** (FreeRTOS braucht eine C-Style-Funktion)
void Battery::updateTask(void *param) {
    Battery *self = static_cast<Battery*>(param);

    for (;;) {
        float rawVoltage = ina2.getBusVoltage_V();  
        float rawCurrent = ina2.getCurrent_mA(); // z.B. Entladung, wird gleich positiv gemacht
        float positiveCurrent = fabs(rawCurrent);
        //float energy_mWh = ina2.getBusPower_mW();  

        float energy_mWh = ina2.getBusPower_mW() * (self->taskDelayMs / 3600000.0); // ms → Stunden

        // Update-Aufruf
        self->update(rawVoltage, positiveCurrent, energy_mWh);

        // Warte das angegebene Intervall ab
        vTaskDelay(self->taskDelayMs / portTICK_PERIOD_MS);
    }
}
