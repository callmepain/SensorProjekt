#include "WebServerHandler.h"
#include "MinMaxStorage.h"
#include "INA219Manager.h"
#include "GY302Manager.h"
#include "VL53L1XManager.h"
#include <Update.h>
#include <vector>
#include <numeric> // Für std::accumulate
#include "Battery.h"
#include <Adafruit_SSD1306.h>
#include "DisplayConfig.h"
#include <SD.h>

extern Adafruit_SSD1306 display;
extern Battery battery; // Deklaration, keine Definition

WebServerHandler::WebServerHandler(SensorHandler& sensorHandler)
    : server(80), sensorHandler(sensorHandler) {}

String WebServerHandler::getContentType(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".js")) return "application/javascript";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".ico")) return "image/x-icon";
    if (path.endsWith(".svg")) return "image/svg+xml";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".txt")) return "text/plain";
    return "application/octet-stream"; // Fallback für unbekannte Typen
}

bool WebServerHandler::streamFileFromSPIFFS(const String& path, const char* contentType) {
    File file = openFile(path, "r");
    if (!file) {
        return false; // Datei nicht gefunden
    }
    server.streamFile(file, contentType);
    file.close();
    return true; // Erfolgreich gestreamt
}

bool WebServerHandler::streamFileFromSD(const String& path, const char* contentType) {
    // Öffne die Datei auf der SD-Karte
    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }

    // Stream die Datei an den Client
    server.streamFile(file, contentType);
    file.close();
    return true;
}

void WebServerHandler::init() {
    server.onNotFound([this]() {
        String path = server.uri(); // Der angeforderte Dateipfad
        if (path.endsWith("/")) {
            path += "index.html"; // Standard-Datei für Verzeichnisse
        }

        // Bestimme den Content-Type basierend auf der Dateierweiterung
        String contentType = getContentType(path);

        // Versuche, die Datei von der SD-Karte zu streamen
        if (this->streamFileFromSD(path, contentType.c_str())) {
            return; // Erfolgreich von SD gestreamt
        }

        // Versuche, die Datei aus SPIFFS zu streamen
        if (this->streamFileFromSPIFFS(path, contentType.c_str())) {
            return; // Erfolgreich aus SPIFFS gestreamt
        }

        // Wenn die Datei weder in SPIFFS noch auf der SD-Karte gefunden wurde
        this->sendError(404, "Datei nicht gefunden: " + path);
    });

    server.on("/api/display", HTTP_POST, [this]() {
        if (!server.hasArg("state")) {
            server.send(400, "application/json", "{\"error\":\"State parameter missing\"}");
            return;
        }

        String state = server.arg("state");
        if (state == "on") {
            displayOn = true;
            // Display einschalten (Beispiel für SSD1306)
            setDisplayState(true);
            setDisplayState(true);
        } else if (state == "off") {
            displayOn = false;
            // Display ausschalten (Beispiel für SSD1306)
            setDisplayState(false);
            setDisplayState(false);
        } else {
            server.send(400, "application/json", "{\"error\":\"Invalid state\"}");
            return;
        }

        server.send(200, "application/json", "{\"message\":\"Display state updated\"}");
    });

    server.on("/ota", HTTP_POST, [this]() {
        if (Update.hasError()) {
            server.send(200, "text/plain", "FAIL");
        } else {
            // Statusseite mit Redirect nach dem Neustart senden
            server.send(200, "text/html",
                "<!DOCTYPE html>"
                "<html lang='de'>"
                "<head>"
                "<meta charset='UTF-8'>"
                "<meta http-equiv='refresh' content='15; url=/index.html'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "<title>OTA Update</title>"
                "<style>"
                "body {"
                "    overflow-y: scroll;"
                "    font-family: Arial, sans-serif;"
                "    margin: 0;"
                "    background-color: #121212;"
                "    color: #E0E0E0;"
                "    display: flex;"
                "    flex-direction: column;"
                "    align-items: center;"
                "    justify-content: center;"
                "    height: 100vh;"
                "}"
                "h1 {"
                "    color: #FFFFFF;"
                "    text-align: center;"
                "    margin-bottom: 20px;"
                "    font-size: 2rem;"
                "    background: linear-gradient(90deg, #FF00FF, #8800FF);"
                "    -webkit-background-clip: text;"
                "    color: transparent;"
                "}"
                "p {"
                "    text-align: center;"
                "    font-size: 1.2rem;"
                "    line-height: 1.5;"
                "    margin-bottom: 20px;"
                "}"
                ".progress-container {"
                "    width: 80%;"
                "    max-width: 400px;"
                "    height: 20px;"
                "    background-color: #1E1E1E;"
                "    border: 2px solid #FF00FF;"
                "    border-radius: 10px;"
                "    overflow: hidden;"
                "    position: relative;"
                "}"
                ".progress-bar {"
                "    height: 100%;"
                "    width: 0%;"
                "    background: linear-gradient(90deg, #FF00FF, #8800FF);"
                "    border-radius: 10px;"
                "}"
                "::-webkit-scrollbar {"
                "   width: 10px;"
                "}"

                "::-webkit-scrollbar-track {"
                "    background: #1E1E1E;"
                "    border-radius: 10px;"
                "}"

                "::-webkit-scrollbar-thumb {"
                "    background: linear-gradient(90deg, #FF00FF, #8800FF);"
                "    border-radius: 10px; "
                "    box-shadow: inset 0 0 5px rgba(0, 0, 0, 0.5);"
                "}"

                "::-webkit-scrollbar-thumb:hover {"
                "    background: linear-gradient(90deg, #8800FF, #FF00FF); "
                "}"
                "</style>"
                "</head>"
                "<body>"
                "<h1>Update abgeschlossen!</h1>"
                "<p>Das Gerät wird neu gestartet. Bitte warten Sie einige Sekunden...</p>"
                "<div class='progress-container'>"
                "    <div class='progress-bar' id='progressBar'></div>"
                "</div>"
                "<script>"
                "    document.addEventListener('DOMContentLoaded', () => {"
                "        const progressBar = document.getElementById('progressBar');"
                "        let progress = 0;"
                "        const interval = setInterval(() => {"
                "            progress += 1;"
                "            progressBar.style.width = progress + '%';"
                "            if (progress >= 100) clearInterval(interval);"
                "        }, 150);"
                "    });"
                "</script>"
                "</body>"
                "</html>");
            delay(1000); // Kurze Verzögerung, um sicherzustellen, dass die Antwort gesendet wird
            ESP.restart();
        }
    }, [this]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("OTA Update gestartet: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Platz für Firmware reservieren
                Serial.println("Update.begin fehlgeschlagen!");
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Serial.println("Update.write fehlgeschlagen!");
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (!Update.end(true)) {
                Serial.println("Update.end fehlgeschlagen!");
                Update.printError(Serial);
            } else {
                Serial.println("OTA Update erfolgreich abgeschlossen!");
            }
        } else {
            Serial.println("OTA Fehler aufgetreten.");
        }
    });

    // Setup API endpoint
    server.on("/api/sensordaten", [this]() {
        this->handleSensorAPI();
    });

    // Reset-Min/Max-Endpunkt
    server.on("/api/reset", HTTP_POST, [this]() {
        resetMinMaxValues(); // Funktion zum Zurücksetzen der Min/Max-Werte
        this->server.send(200, "application/json", String("{\"message\":\"Min/Max zurückgesetzt\"}"));
    });

    // Neue Routen für SPIFFS-Dateien
    server.on("/api/files", HTTP_GET, [this]() {
        this->handleFileListAPI();
    });

    server.on("/api/storage", HTTP_GET, [this]() {
        this->handleStorageInfoAPI();
    });

    server.on("/api/file", HTTP_GET, [this]() {
        this->handleFileContentAPI();
    });

    server.on("/download", HTTP_GET, [this]() {
        this->handleDownloadFile();
    });

    server.on("/upload", HTTP_POST, [this]() {
        this->handleUpload();
    }, [this]() {
        this->handleFileUpload();
    });

    server.on("/delete", HTTP_DELETE, [this]() {
        this->handleDeleteFile();
    });

    server.on("/edit", HTTP_GET, [this]() {
        this->streamFileFromSPIFFS("/edit.html", "text/html");
    });

    server.on("/save", HTTP_POST, [this]() {
        this->handleSaveFile();
    });

    server.on("/api/saveConfig", HTTP_POST, [this]() {
        if (!server.hasArg("plain")) { // JSON-Daten werden im Body unter "plain" gesendet
            server.send(400, "application/json", "{\"error\":\"Missing JSON body\"}");
            return;
        }

        // JSON-Daten aus dem Body parsen
        String body = server.arg("plain");
        JsonDocument jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, body);

        if (error) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
            return;
        }

        // Optional: Debugging - empfangene Daten
        Serial.println("Empfangene JSON-Daten: " + body);

        // Konfiguration speichern
        String fileName = "/config2.json";
        if (saveToSPIFFS(fileName, body)) {
            server.send(200, "application/json", "{\"message\":\"Configuration saved successfully\"}");
        } else {
            server.send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
        }
    });

    server.on("/api/loadConfig", HTTP_GET, [this]() {
        String fileName = "/config2.json"; // Standarddateiname für die Konfiguration
        String content = loadFromSPIFFS(fileName);

        if (content != "") {
            server.send(200, "application/json", content);
        } else {
            server.send(404, "application/json", "{\"error\":\"Configuration file not found\"}");
        }
    });

    server.begin();
    Serial.println("Webserver gestartet!");
}

void WebServerHandler::sendError(int code, String message) {
    server.send(code, "text/plain", message);
    Serial.println("Fehler: " + message);
}

File WebServerHandler::openFile(String path, const char* mode) {
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    File file = SPIFFS.open(path, mode);
    if (!file) {
        Serial.println("Fehler: Datei nicht gefunden: " + path);
    }
    return file;
}

void WebServerHandler::handleClient() {
    server.handleClient();
}

void WebServerHandler::handleFileListAPI() {
    String json = "[";

    // Dateien aus SPIFFS auflisten
    File rootSPIFFS = SPIFFS.open("/");
    if (rootSPIFFS) {
        File file = rootSPIFFS.openNextFile();
        while (file) {
            if (json.length() > 1) json += ",";
            json += "{\"name\":\"SPIFFS:" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
            file = rootSPIFFS.openNextFile();
        }
    } else {
        Serial.println("Fehler: SPIFFS Root-Verzeichnis konnte nicht geöffnet werden.");
    }

    // Dateien aus der SD-Karte auflisten
    if (SD.begin(5)) { // Beispiel: CS-Pin 5
        File rootSD = SD.open("/");
        if (rootSD) {
            File file = rootSD.openNextFile();
            while (file) {
                if (json.length() > 1) json += ",";
                json += "{\"name\":\"SD:" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
                file = rootSD.openNextFile();
            }
        } else {
            Serial.println("Fehler: SD-Karte Root-Verzeichnis konnte nicht geöffnet werden.");
        }
    } else {
        Serial.println("Fehler: SD-Karte konnte nicht initialisiert werden.");
    }

    json += "]";
    server.send(200, "application/json", json);
}

void WebServerHandler::handleStorageInfoAPI() {
    String json = "{";

    // Speicherinformationen für SPIFFS
    size_t spiffsTotal = SPIFFS.totalBytes();
    size_t spiffsUsed = SPIFFS.usedBytes();
    size_t spiffsFree = spiffsTotal - spiffsUsed;

    json += "\"SPIFFS\":{";
    json += "\"total\":" + String(spiffsTotal) + ",";
    json += "\"used\":" + String(spiffsUsed) + ",";
    json += "\"free\":" + String(spiffsFree) + "},";
    
    // Speicherinformationen für SD-Karte
    if (SD.begin(5)) { // Beispiel: CS-Pin 5
        uint64_t sdTotal = SD.cardSize(); // Gesamtkapazität der SD-Karte in Bytes
        uint64_t sdUsed = sdTotal - SD.totalBytes(); // Genutzter Speicher
        uint64_t sdFree = sdTotal - sdUsed;

        json += "\"SD\":{";
        json += "\"total\":" + String(sdTotal) + ",";
        json += "\"used\":" + String(sdUsed) + ",";
        json += "\"free\":" + String(sdFree) + "}";
    } else {
        json += "\"SD\":{\"error\":\"SD card not initialized\"}";
    }

    json += "}";

    server.send(200, "application/json", json);
}

void WebServerHandler::handleFileContentAPI() {
    if (!server.hasArg("name")) {
        server.send(400, "text/plain", "Fehlender Dateiname");
        return;
    }

    String fileName = server.arg("name");
    File file;

    // Prüfen, welches Dateisystem basierend auf dem Präfix verwendet werden soll
    if (fileName.startsWith("SD:")) {
        fileName = fileName.substring(3); // Entferne 'SD:'-Präfix
        fileName = "/" + fileName; // Füge "/" für Root-Dateien hinzu
        if (SD.begin(5)) { // Beispiel: CS-Pin 5
            file = SD.open(fileName, "r");
        } else {
            server.send(500, "text/plain", "SD-Karte konnte nicht initialisiert werden.");
            return;
        }
    } else if (fileName.startsWith("SPIFFS:")) {
        fileName = fileName.substring(7); // Entferne 'SPIFFS:'-Präfix
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        file = SPIFFS.open(fileName, "r");
    } else {
        server.send(400, "text/plain", "Ungültiger Präfix. Unterstützt: SD:, SPIFFS:");
        return;
    }

    if (!file) {
        server.send(404, "text/plain", "Datei nicht gefunden: " + fileName);
        return;
    }

    String content = file.readString();
    file.close();
    server.send(200, "text/plain", content);
}

void WebServerHandler::handleSaveFile() {
    if (!server.hasArg("name") || !server.hasArg("content")) {
        server.send(400, "text/plain", "Fehlender Dateiname oder Inhalt");
        return;
    }

    String fileName = server.arg("name");
    String fileContent = server.arg("content");
    File file;

    // Prüfen, welches Dateisystem basierend auf dem Präfix verwendet werden soll
    if (fileName.startsWith("SD:")) {
        fileName = fileName.substring(3); // Entferne 'SD:'-Präfix
        if (SD.begin(5)) { // Beispiel: CS-Pin 5
            file = SD.open(fileName, "w");
        } else {
            server.send(500, "text/plain", "SD-Karte konnte nicht initialisiert werden.");
            return;
        }
    } else if (fileName.startsWith("SPIFFS:")) {
        fileName = fileName.substring(7); // Entferne 'SPIFFS:'-Präfix
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        file = SPIFFS.open(fileName, "w");
    } else {
        server.send(400, "text/plain", "Ungültiger Präfix. Unterstützt: SD:, SPIFFS:");
        return;
    }

    if (!file) {
        server.send(500, "text/plain", "Fehler beim Öffnen der Datei zum Schreiben: " + fileName);
        return;
    }

    file.print(fileContent);
    file.close();

    server.sendHeader("Location", "/files.html?saved=true");
    server.send(303);
}

void WebServerHandler::handleDeleteFile() {
    if (!server.hasArg("name")) {
        server.send(400, "text/plain", "Fehlender Dateiname");
        return;
    }
    String fileName = server.arg("name");

    // Prüfen, ob der Dateiname mit '/' beginnt
    if (!fileName.startsWith("/")) {
        fileName = "/" + fileName;
    }

    bool fileDeleted = false;

    // Versuche, die Datei auf der SD-Karte zu löschen
    if (SD.begin(5)) { // Beispiel: CS-Pin 5
        if (SD.exists(fileName)) {
            fileDeleted = SD.remove(fileName);
            if (fileDeleted) {
                Serial.println("Datei von der SD-Karte gelöscht: " + fileName);
            } else {
                Serial.println("Fehler beim Löschen der Datei von der SD-Karte: " + fileName);
            }
        }
    } else {
        Serial.println("SD-Karte konnte nicht initialisiert werden.");
    }

    // Wenn die Datei nicht auf der SD-Karte gelöscht wurde, versuche SPIFFS
    if (!fileDeleted) {
        if (SPIFFS.exists(fileName)) {
            fileDeleted = SPIFFS.remove(fileName);
            if (fileDeleted) {
                Serial.println("Datei aus SPIFFS gelöscht: " + fileName);
            } else {
                Serial.println("Fehler beim Löschen der Datei aus SPIFFS: " + fileName);
            }
        }
    }

    // Sende die Antwort basierend auf dem Ergebnis
    if (fileDeleted) {
        server.send(200, "text/plain", "Datei gelöscht: " + fileName);
    } else {
        server.send(404, "text/plain", "Datei nicht gefunden oder konnte nicht gelöscht werden: " + fileName);
    }
}

void WebServerHandler::handleDownloadFile() {
    if (!server.hasArg("name")) {
        server.send(400, "text/plain", "Fehlender Dateiname");
        return;
    }
    String fileName = server.arg("name");

    // Prüfen, ob der Dateiname mit '/' beginnt
    if (!fileName.startsWith("/")) {
        fileName = "/" + fileName;
    }

    File file;

    // Versuche, die Datei zuerst auf der SD-Karte zu öffnen
    if (SD.begin(5)) { // Beispiel: CS-Pin 5
        file = SD.open(fileName, "r");
        if (!file) {
            Serial.println("Datei nicht auf der SD-Karte gefunden: " + fileName);
        }
    } else {
        Serial.println("SD-Karte konnte nicht initialisiert werden.");
    }

    // Fallback: Wenn die Datei nicht auf der SD-Karte ist, SPIFFS verwenden
    if (!file) {
        file = SPIFFS.open(fileName, "r");
        if (!file) {
            server.send(404, "text/plain", "Datei nicht gefunden: " + fileName);
            Serial.println("Datei nicht gefunden: " + fileName);
            return;
        }
        Serial.println("Datei aus SPIFFS geöffnet: " + fileName);
    } else {
        Serial.println("Datei von der SD-Karte geöffnet: " + fileName);
    }

    // Content-Disposition setzen, um den Dateinamen an den Browser zu senden
    server.sendHeader("Content-Disposition", "attachment; filename=" + fileName.substring(1));
    server.streamFile(file, "application/octet-stream");
    file.close();
}

/*void WebServerHandler::handleFileUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        currentUploadName = upload.filename;
        if (currentUploadName.isEmpty()) {
            Serial.println("Kein Dateiname angegeben, Upload abgebrochen.");
            sendError(400, "Kein Dateiname angegeben");
            return;
        }
        if (!currentUploadName.startsWith("/")) {
            currentUploadName = "/" + currentUploadName;
        }
        Serial.println("Upload gestartet: " + currentUploadName);

        File file = SPIFFS.open(currentUploadName, "w");
        if (!file) {
            Serial.println("Fehler beim Öffnen der Datei zum Schreiben!");
            sendError(500, "Fehler beim Öffnen der Datei");
            return;
        }
        file.close();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        File file = SPIFFS.open(currentUploadName, "a");
        if (file) {
            file.write(upload.buf, upload.currentSize);
            file.close();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.println("Upload beendet: " + currentUploadName + " (" + String(upload.totalSize) + " Bytes)");
        if (upload.totalSize == 0) {
            SPIFFS.remove(currentUploadName); // Entferne leere Dateien
            Serial.println("Leere Datei entfernt: " + currentUploadName);
        }
    } else {
        server.send(500, "text/plain", "Fehler beim Hochladen!");
    }
}*/

void WebServerHandler::handleFileUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        currentUploadName = upload.filename;
        if (currentUploadName.isEmpty()) {
            Serial.println("Kein Dateiname angegeben, Upload abgebrochen.");
            sendError(400, "Kein Dateiname angegeben");
            return;
        }
        if (!currentUploadName.startsWith("/")) {
            currentUploadName = "/" + currentUploadName;
        }
        Serial.println("Upload gestartet: " + currentUploadName);

        // Öffne die Datei auf der SD-Karte
        File file = SD.open(currentUploadName, FILE_WRITE);
        if (!file) {
            Serial.println("Fehler beim Öffnen der Datei auf der SD-Karte zum Schreiben!");
            sendError(500, "Fehler beim Öffnen der Datei");
            return;
        }
        file.close();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Schreibe Daten in die Datei auf der SD-Karte
        File file = SD.open(currentUploadName, FILE_APPEND);
        if (file) {
            file.write(upload.buf, upload.currentSize);
            file.close();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.println("Upload beendet: " + currentUploadName + " (" + String(upload.totalSize) + " Bytes)");
        if (upload.totalSize == 0) {
            SD.remove(currentUploadName); // Entferne leere Dateien
            Serial.println("Leere Datei entfernt: " + currentUploadName);
        }
    } else {
        server.send(500, "text/plain", "Fehler beim Hochladen!");
    }
}

void WebServerHandler::handleUpload() {
    server.sendHeader("Location", "/files.html?uploaded=true");
    server.send(303); // Browser wird umgeleitet
}

bool WebServerHandler::saveToSPIFFS(const String& fileName, const String& content) {
    if (!SPIFFS.begin(true)) { // Initialisiert SPIFFS, falls nicht bereits geschehen
        Serial.println("SPIFFS konnte nicht initialisiert werden.");
        return false;
    }

    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Schreiben: " + fileName);
        return false;
    }

    if (file.print(content)) {
        Serial.println("Daten erfolgreich in Datei gespeichert: " + fileName);
    } else {
        Serial.println("Fehler beim Schreiben in Datei: " + fileName);
        file.close();
        return false;
    }

    file.close();
    return true;
}

String WebServerHandler::loadFromSPIFFS(const String& fileName) {
    if (!SPIFFS.begin(true)) { // Initialisiert SPIFFS, falls nicht bereits geschehen
        Serial.println("SPIFFS konnte nicht initialisiert werden.");
        return "";
    }

    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Lesen: " + fileName);
        return "";
    }

    String content = file.readString(); // Liest den gesamten Inhalt der Datei
    file.close();

    Serial.println("Daten erfolgreich aus Datei geladen: " + fileName);
    return content;
}

extern INA219Manager ina1;
extern INA219Manager ina2;
extern GY302Manager lightSensor;
extern VL53L1XManager distanceSensor;

int WebServerHandler::updateVoltageSamplesAndCalculateSoC(float newVoltage) {
    // Spannungssamples verwalten
    voltageSamples.push_back(newVoltage);
    if (voltageSamples.size() > 10) { // Maximale Anzahl an Samples
        voltageSamples.erase(voltageSamples.begin()); // Ältestes Sample entfernen
    }

    // SoC berechnen
    return this->calculateBatteryPercentageWithSamples(voltageSamples);
}

int WebServerHandler::calculateBatteryPercentageWithSamples(const std::vector<float>& voltageSamples) {
    if (voltageSamples.empty()) {
        return 0; // Keine Samples, keine Berechnung möglich
    }

    // Durchschnitt der Samples berechnen
    float averageVoltage = std::accumulate(voltageSamples.begin(), voltageSamples.end(), 0.0f) / voltageSamples.size();

    // Batteriestand berechnen
    if (averageVoltage >= 4.2) return 100;
    if (averageVoltage <= 3.3) return 0;

    // Lineare Interpolation zwischen 3.3V und 4.2V
    return (int)((averageVoltage - 3.3) / (4.2 - 3.3) * 100);
}

float WebServerHandler::calculateRemainingTime(float voltage, float current_mA, int batteryCapacity_mAh) {
    // Ladezustand in % berechnen
    int soc = this->updateVoltageSamplesAndCalculateSoC(voltage); // Aktualisiert Samples und berechnet SoC

    // Verbleibende Kapazität in mAh
    float remainingCapacity = (batteryCapacity_mAh * soc) / 100.0;

    // Laufzeit in Stunden berechnen
    if (current_mA > 0) {
        return remainingCapacity / current_mA;
    } else {
        return -1; // Fehler: Kein Stromverbrauch
    }
}

void WebServerHandler::handleSensorAPI() {
    float temp = sensorHandler.getTemperature();
    float hum = sensorHandler.getHumidity();
    float pres = sensorHandler.getPressure();
    
    float voltage = ina1.getBusVoltage_V();           // in Volt
    float current_mA = ina1.getCurrent_mA();          // in Milliampere
    float current_A = ina1.getCurrent_mA() / 1000.0;  // in Ampere
    float power = ina1.getBusPower_mW();              // in Watt
    float lux = lightSensor.getAverageLux();
    uint16_t distance = distanceSensor.readDistance();

    // Battery-Werte
    float Battery_voltage = battery.getSmoothedVoltage();           // in Volt
    float mA = battery.getSmoothedCurrent();
    if (mA < 0) {
       mA = fabs(mA);
    }
    float Battery_current_mA = mA;          // in Milliampere
    float Battery_current_A = Battery_current_mA / 1000.0;    // in Ampere
    float Battery_power = ina2.getBusPower_mW();              // in Watt

    // Zusätzliche Werte: Batterieprozentsatz und verbleibende Zeit
    int batteryPercentage = battery.calculateSOC(Battery_voltage);
    float remainingTime = battery.calculateRemainingTime();

    char json[512]; // Erhöhe die Größe, um Platz für die zusätzlichen Werte zu schaffen
    snprintf(json, sizeof(json),
             "{\"temperatur\":%.1f,\"luftfeuchtigkeit\":%.1f,\"druck\":%.1f,\"minTemperatur\":%.1f,\"maxTemperatur\":%.1f,\"spannung\":%.2f,\"stromstärke\":%.2f,\"leistung\":%.2f,\"helligkeit\":%.1f,\"entfernung\":%d,"
             "\"battery_spannung\":%.2f,\"battery_stromstärke_mA\":%.2f,\"battery_stromstärke_A\":%.2f,\"battery_leistung\":%.2f,\"battery_prozent\":%d,\"remaining_time\":%.2f}",
             temp, hum, pres, minTemp, maxTemp, voltage, current_mA, power, lux, distance,
             Battery_voltage, Battery_current_mA, Battery_current_A, Battery_power, batteryPercentage, remainingTime);
    server.send(200, "application/json", json);
}