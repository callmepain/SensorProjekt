#include <Wire.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <time.h>
#include <Config.h>
#include "ConfigManager.h"
#include "MinMaxStorage.h"
#include "SensorHandler.h"
#include "WebServerHandler.h"
#include <SPIFFS.h>
#include "Battery.h"
#include "DisplayConfig.h"
#include "ButtonConfig.h"
#include "SensorConfig.h"
#include "MenuConfig.h"
#include <SPI.h>
#include <SD.h>
#include "SDCardLogger.h"
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_debug_helpers.h>
#include <esp_private/panic_reason.h>


#define SD_CS 5 // Pin für CS
int lastClkState;

TaskHandle_t menuTaskHandle;
WiFiServer telnetServer(23); // Telnet-Server auf Port 23
WiFiClient telnetClient;
ConfigManager configManager;  // Konfigurationsmanager erstellen
SensorHandler sensorHandler(bme);
SDCardLogger sdLogger("/system.log", SD_CS);
WebServerHandler webServerHandler(sensorHandler, sdLogger);
Battery battery(3680, "/battery_data.json", sdLogger, 0.03);

float totalEnergy_ina1 = 0.0;
float totalEnergy_ina2 = 0.0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000; // 1 Sekunde

unsigned long lastSwitchTime = 0; // Zeitstempel des letzten Wechsels
bool showPower = true;           // Zustand: true = Leistung, false = Gesamtenergie
unsigned long lastEnergyUpdateTime = 0; // Zeitstempel für Energieberechnung

// Globale Variable für den Reconnect-Status
static bool isReconnecting = false;

// Globale Variable für die Startzeit
static unsigned long startupTime;

void logCrashData(const char* reason) {
    uint32_t resetReason = esp_reset_reason();
    String resetReasonStr;
    
    switch(resetReason) {
        case ESP_RST_POWERON: resetReasonStr = "Power-on"; break;
        case ESP_RST_SW: resetReasonStr = "Software reset"; break;
        case ESP_RST_PANIC: resetReasonStr = "Panic reset"; break;
        case ESP_RST_INT_WDT: resetReasonStr = "Interrupt watchdog"; break;
        case ESP_RST_TASK_WDT: resetReasonStr = "Task watchdog"; break;
        case ESP_RST_WDT: resetReasonStr = "Other watchdog"; break;
        case ESP_RST_DEEPSLEEP: resetReasonStr = "Deep sleep"; break;
        case ESP_RST_BROWNOUT: resetReasonStr = "Brownout"; break;
        default: resetReasonStr = "Unknown"; break;
    }
    
    sdLogger.logError("System Reset - Grund: " + resetReasonStr);
    sdLogger.logError("Auslöser: " + String(reason));
    sdLogger.logError("Free heap: " + String(ESP.getFreeHeap()));
    sdLogger.logError("Min free heap: " + String(ESP.getMinFreeHeap()));
    sdLogger.logError("Max alloc heap: " + String(ESP.getMaxAllocHeap()));
}

void connectToWiFi() {
    Config& config = configManager.getConfig();
    sdLogger.logInfo("Verbinde mit WiFi...");
    if (!config.bluetooth) {
      // Bluetooth komplett abschalten
      btStop();
      sdLogger.logInfo("Bluetooth deaktiviert.");
    }
    // Versuche, die statische IP zu setzen
    if (!WiFi.config(config.ipAddress, config.gateway, config.subnet, config.primaryDNS, config.secondaryDNS)) {
        sdLogger.logError("Fehler beim Setzen der statischen IP.");
    } else {
        sdLogger.logInfo("Statische IP erfolgreich gesetzt:");
        sdLogger.logInfo("IP-Adresse: " + config.ipAddress.toString());
        sdLogger.logInfo("Gateway: " + config.gateway.toString());
        sdLogger.logInfo("Subnetzmaske: " + config.subnet.toString());
        sdLogger.logInfo("Primärer DNS: " + config.primaryDNS.toString());
        sdLogger.logInfo("Sekundärer DNS: " + config.secondaryDNS.toString());
    }

    const char* hostname = config.deviceName.c_str();
    if (!WiFi.setHostname(hostname)) {
        sdLogger.logInfo("Fehler beim Setzen des Hostnamens.");
    } else {
        sdLogger.logInfo("Hostname gesetzt: " + String(hostname));
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_ssid, config.wifi_password);

    // Timeout hinzufügen
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // Max 10 Sekunden warten
        delay(100);
        attempts++;
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi-Verbindung fehlgeschlagen!");
        return;
    }

    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

    sdLogger.logInfo("\nWiFi verbunden!");
    sdLogger.logInfo("Lokale IP-Adresse: " + WiFi.localIP().toString());
}

void syncNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    sdLogger.logInfo("Zeit über NTP synchronisiert!");
}

bool checkWiFiStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        return(true);
    } else {
        return(false);
    }
}

void drawSensors() {
  float t = bme.readTemperature();          // in °C
  float h = bme.readHumidity();             // in %
  float p = bme.readPressure() / 100.0F;    // in hPa
  // Sensorwerte
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("Temp Akt: "); display.print(t); display.print("C");

  display.setCursor(0, 10);
  display.print("Temp Min: "); display.print(minTemp); display.print("C");

  display.setCursor(0, 20);
  display.print("Temp Max: "); display.print(maxTemp); display.print("C");

  display.setCursor(0, 30);
  display.print("Feuchte:  "); display.print(h); display.print("%");

  display.setCursor(0, 40);
  display.print("Druck:    "); display.print(p); display.print(" hPa");

  drawNavBar("", "Back");
}

//solarpanel
void drawIna1() {
    float voltage = ina1.getBusVoltage_V();           // in Volt
    float current_mA = ina1.getCurrent_mA();    // in milliAmpere
    float current_A = ina1.getCurrent_mA() / 1000.0;    // in Ampere
    float power = ina1.getBusPower_mW();               // in milliWatt
    float lux = lightSensor.readLux();
    // Sensorwerte
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.print("Spannung: "); display.print(voltage); display.print("V");

    display.setCursor(0, 10);
    display.print("Strom mA: "); display.print(current_mA); display.print("mA");

    display.setCursor(0, 20);
    display.print("Energie:  "); display.print(totalEnergy_ina1); display.print("mWh");

    display.setCursor(0, 30);
    display.print("Watt:     "); display.print(power); display.print("mW");

    display.setCursor(0, 40);
    display.print("Lux:      "); display.print(lux); display.print("Lx");

    drawNavBar("", "Back");
}

//batterie
void drawIna2() {
    float voltage = ina2.getBusVoltage_V();
    float current_mA = ina2.getCurrent_mA();
    unsigned long currentTime = millis();

    // Batterie-Daten aktualisieren
    float smoothedVoltage = battery.getSmoothedVoltage();  // Gefilterte Spannung
    float smoothedCurrent = battery.getSmoothedCurrent();
    float remainingTime = battery.calculateRemainingTime();
    int batteryPercentage = battery.calculateSOC(smoothedVoltage);

    if (current_mA > 0) {
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print("Spannung: "); 
        display.print(smoothedVoltage); 
        display.print(" V");

        display.setCursor(0, 10);
        display.print("Strom mA: "); 
        display.print(smoothedCurrent); 
        display.print(" mA");

        // Umschalten zwischen Watt und Gesamtenergie alle 5 Sekunden
        if (currentTime - lastSwitchTime >= 5000) {
            lastSwitchTime = currentTime;
            showPower = !showPower; // Zustand umschalten
        }

        display.setCursor(0, 20);
        if (showPower) {
            display.print("Watt:     "); 
            display.print(ina2.getBusPower_mW());
            display.print(" mW");
        } else {
            display.print("Energie: "); 
            display.print(battery.getTotalEnergy());
            display.print(" Wh");
        }

        display.setCursor(0, 30);
        display.print("Restzeit: "); 
        display.print(remainingTime); 
        display.print(" h");

        // Anzeige des Batteriezustandes in %
        display.setCursor(0, 40);
        display.print("Batterie: "); 
        display.print(batteryPercentage); 
        display.print(" %");

        drawNavBar("", "Back");
    } else {
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print("Batterie laden... ");

        float currentChargingTime = battery.calculateChargingTime(current_mA);
        if (currentChargingTime >= 1.00) {
            display.setCursor(0, 10);
            display.print("Dauer: "); 
            display.print(currentChargingTime, 2); // Zwei Nachkommastellen für Stunden
            display.print(" Stunden");
        } else {
            float minutes = currentChargingTime * 60; // Umrechnung in Minuten
            display.setCursor(0, 10);
            display.print("Dauer: "); 
            display.print(minutes, 1); // Eine Nachkommastelle für Minuten
            display.print(" Minuten");
        }
        
        display.setCursor(0, 20);
        display.print("Ladestrom: "); 
        display.print(fabs(current_mA)); 
        display.print(" mA");

        float adjustedVoltage = smoothedVoltage * 0.95; // Korrekturfaktor anpassen!
        display.setCursor(0, 30);
        display.print("Ladespannung: "); 
        display.print(adjustedVoltage); 
        display.print(" V");

        // Anzeige des Batteriezustandes in %
        display.setCursor(0, 40);
        display.print("Batterie: "); 
        display.print(batteryPercentage); 
        display.print(" %");

        drawNavBar("", "Back");
    }
}

void drawClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    sdLogger.logInfo("Fehler beim Abrufen der Zeit!");

    // Fehler anzeigen, statt den Bildschirm leer zu lassen
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Zeit konnte");
    display.setCursor(0, 10);
    display.print("nicht geladen");
    display.display();
    return;
  }

  char timeBuffer[9];
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);
  char dateBuffer[11];
  strftime(dateBuffer, sizeof(dateBuffer), "%d.%m.%Y", &timeinfo);

  display.setTextSize(2);
  display.setCursor((128 - 12 * strlen(timeBuffer)) / 2, 10);
  display.print(timeBuffer);
  display.setTextSize(1);
  display.setCursor((128 - 6 * strlen(dateBuffer)) / 2, 40);
  display.print(dateBuffer);

  drawNavBar("", "Back");
}

void drawWifiInfo() {
    bool status = checkWiFiStatus();
    String boolToString = status ? "true" : "false";
    wifi_power_t txPowerValue = WiFi.getTxPower();
    String txPowerStr;
    switch (txPowerValue) {
        case WIFI_POWER_19_5dBm: txPowerStr = "19.5 dBm"; break;
        case WIFI_POWER_19dBm: txPowerStr = "19 dBm"; break;
        case WIFI_POWER_18_5dBm: txPowerStr = "18.5 dBm"; break;
        case WIFI_POWER_17dBm: txPowerStr = "17 dBm"; break;
        case WIFI_POWER_15dBm: txPowerStr = "15 dBm"; break;
        case WIFI_POWER_13dBm: txPowerStr = "13 dBm"; break;
        case WIFI_POWER_11dBm: txPowerStr = "11 dBm"; break;
        case WIFI_POWER_8_5dBm: txPowerStr = "8.5 dBm"; break;
        case WIFI_POWER_7dBm: txPowerStr = "7 dBm"; break;
        case WIFI_POWER_5dBm: txPowerStr = "5 dBm"; break;
        case WIFI_POWER_2dBm: txPowerStr = "2 dBm"; break;
        case WIFI_POWER_MINUS_1dBm: txPowerStr = "-1 dBm"; break;
        default: txPowerStr = "Unbekannt"; break;
    }
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Wifi Info");
    display.setCursor(0, 10);
    display.print("Empfang: "); display.print(WiFi.RSSI()); display.print("dBm");
    display.setCursor(0, 20);
    display.print("Verbindung: "); display.print(boolToString);
    display.setCursor(0, 30);
    display.print("tx Power: "); display.print((int)txPowerValue);
    
    drawNavBar("", "Back");
    display.display();
}

void drawLuxInfo() {
    static float maxAVG = 0.00;
    float avg = lightSensor.getAverageLux();
    if (avg > maxAVG) {
        maxAVG = avg;
    }
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Helligkeit: ");
    display.setCursor(0, 10);
    display.print("Aktuell:    "); display.print(lightSensor.readLux()); display.print(" Lx");
    display.setCursor(0, 20);
    display.print("Avg:        "); display.print(avg); display.print(" Lx");
    display.setCursor(0, 30);
    display.print("Max Avg:    "); display.print(maxAVG); display.print(" Lx");
    
    drawNavBar("", "Back");
    display.display();
}

void TestI2C() {
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
        sdLogger.logInfo("I2C device found at address 0x");
        if (address < 16) sdLogger.logInfo("0");
        sdLogger.logInfo("Address: " + String(address, HEX));
        nDevices++;
        } else if (error == 4) {
        sdLogger.logInfo("Unknown error at address 0x");
        if (address < 16) sdLogger.logInfo("0");
        sdLogger.logInfo("Address: " + String(address, HEX));
        }
    }

    if (nDevices == 0)
        sdLogger.logInfo("No I2C devices found\n");
    else
        sdLogger.logInfo("done\n");

}

// Globale Objekte
View sensorsView(drawSensors);
View clockView(drawClock);
View Ina1View(drawIna1);
View Ina2View(drawIna2);
View WifiView(drawWifiInfo);
View LuxView(drawLuxInfo);

void processMenuSelection() {
    String selected = currentMenu->getSelectedItem();
    if (currentMenu == &mainMenu) {
        if (selected == "Sensorwerte") {
            currentState = STATE_VIEW;
            currentView = &sensorsView;
        } else if (selected == "Uhr") {
            currentState = STATE_VIEW;
            currentView = &clockView;
        } else if (selected == "Strom") {
            currentState = STATE_VIEW;
            currentView = &Ina1View;
        } else if (selected == "Batterie") {
            currentState = STATE_VIEW;
            currentView = &Ina2View;
        } else if (selected == "Wifi") {
            currentState = STATE_VIEW;
            currentView = &WifiView;
        }  else if (selected == "Lux") {
            currentState = STATE_VIEW;
            currentView = &LuxView;
        } else if (selected == "Einstellungen") {
            currentMenu = &settingsMenu;
            settingsMenu.draw();
        } else {
            sdLogger.logInfo("Unbekannte Option ausgewählt: " + selected);
        }

    } else if (currentMenu == &settingsMenu) {
        if (selected == "Back") {
            currentMenu = &mainMenu;
            mainMenu.draw();
        } else {
            settingsMenu.toggleSelectedItem();
        }
    }
}

void handleMenuNavigation() {
    if (!isDisplayOn()) return;  // Zeichnen überspringen, wenn Display aus
    static int lastEncoderState = HIGH;
    static bool prevSelect = true;
    static unsigned long lastSelectPress = 0;

    // Rotary Encoder Zustände lesen
    int encoderState = digitalRead(CLK_PIN);
    bool dtState = digitalRead(DT_PIN);
    bool selectState = digitalRead(SW_PIN);

    // Navigation mit Rotary Encoder
    if (encoderState != lastEncoderState && encoderState == LOW) {
        // Drehrichtung prüfen
        if (dtState != encoderState) {
            // Nach unten navigieren
            if (currentState == STATE_MENU && currentMenu) {
                currentMenu->navigate("down");
            }
        } else {
            // Nach oben navigieren
            if (currentState == STATE_MENU && currentMenu) {
                currentMenu->navigate("up");
            }
        }
    }

    // Auswahl mit Taster (SW_PIN)
    if (prevSelect && !selectState) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastSelectPress > 300) { // Entprellen
            lastSelectPress = currentMillis;

            if (currentState == STATE_MENU && currentMenu) {
                processMenuSelection(); // Menüpunkt auswählen
            } else if (currentState == STATE_VIEW) {
                currentState = STATE_MENU; // Zurück ins Menü
                currentMenu = &mainMenu;
                mainMenu.draw();
            }
        }
    }

    lastEncoderState = encoderState;
    prevSelect = selectState;
}

void handleCurrentViewUpdate() {
    if (!isDisplayOn()) return;  // Zeichnen überspringen, wenn Display aus
    if (currentState == STATE_VIEW) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastUpdate >= updateInterval) {
            lastUpdate = currentMillis;
            currentView->show();
        }
    }
}

// Deklaration unserer Task-Funktion
void WebHandlingTask(void *pvParameters) {
    while (true) {
        webServerHandler.handleClient(); 
        vTaskDelay(pdMS_TO_TICKS(5));  // Minimales Delay für Task-Switching
    }
}

void menuTask(void *parameter) {
    for (;;) {
        // Nur Menu-Updates durchführen wenn Display an ist
        if (getDisplayState()) {
            handleMenuNavigation();
            handleCurrentViewUpdate();
        } else {
            // Längere Pause wenn Display aus ist, um CPU zu sparen
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(5));  // Minimales Delay für Task-Switching
    }
}

// WiFi Task mit Callback für abhängige Initialisierungen
void connectWiFiTask(void * parameter) {
    Serial.println("WiFi Task gestartet");
    connectToWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi-abhängige Initialisierungen abgeschlossen");
    }
    
    isReconnecting = false;
    vTaskDelete(NULL);
}

void checkAndReconnectWiFi() {
    static unsigned long lastWiFiCheck = 0;
    static bool servicesInitialized = false;
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastWiFiCheck >= 30000) {
        lastWiFiCheck = currentMillis;
        if (WiFi.status() == WL_CONNECTED && !servicesInitialized) {
            // Initialisiere Dienste nach erfolgreicher Verbindung
            syncNTP();
            webServerHandler.init();
            servicesInitialized = true;
            sdLogger.logInfo("WiFi-Dienste initialisiert");
        } else if (WiFi.status() != WL_CONNECTED) {
            servicesInitialized = false;
            if (!isReconnecting) {
                String message = "WiFi-Verbindung verloren. Versuche Neuverbindung...";
                sdLogger.logWarning(message);
                isReconnecting = true;
                xTaskCreate(
                    connectWiFiTask,
                    "WiFiReconnect",
                    4096,
                    NULL,
                    1,
                    NULL
                );
            }
        }
    }
}

void initConfigManager() {
    if (!configManager.begin()) {
        sdLogger.logInfo("Fehler beim Initialisieren des Config-Managers. Verwende Standardwerte...");
        Config& config = configManager.getConfig();
        config.bluetooth = false;
        config.bluetooth_name = "ESP32";
        config.deviceName = "ESP32";

        config.ipAddress.fromString("192.168.2.112");
        config.gateway.fromString("192.168.2.1");
        config.subnet.fromString("255.255.255.0");
        config.primaryDNS.fromString("8.8.8.8");
        config.secondaryDNS.fromString("8.8.4.4");
        config.wifi_password = "Alpha3k1?";
        config.wifi_ssid = "Kühle";
        configManager.saveConfig();
    }
    //configManager.listFiles();
    //configManager.printConfig();
    loadMinMaxFromJson();
}

void initSPIFFS() {
    if (!SPIFFS.begin(true)) {
        sdLogger.logInfo("Fehler beim Initialisieren von SPIFFS");
        return;
    }
    sdLogger.logInfo("SPIFFS erfolgreich gemountet");

}

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        sdLogger.logInfo("SSD1306 nicht gefunden!");
        while (true) {}
    }
    display.clearDisplay();
    display.display();

}

void initSensors() {
    Wire.begin(I2C_SDA, I2C_SCL);
    initDisplay();
    sensorHandler.init();
    
    if (!ina1.begin() || !ina2.begin() || !ina3.begin()) {
        sdLogger.logInfo("INA konnte nicht initialisiert werden!");
        Serial.println("INA Fehler!");
    }

    ina1.setADCMode(SAMPLE_MODE_4);
    ina1.setPGain(PG_40);
    ina1.setBusRange(BRNG_16);
    ina2.setADCMode(SAMPLE_MODE_16);
    ina2.setPGain(PG_160);
    ina2.setBusRange(BRNG_16);
    ina3.setADCMode(SAMPLE_MODE_16);
    ina3.setPGain(PG_160);
    ina3.setBusRange(BRNG_16);
    
    battery.initialize();
    float initialVoltage = ina2.getBusVoltage_V();
    float initialCurrent = ina2.getCurrent_mA();
    battery.initializeSamples(initialVoltage, initialCurrent);
    battery.startUpdateTask(1000, 1, 1);

    if (!lightSensor.begin()) {
        sdLogger.logInfo("GY-302 konnte nicht initialisiert werden!");
    }
    lightSensor.startSampling(200);
}

void initButtons() {
    // Pins für den Drehgeber initialisieren
    pinMode(CLK_PIN, INPUT);
    pinMode(DT_PIN, INPUT);
    pinMode(SW_PIN, INPUT_PULLUP);

    lastClkState = digitalRead(CLK_PIN);
}

void initTasks() {
    xTaskCreate(
        WebHandlingTask,    
        "WebHandlingTask",  
        8192,               // Von 4096 auf 8192 erhöht
        NULL,               
        1,                  
        NULL                
    );
    xTaskCreatePinnedToCore(
        menuTask,         
        "MenuTask",       
        8192,             // Von 4096 auf 8192 erhöht
        NULL,             
        1,                
        &menuTaskHandle,  
        1                 
    );
}

void setup() {
    // Watchdog für Setup aktivieren
    esp_task_wdt_init(30, true); // 30 Sekunden Timeout
    esp_task_wdt_add(NULL);
    
    startupTime = millis();
    logCrashData("Setup started");
    
    if (!SD.begin(SD_CS)) {
        sdLogger.logError("SD-Karte konnte nicht initialisiert werden.");
        Serial.println("SD-Karte Fehler!");
        return;
    }

    if (!sdLogger.begin()) {
        sdLogger.logError("SD-Karten-Logger konnte nicht initialisiert werden.");
        Serial.println("Logger Fehler!");
        return;
    }

    initConfigManager();
    initSPIFFS();
    initSensors();

    // WiFi-Verbindung in separatem Task starten
    xTaskCreate(
        connectWiFiTask,
        "WiFiConnect",
        4096,
        NULL,
        1,
        NULL
    );

    // Nicht-WiFi-abhängige Initialisierungen
    setCpuFrequencyMhz(80);
    sdLogger.logInfo("System gestartet");
    initDisplay();
    initButtons();
    mainMenu.draw();
    initTasks();

    // GPIO 17 als Ausgang definieren
    pinMode(17, OUTPUT);
    digitalWrite(17, LOW);

    unsigned long setupDuration = millis() - startupTime;
    Serial.printf("Setup fertig nach %lu ms\n", setupDuration);
    sdLogger.logInfo("Setup-Zeit: " + String(setupDuration) + "ms");

    // Watchdog für Setup deaktivieren
    esp_task_wdt_delete(NULL);
}

void loop() {
    static bool test = true;
    if (!test) {
        TestI2C();
        test = true;
    }
    
    sensorHandler.updateSensors();
    totalEnergy_ina1 = ina1.updateEnergy();
    totalEnergy_ina2 = ina2.updateEnergy();
    
    // Kurze Pause für andere Tasks
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms Pause statt yield()
    
    checkAndReconnectWiFi();

    static unsigned long lastWatchdogReset = 0;
    static unsigned long lastHeapCheck = 0;
    
    // Heap-Überwachung alle 5 Minuten
    if (millis() - lastHeapCheck > 300000) {
        lastHeapCheck = millis();
        if (ESP.getFreeHeap() < 10000) { // Weniger als 10KB frei
            sdLogger.logWarning("Kritisch niedriger Heap: " + String(ESP.getFreeHeap()) + " bytes");
        }
    }
    
    // Watchdog alle 5 Sekunden zurücksetzen
    if (millis() - lastWatchdogReset > 5000) {
        lastWatchdogReset = millis();
        esp_task_wdt_reset();
    }
}

// Crash-Handler hinzufügen
void IRAM_ATTR resetModule() {
    sdLogger.logError("Watchdog ausgelöst - System wird neu gestartet");
    esp_restart();
}
