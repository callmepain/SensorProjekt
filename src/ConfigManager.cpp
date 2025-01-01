#include "ConfigManager.h"

// Initialisiert SPIFFS und lädt die Konfiguration
bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Fehler beim Start von SPIFFS!");
        return false;
    }
    return loadConfig();
}

// Lädt die Konfiguration aus SPIFFS
bool ConfigManager::loadConfig() {
    File file = SPIFFS.open(configFilePath, "r");
    if (!file) {
        Serial.println("Konfigurationsdatei nicht gefunden!");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Fehler beim Laden der Konfigurationsdatei!");
        return false;
    }

    // JSON-Daten in die Konfiguration kopieren
    config.bluetooth = doc["bluetooth"] | false;
    config.bluetooth_name = doc["bluetooth_name"] | "ESP32";
    config.deviceName = doc["device_name"] | "ESP32";
    config.ipAddress.fromString(doc["ip_address"] | "0.0.0.0");
    config.gateway.fromString(doc["gateway"] | "0.0.0.0");
    config.subnet.fromString(doc["subnet_mask"] | "255.255.255.0");
    config.primaryDNS.fromString(doc["primary_dns"] | "8.8.8.8");
    config.secondaryDNS.fromString(doc["secondary_dns"] | "8.8.4.4");
    config.wifi_password = doc["wifi_password"] | "nix";
    config.wifi_ssid = doc["wifi_ssid"] | "niemand";
    config.led_enable = doc["led_enable"] | false;
    
    config.updateInterval = doc["updateInterval"] | 1000;

    Serial.println("Konfiguration erfolgreich geladen!");
    return true;
}

// Speichert die aktuelle Konfiguration in SPIFFS
bool ConfigManager::saveConfig() {
    File file = SPIFFS.open(configFilePath, "w");
    if (!file) {
        Serial.println("Fehler beim Öffnen der Konfigurationsdatei zum Schreiben!");
        return false;
    }

    JsonDocument doc;
    doc["bluetooth"] = config.bluetooth;
    doc["bluetooth_name"] = config.bluetooth_name;
    doc["device_name"] = config.deviceName;
    doc["ip_address"] = config.ipAddress;
    doc["gateway"] = config.gateway;
    doc["subnet_mask"] = config.subnet;
    doc["primary_dns"] = config.primaryDNS;
    doc["secondary_dns"] = config.secondaryDNS;
    doc["wifi_password"] = config.wifi_password;
    doc["wifi_ssid"] = config.wifi_ssid;
    doc["led_enable"] = config.led_enable;

    if (serializeJson(doc, file) == 0) {
        Serial.println("Fehler beim Schreiben in die Konfigurationsdatei!");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Konfiguration erfolgreich gespeichert!");
    return true;
}

void ConfigManager::printConfig() {
    Serial.println("Aktuelle Konfiguration:");

    // Erstelle ein JSON-Dokument für die Ausgabe
    JsonDocument doc;
    doc["bluetooth"] = config.bluetooth;
    doc["bluetooth_name"] = config.bluetooth_name;
    doc["deviceName"] = config.deviceName;
    doc["ipAddress"] = config.ipAddress.toString();
    doc["gateway"] = config.gateway.toString();
    doc["subnet"] = config.subnet.toString();
    doc["primaryDNS"] = config.primaryDNS.toString();
    doc["secondaryDNS"] = config.secondaryDNS.toString();
    doc["wifi_ssid"] = config.wifi_ssid;
    doc["wifi_password"] = config.wifi_password;
    doc["led_enable"] = config.led_enable;
    doc["updateInterval"] = config.updateInterval;

    // JSON in den seriellen Monitor ausgeben
    serializeJsonPretty(doc, Serial);
    Serial.println();
}

// Gibt einen Verweis auf die Konfigurationsdaten zurück
Config& ConfigManager::getConfig() {
    return config;
}

// Debugging: Listet alle Dateien im SPIFFS auf
void ConfigManager::listFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    while (file) {
        Serial.print("Datei: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}
