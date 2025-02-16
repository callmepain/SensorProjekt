#ifndef MINMAXSTORAGE_H
#define MINMAXSTORAGE_H

#include <ArduinoJson.h>
#include <SD.h>
#include "SDCardLogger.h"

// Globale Variablen, die du verwenden möchtest
extern float minTemp;
extern float maxTemp;
extern SDCardLogger* logger;  // Pointer auf den Logger

// Funktionsprototypen
void saveMinMaxToJson();
void resetMinMaxValues();
void loadMinMaxFromJson();
void initMinMaxStorage(SDCardLogger* sdLogger);  // Neue Initialisierungsfunktion

#endif // MINMAXSTORAGE_H
