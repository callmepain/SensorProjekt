#pragma once

#include <SD.h>
#include <Arduino.h>

class SDCardLogger {
private:
    const char* logFile;
    uint8_t csPin;
    bool initialized;
    
    // Maximale Anzahl von Log-Dateien für Rotation
    static const int MAX_LOG_FILES = 5;
    // Maximale Größe einer Log-Datei in Bytes (z.B. 1MB)
    static const unsigned long MAX_FILE_SIZE = 1024 * 1024;

public:
    SDCardLogger(const char* filename = "/system.log", uint8_t chipSelectPin = 5);
    
    bool begin();
    void log(const String& message, bool addTimestamp = true);
    void logError(const String& message);
    void logWarning(const String& message);
    void logInfo(const String& message);
    void clearLog();
    
    // Neue Funktionen für erweitertes Logging
    bool rotateLogFiles();
    unsigned long getLogSize();
    void setMaxFileSize(unsigned long size);
    bool exportLogs(const char* exportFilename);
}; 