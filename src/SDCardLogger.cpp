#include "SDCardLogger.h"
#include <time.h>

SDCardLogger::SDCardLogger(const char* filename, uint8_t chipSelectPin) 
    : logFile(filename), csPin(chipSelectPin), initialized(false) {
}

bool SDCardLogger::begin() {
    if (!SD.begin(csPin)) {
        log("[ERROR] SD-Karte konnte nicht initialisiert werden");
        return false;
    }
    initialized = true;
    return true;
}

void SDCardLogger::log(const String& message, bool addTimestamp) {
    if (!initialized) return;

    File file = SD.open(logFile, FILE_APPEND);
    if (!file) {
        // Hier können wir nicht loggen, da das Logging selbst fehlgeschlagen ist
        return;
    }

    if (addTimestamp) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char timeString[25];
            strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
            file.print(timeString);
            file.print(" - ");
        }
    }

    file.println(message);
    file.close();

    // Prüfen ob Rotation notwendig ist
    if (getLogSize() >= MAX_FILE_SIZE) {
        rotateLogFiles();
    }
}

void SDCardLogger::logError(const String& message) {
    log("[ERROR] " + message);
}

void SDCardLogger::logWarning(const String& message) {
    log("[WARN] " + message);
}

void SDCardLogger::logInfo(const String& message) {
    log("[INFO] " + message);
}

void SDCardLogger::clearLog() {
    if (!initialized) return;
    SD.remove(logFile);
}

bool SDCardLogger::rotateLogFiles() {
    if (!initialized) return false;

    // Lösche älteste Log-Datei
    String oldestLog = String(logFile) + "." + String(MAX_LOG_FILES - 1);
    if (SD.exists(oldestLog.c_str())) {
        SD.remove(oldestLog.c_str());
    }

    // Verschiebe bestehende Log-Dateien
    for (int i = MAX_LOG_FILES - 2; i >= 0; i--) {
        String currentLog = String(logFile) + (i > 0 ? "." + String(i) : "");
        String nextLog = String(logFile) + "." + String(i + 1);
        
        if (SD.exists(currentLog.c_str())) {
            SD.rename(currentLog.c_str(), nextLog.c_str());
        }
    }

    return true;
}

unsigned long SDCardLogger::getLogSize() {
    if (!initialized) return 0;

    File file = SD.open(logFile);
    if (!file) return 0;
    
    unsigned long size = file.size();
    file.close();
    return size;
}

bool SDCardLogger::exportLogs(const char* exportFilename) {
    if (!initialized) return false;

    File exportFile = SD.open(exportFilename, FILE_WRITE);
    if (!exportFile) return false;

    // Exportiere alle Log-Dateien
    for (int i = MAX_LOG_FILES - 1; i >= 0; i--) {
        String logName = String(logFile) + (i > 0 ? "." + String(i) : "");
        if (SD.exists(logName.c_str())) {
            File currentLog = SD.open(logName.c_str());
            if (currentLog) {
                while (currentLog.available()) {
                    exportFile.write(currentLog.read());
                }
                currentLog.close();
            }
        }
    }

    exportFile.close();
    return true;
} 