#include "Battery.h"
#include "INA219Manager.h"
#include "SDCardLogger.h"

extern INA219Manager ina2;


// Konstruktor
Battery::Battery(int capacity, const char* path, SDCardLogger& logger, float resistance) 
    : batteryCapacity(capacity), filePath(path), sampleIndex(0), sdLogger(logger), internalResistance(resistance)
{
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        voltageSamples[i] = 0;
        currentSamples[i] = 0;
    }
}

void Battery::initialize() {
    // JSON-Daten laden, nachdem das Dateisystem initialisiert wurde
    loadFromJson();
    sdLogger.logInfo("Battery erfolgreich initialisiert.");
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
        sdLogger.logError("Fehler beim Öffnen der Datei zum Schreiben");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        sdLogger.logError("Fehler beim Schreiben der JSON-Daten");
    }
    file.close();
}

// JSON-Datei laden
void Battery::loadFromJson() {
    File file = SPIFFS.open(filePath, FILE_READ);
    if (!file) {
        sdLogger.logError("Fehler beim Öffnen der Datei zum Lesen");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        sdLogger.logError("Fehler beim Lesen der JSON-Daten: " + String(error.c_str()));
        return;
    }

    if (doc["totalEnergy"].is<float>()) {
        totalEnergy = doc["totalEnergy"].as<float>();
    } else {
        totalEnergy = 0; // Standardwert
    }

    file.close();
    sdLogger.logInfo("Energie Daten erfolgreich geladen.");
}

// Ladezeit berechnen
float Battery::calculateChargingTime(float current_mA) {
    // Durchschnittliche Batteriespannung ermitteln
    float avgVoltage = getSmoothedVoltage();
    
    // Aktuellen Ladezustand (SOC) berechnen
    int soc = calculateSOC(avgVoltage);
    
    // Berechnung der verbleibenden Kapazität in mAh
    float remainingCapacity_mAh = batteryCapacity * (100 - soc) / 100.0;
    
    // Überprüfen, ob überhaupt ein Ladestrom vorhanden ist
    if (current_mA == 0) {
        return -1;  // kein Ladestrom -> Ladezeit nicht berechenbar
    }
    
    // Ladestrom absolut betrachten (in mA)
    float positiveCurrent_mA = fabs(current_mA);
    
    // Dynamische Effizienz: Im Ladebereich (SOC < 50) geht man oft von einer relativ hohen Effizienz aus,
    // während im oberen Bereich (z.B. CV-Phase) mehr Verluste entstehen.
    float efficiency = (soc < 50) ? 0.95 : 0.80;
    
    // Zusätzlich kann in der Endphase ein fester Zeitoffset berücksichtigt werden,
    // um z.B. die durch das CV-Ladeverhalten entstehende verlängerte Ladezeit abzubilden.
    float baselineTimeOffset = 0.5; // in Stunden, anpassbar je nach Batteriecharakteristik
    
    // Berechnung der Ladezeit in Stunden
    float chargingTime = (remainingCapacity_mAh / positiveCurrent_mA) / efficiency + baselineTimeOffset;
    
    return chargingTime;  
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
    // Spannung unter Last korrigieren
    float current = getCurrentSigned(); // in mA
    float voltageDropIR = (current / 1000.0f) * internalResistance;
    float openCircuitVoltage = voltage + voltageDropIR;
    
    // Verbesserte SOC-Berechnung für LiPo Akkus
    if (openCircuitVoltage >= 4.2) return 100;
    if (openCircuitVoltage <= 3.1) return 0;  // Neue untere Grenze
    
    // Nicht-lineare Interpolation für genauere Werte
    if (openCircuitVoltage >= 3.9) {
        // 70-100% Bereich
        return 70 + (openCircuitVoltage - 3.9) * 100;
    } else if (openCircuitVoltage >= 3.7) {
        // 30-70% Bereich
        return 30 + (openCircuitVoltage - 3.7) * 200;
    } else {
        // 0-30% Bereich: Linear von 3.1V bis 3.7V
        return (openCircuitVoltage - 3.1) * 50;  // Angepasst für neue Grenze
    }
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

    // Korrigiere die Spannung um den Spannungsabfall am Innenwiderstand
    float voltageDropIR = (avgCurrent / 1000.0f) * internalResistance;
    float openCircuitVoltage = avgVoltage + voltageDropIR;

    if (avgCurrent < 1.0) {
        return -1; // zu niedriger Verbrauch
    }

    int soc = calculateSOC(openCircuitVoltage);

    // Batterie-Entladung nicht unter 20 % zulassen
    if (soc <= 20) {
        return 0; // Batterie ist auf Minimum
    }

    // Berechne Effizienz basierend auf Strom und Spannung
    float efficiency = 0.95; // Basis-Effizienz
    if (avgCurrent > 500) {
        // Bei höherem Strom sinkt die Effizienz
        efficiency = 0.95 - ((avgCurrent - 500) / 1500) * 0.1;
    }
    
    // Berechnung der verbleibenden Kapazität ab 20 % SOC
    float usableCapacity = (batteryCapacity * (soc - 20)) / 100.0;
    
    // Zeit in Stunden = (Kapazität in mAh * Effizienz) / (Strom in mA)
    float remainingTime = (usableCapacity * efficiency) / avgCurrent;
    
    // Berücksichtige zusätzliche Verluste bei niedrigem SOC
    if (soc < 50) {
        remainingTime *= (0.8 + (soc / 50.0) * 0.2);
    }
    
    return remainingTime;
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

// Aktuellen Strom abrufen (ohne Glättung)
float Battery::getCurrent() {
    return ina2.getCurrent_mA();
}

// Optional: Aktuellen Strom mit Vorzeichen
// Positiv = Entladung, Negativ = Ladung
float Battery::getCurrentSigned() {
    return -ina2.getCurrent_mA(); // Vorzeichen umkehren für intuitivere Darstellung
}

// Aktuelle Leistung in mW
float Battery::getPower() {
    return ina2.getBusPower_mW();
}
