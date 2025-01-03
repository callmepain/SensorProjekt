#ifndef WEBSERVERHANDLER_H
#define WEBSERVERHANDLER_H

#include <WebServer.h>
#include "SensorHandler.h"
#include <FS.h>    // Für das generische Dateisystem
#include <SPIFFS.h> // Für SPIFFS-spezifische Funktionen

class WebServerHandler {
public:
    WebServerHandler(SensorHandler& sensorHandler);

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
    bool saveToSPIFFS(const String& fileName, const String& content);
    String loadFromSPIFFS(const String& fileName);

    // HTML-Seiten
    void handleUploadPage();
    void handleEditPage();

private:
    WebServer server;
    SensorHandler& sensorHandler;

    std::vector<float> voltageSamples;

    String currentUploadName;
    bool displayOn = true;
    
    int updateVoltageSamplesAndCalculateSoC(float newVoltage);
    int calculateBatteryPercentageWithSamples(const std::vector<float>& voltageSamples);
    float calculateRemainingTime(float voltage, float current_mA, int batteryCapacity_mAh);
    void streamFileFromSPIFFS(const String& path, const char* contentType);

    // Hilfsfunktionen
    File openFile(String path, const char* mode);
    void sendError(int code, String message);
};

#endif // WEBSERVERHANDLER_H
