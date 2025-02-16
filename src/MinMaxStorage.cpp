#include "MinMaxStorage.h"

float minTemp = NAN; // Initialisiere globale Variablen
float maxTemp = NAN;
SDCardLogger* logger = nullptr;  // Initialisiere den Logger-Pointer

void initMinMaxStorage(SDCardLogger* sdLogger) {
    logger = sdLogger;
}

void saveMinMaxToJson() {
    // JSON-Objekt erstellen
    JsonDocument jsonDoc;
    jsonDoc["minTemp"] = minTemp;
    jsonDoc["maxTemp"] = maxTemp;

    // JSON in Datei schreiben
    File file = SD.open("/min_max.json", FILE_WRITE);
    if (!file) {
        if (logger) logger->logError("Fehler beim Öffnen der min_max.json zum Schreiben!");
        return;
    }

    if (serializeJson(jsonDoc, file) == 0) {
        if (logger) logger->logError("Fehler beim Schreiben in die min_max.json!");
    } else {
        if (logger) logger->logInfo("Min/Max-Temperaturen gespeichert.");
    }

    file.close();
}

void resetMinMaxValues() {
    minTemp = NAN;
    maxTemp = NAN;

    saveMinMaxToJson();
    if (logger) logger->logInfo("Min/Max-Werte zurückgesetzt.");
}

void loadMinMaxFromJson() {
    // Datei öffnen
    File file = SD.open("/min_max.json", FILE_READ);
    if (!file) {
        if (logger) logger->logWarning("min_max.json nicht gefunden, starte mit neuen Werten.");
        minTemp = NAN;
        maxTemp = NAN;
        return;
    }

    // JSON-Dokument erstellen
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, file);
    if (error) {
        if (logger) logger->logError("Fehler beim Lesen der min_max.json: " + String(error.c_str()));
        file.close();
        return;
    }

    // Werte aus JSON lesen
    minTemp = jsonDoc["minTemp"] | NAN;
    maxTemp = jsonDoc["maxTemp"] | NAN;

    if (logger) {
        logger->logInfo("Geladene Min-Temp: " + String(minTemp));
        logger->logInfo("Geladene Max-Temp: " + String(maxTemp));
    }

    file.close();
}
