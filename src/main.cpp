#include <Wire.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <time.h>
#include <Config.h>
#include "ConfigManager.h"
#include "LEDControl.h"
#include "MinMaxStorage.h"
#include "SensorHandler.h"
#include "WebServerHandler.h"
#include <SPIFFS.h>
#include "Battery.h"
#include "DisplayConfig.h"
#include "ButtonConfig.h"
#include "LEDConfig.h"
#include "SensorConfig.h"
#include "MenuConfig.h"
#include "TelnetLogger.h"

#define RELAIS_PIN 26

Battery battery(13600, "/battery_data.json");
TaskHandle_t menuTaskHandle;
WiFiServer telnetServer(23); // Telnet-Server auf Port 23
WiFiClient telnetClient;
TelnetLogger logger(&telnetClient);
ConfigManager configManager;  // Konfigurationsmanager erstellen
LEDControl ledControl(strip);
SensorHandler sensorHandler(bme);
WebServerHandler webServerHandler(sensorHandler);

float totalEnergy_ina1 = 0.0;
float totalEnergy_ina2 = 0.0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000; // 1 Sekunde

unsigned long lastSwitchTime = 0; // Zeitstempel des letzten Wechsels
bool showPower = true;           // Zustand: true = Leistung, false = Gesamtenergie
unsigned long lastEnergyUpdateTime = 0; // Zeitstempel für Energieberechnung

void connectToWiFi() {
    Config& config = configManager.getConfig();
    logger.log("Verbinde mit WiFi...");
    if (!config.bluetooth) {
      // Bluetooth komplett abschalten
      btStop();
      logger.log("Bluetooth deaktiviert.");
    }
    // Versuche, die statische IP zu setzen
    if (!WiFi.config(config.ipAddress, config.gateway, config.subnet, config.primaryDNS, config.secondaryDNS)) {
        Serial.println("Fehler beim Setzen der statischen IP.");
    } else {
        Serial.println("Statische IP erfolgreich gesetzt:");
        Serial.print("IP-Adresse: ");
        Serial.println(config.ipAddress);
        Serial.print("Gateway: ");
        Serial.println(config.gateway);
        Serial.print("Subnetzmaske: ");
        Serial.println(config.subnet);
        Serial.print("Primärer DNS: ");
        Serial.println(config.primaryDNS);
        Serial.print("Sekundärer DNS: ");
        Serial.println(config.secondaryDNS);
    }

    const char* hostname = config.deviceName.c_str();
    if (!WiFi.setHostname(hostname)) {
        logger.log("Fehler beim Setzen des Hostnamens.");
    } else {
        logger.log("Hostname gesetzt: " + String(hostname));
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_ssid, config.wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        logger.log(".");
    }

    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

    logger.log("\nWiFi verbunden!");
    logger.log("Lokale IP-Adresse: " + WiFi.localIP().toString());
}

void syncNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    logger.log("Zeit über NTP synchronisiert!");
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
    float current_mA = ina2.getCurrent_mA();          // in mA
    float power = ina2.getBusPower_mW();              // in mW
    unsigned long currentTime = millis();

    // Batterie-Daten aktualisieren
    //battery.update(voltage, current_mA, totalEnergy_ina2);

    float smoothedVoltage = battery.getSmoothedVoltage();
    float smoothedCurrent = battery.getSmoothedCurrent();
    float remainingTime = battery.calculateRemainingTime();
    int batteryPercentage = battery.calculateSOC(smoothedVoltage);

    if (current_mA > 0) {
        // Display aktualisieren
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print("Spannung: "); display.print(smoothedVoltage); display.print(" V");

        display.setCursor(0, 10);
        display.print("Strom mA: "); display.print(smoothedCurrent); display.print(" mA");

        // Umschalten zwischen Watt und Gesamtenergie alle 5 Sekunden
        if (currentTime - lastSwitchTime >= 5000) {
            lastSwitchTime = currentTime;
            showPower = !showPower; // Zustand umschalten
        }

        display.setCursor(0, 20);
        if (showPower) {
            display.print("Watt:     "); display.print(power); display.print(" mW");
        } else {
            display.print("Energie: "); display.print(battery.getTotalEnergy()); display.print(" mWh");
        }

        display.setCursor(0, 30);
        display.print("Restzeit: "); display.print(remainingTime); display.print(" h");

        display.setCursor(0, 40);
        display.print("Batterie: "); display.print(batteryPercentage); display.print(" %");

        drawNavBar("", "Back");
    } else {
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print("Batterie laden... ");

        float currentChargingTime = battery.calculateChargingTime(current_mA);
        display.setCursor(0, 20);
        display.print("Dauer: "); display.print(currentChargingTime); display.print(" Stunden");

        display.setCursor(0, 30);
        display.print("Ladestrom: "); display.print(fabs(current_mA)); display.print(" mA");

        display.setCursor(0, 40);
        display.print("Ladespannung: "); display.print(ina2.getBusVoltage_V()); display.print(" V");
        drawNavBar("", "Back");
    }
    
}

void drawDistance() {
    uint16_t distance = distanceSensor.readDistance();
    if(distanceSensor.isRangeValid()) {
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print("Distanz: "); display.print(distance); display.print("mm");
    } else {
        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.setCursor(0, 0);
        display.print("Messwert ungültig oder außer Reichweite");
    }

    drawNavBar("", "Back");
}

void drawClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    logger.log("Fehler beim Abrufen der Zeit!");

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

    logger.log("Scanning...");

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
        logger.log("I2C device found at address 0x");
        if (address < 16) logger.log("0");
        logger.log("Address: " + String(address, HEX));
        nDevices++;
        } else if (error == 4) {
        logger.log("Unknown error at address 0x");
        if (address < 16) logger.log("0");
        logger.log("Address: " + String(address, HEX));
        }
    }

    if (nDevices == 0)
        logger.log("No I2C devices found\n");
    else
        logger.log("done\n");
}

// Globale Objekte
View sensorsView(drawSensors);
View clockView(drawClock);
View Ina1View(drawIna1);
View Ina2View(drawIna2);
View DistanceView(drawDistance);
View WifiView(drawWifiInfo);
View LuxView(drawLuxInfo);

void handleSensorAndLEDControl() {
    if (ledControl.isOn()) {
        sensorHandler.checkSensors();
        sensorHandler.updateSensors();
        ledControl.update(sensorHandler.getTemperature());

        int potValue = analogRead(POT_PIN);
        ledControl.adjustBrightness(potValue);
    } else {
        sensorHandler.updateSensors();
    }

    totalEnergy_ina1 = ina1.updateEnergy();
    totalEnergy_ina2 = ina2.updateEnergy();
}

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
        } else if (selected == "Distanz") {
            currentState = STATE_VIEW;
            currentView = &DistanceView;
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
            logger.log("Unbekannte Option ausgewählt: " + selected);
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
    static bool prevDown = true;
    static bool prevSelect = true;
    static unsigned long lastSelectPress = 0;

    bool downState = digitalRead(buttonDownPin);
    bool selectState = digitalRead(buttonSelectPin);

    if (!downState && prevDown) {
        if (currentState == STATE_MENU && currentMenu) {
            currentMenu->navigate("down");
        }
    }

    if (prevSelect && !selectState) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastSelectPress > 300) {
            lastSelectPress = currentMillis;

            if (currentState == STATE_MENU && currentMenu) {
                processMenuSelection();
            } else if (currentState == STATE_VIEW) {
                currentState = STATE_MENU;
                currentMenu = &mainMenu;
                mainMenu.draw();
            }
        }
    }

    prevDown = downState;
    prevSelect = selectState;
}

void handleCurrentViewUpdate() {
    if (currentState == STATE_VIEW) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastUpdate >= updateInterval) {
            lastUpdate = currentMillis;
            currentView->show();
        }
    }
}

void handleTelnetConnection() {
    if (telnetServer.hasClient()) {
        if (telnetClient) {
            telnetClient.stop();
        }
        telnetClient = telnetServer.available();
        logger.log("Neuer Telnet-Client verbunden");
    }

    if (telnetClient && telnetClient.connected()) {
        while (telnetClient.available()) {
            char c = telnetClient.read();
            Serial.write(c);
        }
    }
}

// Deklaration unserer Task-Funktion
void WebHandlingTask(void *pvParameters) {
    while (true) {
        webServerHandler.handleClient(); 
        vTaskDelay(10); // oder höher, damit der Task nicht die CPU flutet
    }
}

void menuTask(void *parameter) {
    for (;;) {
        handleMenuNavigation();  // Menülogik
        handleCurrentViewUpdate();  // Display-Updates

        // Task-Delay für reduzierte CPU-Auslastung
        vTaskDelay(pdMS_TO_TICKS(100)); // 100 ms Intervall
    }
}

// -----------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------
void initConfigManager() {
    if (!configManager.begin()) {
        logger.log("Fehler beim Initialisieren des Config-Managers. Verwende Standardwerte...");
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
    configManager.listFiles();
    configManager.printConfig();
    loadMinMaxFromJson();
}

void initSPIFFS() {
    if (!SPIFFS.begin(true)) {
        logger.log("Fehler beim Initialisieren von SPIFFS");
        return;
    }
    logger.log("SPIFFS erfolgreich gemountet");
}

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        logger.log("SSD1306 nicht gefunden!");
        while (true) {}
    }
    display.clearDisplay();
    display.display();
}

void initSensors() {
    Wire.begin(I2C_SDA, I2C_SCL);
    initDisplay();
    sensorHandler.init();
    if (!ina1.begin() || !ina2.begin()) {
        logger.log("INA konnte nicht initialisiert werden!");
    }
    
    ina1.setADCMode(SAMPLE_MODE_128);
    ina1.setPGain(PG_320);
    ina1.setBusRange(BRNG_16);
    ina2.setADCMode(SAMPLE_MODE_128);
    ina2.setPGain(PG_320);
    ina2.setBusRange(BRNG_16);

    float initialVoltage = ina2.getBusVoltage_V();
    float initialCurrent = ina2.getCurrent_mA();
    //battery.initializeSamples(initialVoltage, initialCurrent);
    battery.initializeSamples(initialVoltage, initialCurrent);
    battery.startUpdateTask(1000, 1, 1);
    //battery.startSampling(1000);

    if (!lightSensor.begin()) {
        logger.log("GY-302 konnte nicht initialisiert werden!");
    }
    lightSensor.startSampling(200);
    logger.log("GY-302 gestartet.");
    if (!distanceSensor.begin()) {
        logger.log("VL53L1X konnte nicht initialisiert werden!");
    }
    logger.log("VL53L1X gestartet.");
    battery.loadFromJson();
}

void initButtons() {
    pinMode(buttonDownPin, INPUT_PULLUP);
    pinMode(buttonSelectPin, INPUT_PULLUP);
}

void initTasks() {
    xTaskCreate(
        WebHandlingTask,    // Task-Funktion
        "WebHandlingTask",  // Name
        4096,               // Stack-Größe in Bytes
        NULL,               // Parameter
        1,                  // Priorität
        NULL                // Task-Handle
    );
    xTaskCreatePinnedToCore(
        menuTask,         // Task-Funktion
        "MenuTask",       // Name des Tasks
        4096,             // Stackgröße
        NULL,             // Parameter
        1,                // Priorität
        &menuTaskHandle,  // Task-Handle
        1                 // Core (optional)
    );
}

void setup() {
    Serial.begin(115200);

    pinMode(RELAIS_PIN, OUTPUT); // Pin als Ausgang definieren
    digitalWrite(RELAIS_PIN, LOW); // Relais anfangs ausgeschaltet

    initConfigManager();
    initSPIFFS();
    ledControl.init();
    initSensors();
    initDisplay();
    initButtons();
    connectToWiFi();
    syncNTP();
    webServerHandler.init();
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    mainMenu.draw();
    initTasks();
}

void CheckVoltage() {
    static bool Charging = false;
    float Voltage = battery.getSmoothedVoltage();
    if (Voltage <= 3.8 && !Charging) {
        digitalWrite(RELAIS_PIN, HIGH);
        Charging = true;
    } else if (Voltage >= 3.85 && Charging) {
        digitalWrite(RELAIS_PIN, LOW);
        Charging = false;
    }
}

void loop() {
    static bool test = true;
    if (!test) {
        TestI2C();
        test = true;
    }

    handleTelnetConnection();
    handleSensorAndLEDControl();
    CheckVoltage();
    // Kurzes Debouncing
    delay(100);
}
