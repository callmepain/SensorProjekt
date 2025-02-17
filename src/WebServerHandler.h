#ifndef WEBSERVERHANDLER_H
#define WEBSERVERHANDLER_H

#include <WebServer.h>
#include "SensorHandler.h"
#include <FS.h>    // Für das generische Dateisystem
#include <SPIFFS.h> // Für SPIFFS-spezifische Funktionen
#include "SDCardLogger.h"

class WebServerHandler {
public:
    WebServerHandler(SensorHandler& sensorHandler, SDCardLogger& logger);

    void init();
    void handleClient();

    // API-Endpunkte
    void handleFileListAPI();
    void handleStorageInfoAPI();
    void handleFileContentAPI();
    void handleSensorAPI();
    void handleSaveFile();

    // Dateiverwaltung
    void handleDeleteFile();
    void handleDownloadFile();
    void handleUpload();
    void handleFileUpload();
    bool saveToSD(const String& fileName, const String& content);
    String loadFromSD(const String& fileName);

    // HTML-Seiten
    void handleUploadPage();
    void handleEditPage();

private:
    WebServer server;
    SensorHandler& sensorHandler;
    SDCardLogger& sdLogger;

    std::vector<float> voltageSamples;

    String currentUploadName;
    bool displayOn = true;
    
    int updateVoltageSamplesAndCalculateSoC(float newVoltage);
    int calculateBatteryPercentageWithSamples(const std::vector<float>& voltageSamples);
    float calculateRemainingTime(float voltage, float current_mA, int batteryCapacity_mAh);
    String getContentType(const String& path);
    bool streamFileFromSD(const String& path, const char* contentType);
    void listDirectory(File dir, String path, String& json, bool& firstEntry);

    // Hilfsfunktionen
    void sendError(int code, String message);
    String escapeJsonString(const String& input);
    void createDirectoryPath(fs::FS &fs, String path);
};

#endif // WEBSERVERHANDLER_H
