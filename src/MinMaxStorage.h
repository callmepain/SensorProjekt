#ifndef MINMAXSTORAGE_H
#define MINMAXSTORAGE_H

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

// Globale Variablen, die du verwenden möchtest
extern float minTemp;
extern float maxTemp;

// Funktionsprototypen
void saveMinMaxToJson();
void resetMinMaxValues();
void loadMinMaxFromJson();

#endif // MINMAXSTORAGE_H
