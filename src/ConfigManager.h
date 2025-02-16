#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <FS.h>
#include <SD.h>  // Füge SD-Karten-Unterstützung hinzu
#include <ArduinoJson.h>

struct Config {
    bool bluetooth;
    String bluetooth_name;
    String deviceName;
    IPAddress ipAddress;       // Lokale IP-Adresse
    IPAddress gateway;         // Gateway
    IPAddress subnet;          // Subnetzmaske
    IPAddress primaryDNS;      // Primärer DNS-Server (optional)
    IPAddress secondaryDNS;    // Sekundärer DNS-Server (optional)
    int tx_power;
    String wifi_password;
    String wifi_ssid;
    bool led_enable;
    
    int updateInterval;
};

class ConfigManager {
private:
    const char* configFilePath = "/config2.json";  // Pfad auf der SD-Karte
    Config config;

public:
    // Konstruktor
    ConfigManager() {}

    // Initialisiert SD-Karte und lädt die Konfiguration
    bool begin();

    void printConfig();

    // Lädt die Konfiguration von der SD-Karte
    bool loadConfig();

    // Speichert die aktuelle Konfiguration auf der SD-Karte
    bool saveConfig();

    // Gibt einen Verweis auf die Konfigurationsdaten zurück
    Config& getConfig();

    // Debugging: Listet Dateien auf der SD-Karte auf
    void listFiles();
};

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
