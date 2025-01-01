#ifndef TELNET_LOGGER_H
#define TELNET_LOGGER_H

#include <WiFi.h>
#include <vector>

class TelnetLogger {
private:
    WiFiClient *telnetClient;
    size_t bufferSize;

public:
    // Konstruktor mit optionaler Puffergröße
    TelnetLogger(WiFiClient *client, size_t bufferSize = 256) 
        : telnetClient(client), bufferSize(bufferSize) {}

    // Log-Nachricht senden
    void log(const String &msg) {
        Serial.println(msg);  // Immer an Serial ausgeben
        if (telnetClient && telnetClient->connected()) {
            telnetClient->println(msg);  // An den Telnet-Client senden
        }
    }

    // Verbindung überprüfen
    bool isConnected() const {
        return telnetClient && telnetClient->connected();
    }

    // Nachricht mit Formatierung senden
    void logFormatted(const char *format, ...) {
        char buffer[256]; // Temporärer Puffer
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        log(String(buffer)); // Nutzt die bestehende log()-Methode
    }

    // Überladung für numerische Werte
    void logFormatted(float value) {
        char buffer[32]; // Kleiner Puffer für eine Zahl
        snprintf(buffer, sizeof(buffer), "%.2f", value); // Format mit 2 Dezimalstellen
        log(String(buffer));
    }

    void logFormatted(int value) {
        char buffer[16]; // Puffer für Ganzzahlen
        snprintf(buffer, sizeof(buffer), "%d", value);
        log(String(buffer));
    }

    // Einfaches Logging für numerische Werte
    template <typename T>
    void logValue(const T &value) {
        log(String(value));
    }

    // Telnet-Client aktualisieren
    void setClient(WiFiClient *client) {
        telnetClient = client;
    }

    // Puffergröße anpassen
    void setBufferSize(size_t newSize) {
        bufferSize = newSize;
    }
};

#endif // TELNET_LOGGER_H
