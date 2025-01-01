#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <FS.h>
#include <SPIFFS.h>
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
    const char* configFilePath = "/config2.json";  // Standardpfad für die Config-Datei
    Config config;

public:
    // Konstruktor
    ConfigManager() {}

    // Initialisiert SPIFFS und lädt die Konfiguration
    bool begin();

    void printConfig();

    // Lädt die Konfiguration aus SPIFFS
    bool loadConfig();

    // Speichert die aktuelle Konfiguration in SPIFFS
    bool saveConfig();

    // Gibt einen Verweis auf die Konfigurationsdaten zurück
    Config& getConfig();

    // Debugging: Listet Dateien im SPIFFS auf
    void listFiles();
};

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
