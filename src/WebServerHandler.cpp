#include "WebServerHandler.h"
#include "MinMaxStorage.h"
#include "INA219Manager.h"
#include "GY302Manager.h"
#include <Update.h>
#include <vector>
#include <numeric> // Für std::accumulate
#include "Battery.h"
#include <Adafruit_SSD1306.h>
#include "DisplayConfig.h"
#include <SD.h>
#include "SDCardLogger.h"

extern Adafruit_SSD1306 display;
extern Battery battery; // Deklaration, keine Definition
extern SDCardLogger sdLogger;

WebServerHandler::WebServerHandler(SensorHandler& sensorHandler, SDCardLogger& logger)
    : server(80), sensorHandler(sensorHandler), sdLogger(logger) {}

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
        } else if (state == "off") {
            displayOn = false;
            // Display ausschalten (Beispiel für SSD1306)
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
            sdLogger.logInfo("OTA Update gestartet: " + String(upload.filename));
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Platz für Firmware reservieren
                sdLogger.logError("Update.begin fehlgeschlagen!");
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                sdLogger.logError("Update.write fehlgeschlagen!");
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (!Update.end(true)) {
                sdLogger.logError("Update.end fehlgeschlagen!");
                Update.printError(Serial);
            } else {
                sdLogger.logInfo("OTA Update erfolgreich abgeschlossen!");
            }
        } else {
            sdLogger.logError("OTA Fehler aufgetreten.");
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

    server.on("/save", HTTP_POST, std::bind(&WebServerHandler::handleSaveFile, this));

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
        sdLogger.logInfo("Empfangene JSON-Daten: " + body);

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
    sdLogger.logInfo("Webserver gestartet!");
}

void WebServerHandler::sendError(int code, String message) {
    server.send(code, "text/plain", message);
    sdLogger.logError(message);
}

File WebServerHandler::openFile(String path, const char* mode) {
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    File file = SPIFFS.open(path, mode);
    if (!file) {
        sdLogger.logError("Datei nicht gefunden: " + path);
    }
    return file;
}

void WebServerHandler::handleClient() {
    server.handleClient();
}

String WebServerHandler::escapeJsonString(const String& input) {
    String output = "";
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (c < 0x20) {
                    // Escapen von Steuerzeichen
                    char hex[7];
                    snprintf(hex, 7, "\\u%04x", c);
                    output += hex;
                } else {
                    output += c;
                }
        }
    }
    return output;
}

bool isValidFilename(String name) {
    for (size_t i = 0; i < name.length(); i++) {
        char c = name[i];
        if (c < 32 || c == '\"' || c == '\\') { // Verbotene Steuerzeichen oder JSON-feindliche Zeichen
            return false;
        }
    }
    return true;
}

void WebServerHandler::handleFileListAPI() {
    String requestedPath = "";
    if (server.hasArg("path")) {
        requestedPath = server.arg("path");
        sdLogger.logInfo("Requested path: " + requestedPath);
    }

    String json = "[";
    bool firstEntry = true;

    // Bestimme das Dateisystem und den tatsächlichen Pfad
    String filesystem = "";
    String actualPath = requestedPath;
    
    if (requestedPath.startsWith("SD:")) {
        filesystem = "SD";
        actualPath = requestedPath.substring(3); // Entferne "SD:"
    } else if (requestedPath.startsWith("SPIFFS:")) {
        filesystem = "SPIFFS";
        actualPath = requestedPath.substring(7); // Entferne "SPIFFS:"
    }

    // Debug-Ausgaben
    sdLogger.logInfo("Filesystem: " + filesystem);
    sdLogger.logInfo("Actual path: " + actualPath);

    // Stelle sicher, dass der Pfad mit "/" beginnt
    if (!actualPath.startsWith("/")) {
        actualPath = "/" + actualPath;
    }

    if (filesystem == "SD" || filesystem == "") {
        if (SD.begin(5)) {
            File dir = SD.open(actualPath);
            if (dir && dir.isDirectory()) {
                sdLogger.logInfo("Reading SD directory: " + actualPath);
                File file = dir.openNextFile();
                while (file) {
                    if (!firstEntry) json += ",";
                    firstEntry = false;
                    
                    bool isDir = file.isDirectory();
                    String fileName = String(file.name());
                    
                    // Debug-Ausgabe
                    sdLogger.logInfo("Found file: " + fileName + " " + (isDir ? "(dir)" : "(file)"));
                    
                    // Entferne den Basispfad vom Dateinamen für relative Pfade
                    if (fileName.startsWith(actualPath)) {
                        fileName = fileName.substring(actualPath.length());
                        if (fileName.startsWith("/")) {
                            fileName = fileName.substring(1);
                        }
                    }
                    
                    json += "{\"name\":\"" + escapeJsonString(fileName) + "\",";
                    json += "\"type\":\"" + String(isDir ? "dir" : "file") + "\",";
                    json += "\"size\":" + String(isDir ? 0 : file.size()) + "}";
                    
                    file = dir.openNextFile();
                }
                dir.close();
            } else {
                sdLogger.logError("Failed to open directory or not a directory: " + actualPath);
            }
        } else {
            sdLogger.logError("Failed to initialize SD card");
        }
    }

    if (filesystem == "SPIFFS" || filesystem == "") {
        File dir = SPIFFS.open(actualPath);
        if (dir && dir.isDirectory()) {
            sdLogger.logInfo("Reading SPIFFS directory: " + actualPath);
            File file = dir.openNextFile();
            while (file) {
                if (!firstEntry) json += ",";
                firstEntry = false;
                
                bool isDir = file.isDirectory();
                String fileName = String(file.name());
                
                // Debug-Ausgabe
                sdLogger.logInfo("Found file: " + fileName + " " + (isDir ? "(dir)" : "(file)"));
                
                // Entferne den Basispfad vom Dateinamen
                if (fileName.startsWith(actualPath)) {
                    fileName = fileName.substring(actualPath.length());
                    if (fileName.startsWith("/")) {
                        fileName = fileName.substring(1);
                    }
                }
                
                json += "{\"name\":\"" + escapeJsonString(fileName) + "\",";
                json += "\"type\":\"" + String(isDir ? "dir" : "file") + "\",";
                json += "\"size\":" + String(isDir ? 0 : file.size()) + "}";
                
                file = dir.openNextFile();
            }
            dir.close();
        } else {
            sdLogger.logError("Failed to open SPIFFS directory or not a directory: " + actualPath);
        }
    }

    json += "]";
    sdLogger.logInfo("Sending JSON response: " + json);
    server.send(200, "application/json", json);
}

// Neue Hilfsfunktion zum Auflisten von Unterordnern
void WebServerHandler::listDirectory(File dir, String path, String& json, bool& firstEntry) {
    File file = dir.openNextFile();
    while (file) {
        if (isValidFilename(file.name())) {
            if (!firstEntry) json += ",";
            firstEntry = false;
            
            bool isDir = file.isDirectory();
            String fullPath = path + "/" + String(file.name());
            
            json += "{\"name\":\"" + escapeJsonString(fullPath) + "\",";
            json += "\"type\":\"" + String(isDir ? "dir" : "file") + "\",";
            json += "\"size\":" + String(isDir ? 0 : file.size()) + "}";
            
            // Rekursiv Unterordner auflisten
            if (isDir) {
                listDirectory(file, fullPath, json, firstEntry);
            }
        }
        file = dir.openNextFile();
    }
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
    sdLogger.logInfo("Requested file content for: " + fileName);

    File file;
    bool fileOpened = false;

    // Prüfen, welches Dateisystem basierend auf dem Präfix verwendet werden soll
    if (fileName.startsWith("SD:")) {
        fileName = fileName.substring(3); // Entferne 'SD:'-Präfix
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        sdLogger.logInfo("Opening SD file: " + fileName);
        if (SD.begin(5)) {
            file = SD.open(fileName, "r");
            fileOpened = file;
        } else {
            sdLogger.logError("SD card initialization failed");
            server.send(500, "text/plain", "SD-Karte konnte nicht initialisiert werden.");
            return;
        }
    } else if (fileName.startsWith("SPIFFS:")) {
        fileName = fileName.substring(7); // Entferne 'SPIFFS:'-Präfix
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        sdLogger.logInfo("Opening SPIFFS file: " + fileName);
        file = SPIFFS.open(fileName, "r");
        fileOpened = file;
    } else {
        sdLogger.logError("Invalid prefix in filename: " + fileName);
        server.send(400, "text/plain", "Ungültiger Präfix. Unterstützt: SD:, SPIFFS:");
        return;
    }

    if (!fileOpened) {
        sdLogger.logError("Failed to open file: " + fileName);
        server.send(404, "text/plain", "Datei nicht gefunden: " + fileName);
        return;
    }

    String content = file.readString();
    file.close();
    sdLogger.logInfo("File content length: " + String(content.length()));
    server.send(200, "text/plain", content);
}

void WebServerHandler::handleSaveFile() {
    sdLogger.logInfo("\n--- handleSaveFile called ---");

    // Prüfe Content-Type
    String contentType = server.header("Content-Type");
    sdLogger.logInfo("Content-Type: " + contentType);

    // Prüfe die Anzahl der Argumente
    sdLogger.logInfo("Number of arguments: " + String(server.args()));
    
    // Zeige alle verfügbaren Argumente
    for(int i = 0; i < server.args(); i++) {
        sdLogger.logInfo("Arg " + String(i) + ": " + server.argName(i) + " = " + server.arg(i));
    }

    // Hole die Rohdaten direkt aus dem Request-Body
    String jsonData = server.arg(0);
    
    if (jsonData.length() == 0) {
        jsonData = server.arg("plain");
    }
    
    if (jsonData.length() == 0) {
        sdLogger.logError("No data received in any form");
        server.send(400, "text/plain", "Keine Daten empfangen");
        return;
    }

    sdLogger.logInfo("Received data length: " + String(jsonData.length()));
    // Verwende static_cast um den Typ-Konflikt zu lösen
    size_t previewLength = static_cast<size_t>(100);
    if (jsonData.length() < previewLength) previewLength = jsonData.length();
    sdLogger.logInfo("First chars: " + jsonData.substring(0, previewLength));

    // Verwende JsonDocument statt DynamicJsonDocument
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error) {
        String errorMsg = "JSON parsing failed: " + String(error.c_str());
        sdLogger.logError(errorMsg);
        sdLogger.logError("Failed JSON data: " + jsonData);
        server.send(400, "text/plain", errorMsg);
        return;
    }

    // JSON wurde erfolgreich geparst
    sdLogger.logInfo("JSON successfully parsed");

    // Prüfe die Felder
    if (!doc["name"].is<const char*>()) {
        sdLogger.logError("'name' field missing or wrong type");
        server.send(400, "text/plain", "'name' Feld fehlt oder hat falschen Typ");
        return;
    }

    if (!doc["content"].is<const char*>()) {
        sdLogger.logError("'content' field missing or wrong type");
        server.send(400, "text/plain", "'content' Feld fehlt oder hat falschen Typ");
        return;
    }

    String fileName = doc["name"].as<String>();
    String content = doc["content"].as<String>();

    sdLogger.logInfo("File name: " + fileName);
    sdLogger.logInfo("Content length: " + String(content.length()));

    File file;
    bool success = false;

    // Prüfen, welches Dateisystem verwendet werden soll
    if (fileName.startsWith("SD:")) {
        fileName = fileName.substring(3); // Entferne "SD:"-Präfix
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        
        if (SD.begin(5)) {
            if (SD.exists(fileName)) {
                SD.remove(fileName);
            }
            file = SD.open(fileName, "w");
            if (file) {
                success = file.print(content);
                file.close();
            }
        }
    } else if (fileName.startsWith("SPIFFS:")) {
        fileName = fileName.substring(7); // Entferne "SPIFFS:"-Präfix
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        
        if (SPIFFS.exists(fileName)) {
            SPIFFS.remove(fileName);
        }
        file = SPIFFS.open(fileName, "w");
        if (file) {
            success = file.print(content);
            file.close();
        }
    }

    if (success) {
        server.send(200, "text/plain", "Datei erfolgreich gespeichert");
    } else {
        server.send(500, "text/plain", "Fehler beim Speichern der Datei");
    }
}

// Neue Hilfsfunktion zum Erstellen von Verzeichnispfaden
void WebServerHandler::createDirectoryPath(fs::FS &fs, String path) {
    if (path.length() == 0) return;

    String currentPath = "";
    int start = 1; // Skip first slash
    
    while (start < path.length()) {
        int end = path.indexOf('/', start);
        if (end == -1) end = path.length();
        
        String folder = path.substring(0, end);
        if (!fs.exists(folder)) {
            fs.mkdir(folder);
        }
        
        start = end + 1;
    }
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
                sdLogger.logInfo("Datei von der SD-Karte gelöscht: " + fileName);
            } else {
                sdLogger.logError("Fehler beim Löschen der Datei von der SD-Karte: " + fileName);
            }
        }
    } else {
        sdLogger.logError("SD-Karte konnte nicht initialisiert werden.");
    }

    // Wenn die Datei nicht auf der SD-Karte gelöscht wurde, versuche SPIFFS
    if (!fileDeleted) {
        if (SPIFFS.exists(fileName)) {
            fileDeleted = SPIFFS.remove(fileName);
            if (fileDeleted) {
                sdLogger.logInfo("Datei aus SPIFFS gelöscht: " + fileName);
            } else {
                sdLogger.logError("Fehler beim Löschen der Datei aus SPIFFS: " + fileName);
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
            sdLogger.logError("Datei nicht auf der SD-Karte gefunden: " + fileName);
        }
    } else {
        sdLogger.logError("SD-Karte konnte nicht initialisiert werden.");
    }

    // Fallback: Wenn die Datei nicht auf der SD-Karte ist, SPIFFS verwenden
    if (!file) {
        file = SPIFFS.open(fileName, "r");
        if (!file) {
            server.send(404, "text/plain", "Datei nicht gefunden: " + fileName);
            sdLogger.logError("Datei nicht gefunden: " + fileName);
            return;
        }
        sdLogger.logInfo("Datei aus SPIFFS geöffnet: " + fileName);
    } else {
        sdLogger.logInfo("Datei von der SD-Karte geöffnet: " + fileName);
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
            sdLogger.logError("Kein Dateiname angegeben, Upload abgebrochen.");
            sendError(400, "Kein Dateiname angegeben");
            return;
        }
        if (!currentUploadName.startsWith("/")) {
            currentUploadName = "/" + currentUploadName;
        }
        sdLogger.logInfo("Upload gestartet: " + currentUploadName);

        // Öffne die Datei auf der SD-Karte
        File file = SD.open(currentUploadName, FILE_WRITE);
        if (!file) {
            sdLogger.logError("Fehler beim Öffnen der Datei auf der SD-Karte zum Schreiben!");
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
        sdLogger.logInfo("Upload beendet: " + currentUploadName + " (" + String(upload.totalSize) + " Bytes)");
        if (upload.totalSize == 0) {
            SD.remove(currentUploadName); // Entferne leere Dateien
            sdLogger.logInfo("Leere Datei entfernt: " + currentUploadName);
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
        sdLogger.logError("SPIFFS konnte nicht initialisiert werden.");
        return false;
    }

    File file = SPIFFS.open(fileName, "w");
    if (!file) {
        sdLogger.logError("Fehler beim Öffnen der Datei zum Schreiben: " + fileName);
        return false;
    }

    if (file.print(content)) {
        sdLogger.logInfo("Daten erfolgreich in Datei gespeichert: " + fileName);
    } else {
        sdLogger.logError("Fehler beim Schreiben in Datei: " + fileName);
        file.close();
        return false;
    }

    file.close();
    return true;
}

String WebServerHandler::loadFromSPIFFS(const String& fileName) {
    if (!SPIFFS.begin(true)) { // Initialisiert SPIFFS, falls nicht bereits geschehen
        sdLogger.logError("SPIFFS konnte nicht initialisiert werden.");
        return "";
    }

    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        sdLogger.logError("Fehler beim Öffnen der Datei zum Lesen: " + fileName);
        return "";
    }

    String content = file.readString(); // Liest den gesamten Inhalt der Datei
    file.close();

    sdLogger.logInfo("Daten erfolgreich aus Datei geladen: " + fileName);
    return content;
}

extern INA219Manager ina1;
extern INA219Manager ina2;
extern GY302Manager lightSensor;

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
         "{\"temperatur\":%.1f,\"luftfeuchtigkeit\":%.1f,\"druck\":%.1f,\"minTemperatur\":%.1f,\"maxTemperatur\":%.1f,\"spannung\":%.2f,\"stromstärke\":%.2f,\"leistung\":%.2f,\"helligkeit\":%.1f,"
         "\"battery_spannung\":%.2f,\"battery_stromstärke_mA\":%.2f,\"battery_stromstärke_A\":%.2f,\"battery_leistung\":%.2f,\"battery_prozent\":%d,\"remaining_time\":%.2f}",
         temp, hum, pres, minTemp, maxTemp, voltage, current_mA, power, lux,
         Battery_voltage, Battery_current_mA, Battery_current_A, Battery_power, batteryPercentage, remainingTime);
         
    server.send(200, "application/json", json);
}