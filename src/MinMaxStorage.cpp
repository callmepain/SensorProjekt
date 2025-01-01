#include "MinMaxStorage.h"

float minTemp = NAN; // Initialisiere globale Variablen
float maxTemp = NAN;

void saveMinMaxToJson() {
    // JSON-Objekt erstellen
    JsonDocument jsonDoc;
    jsonDoc["minTemp"] = minTemp;
    jsonDoc["maxTemp"] = maxTemp;

    // JSON in Datei schreiben
    File file = SPIFFS.open("/min_max.json", FILE_WRITE);
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Schreiben!");
        return;
    }

    if (serializeJson(jsonDoc, file) == 0) {
        Serial.println("Fehler beim Schreiben in die JSON-Datei!");
    } else {
        Serial.println("Min/Max-Temperaturen gespeichert.");
    }

    file.close();
}

void resetMinMaxValues() {
    minTemp = NAN;
    maxTemp = NAN;

    saveMinMaxToJson();
}

void loadMinMaxFromJson() {
    // Datei öffnen
    File file = SPIFFS.open("/min_max.json", FILE_READ);
    if (!file) {
        Serial.println("JSON-Datei nicht gefunden, starte mit neuen Werten.");
        minTemp = NAN;
        maxTemp = NAN;
        return;
    }

    // JSON-Dokument erstellen
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, file);
    if (error) {
        Serial.println("Fehler beim Lesen der JSON-Datei.");
        file.close();
        return;
    }

    // Werte aus JSON lesen
    minTemp = jsonDoc["minTemp"] | NAN;
    maxTemp = jsonDoc["maxTemp"] | NAN;

    Serial.print("Geladene Min-Temp: ");
    Serial.println(minTemp);
    Serial.print("Geladene Max-Temp: ");
    Serial.println(maxTemp);

    file.close();
}
