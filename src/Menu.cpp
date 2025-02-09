#include "Menu.h"
#include "ConfigManager.h"
#include "DisplayConfig.h"

int test = 1;
// Definition von externen Variablen (müssen im Hauptprogramm deklariert sein)
extern Adafruit_SSD1306 display;
extern Settings settings;

void drawNavBar(const char* left, const char* right) {
    display.fillRect(0, 54, SCREEN_WIDTH, 10, WHITE);  // Hintergrund zeichnen
    display.setTextColor(BLACK);

    // Linker Button
    display.setCursor(5, 55);
    display.print(left);

    // Trennlinie
    display.drawLine(SCREEN_WIDTH / 2, 54, SCREEN_WIDTH / 2, 64, BLACK);

    // Rechter Button
    display.setCursor(SCREEN_WIDTH / 2 + 5, 55);  // Abstand von der Trennlinie
    display.print(right);

    display.setTextColor(WHITE);
}

void Menu::navigate(const String& direction) {
    int menuLength = items.size();
    if (direction == "down") {
        selectedIndex++;
        if (selectedIndex >= menuLength) {
            selectedIndex = 0;
            startIndex = 0;
        } else if (selectedIndex >= startIndex + maxVisibleItems) {
            startIndex++;
        }
    } else if (direction == "up") {
        selectedIndex--;
        if (selectedIndex < 0) {
            selectedIndex = menuLength - 1;
            startIndex = (menuLength - maxVisibleItems >= 0) ? menuLength - maxVisibleItems : 0;
        } else if (selectedIndex < startIndex) {
            startIndex--;
        }
    }
    draw();
}

void Menu::draw() {
     if (!isDisplayOn()) return;  // Zeichnen überspringen, wenn Display aus
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    int visibleEnd = startIndex + maxVisibleItems;
    if (visibleEnd > (int)items.size()) {
        visibleEnd = items.size();
    }

    for (int i = startIndex; i < visibleEnd; i++) {
        int row = i - startIndex;
        if (i == selectedIndex) {
            display.fillRect(0, row * 10, SCREEN_WIDTH, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(5, row * 10 + 1);
        display.print(items[i]);
    }

    drawNavBar("Navigate", "Select");
    display.display();
}

String Menu::getSelectedItem() {
    return items[selectedIndex];
}

void SubMenu::toggleSelectedItem() {
    // Zugriff auf die Konfiguration
    Config& config = configManager.getConfig();

    // Aktuelles Menüelement
    String selected = items[selectedIndex];

    // Zustand basierend auf dem ausgewählten Item umschalten
    if (selected == "bluetooth") {
        config.bluetooth = !config.bluetooth;
        if (config.bluetooth) {
            Serial.println("Bluetooth aktiviert");
            btStart();  // Bluetooth aktivieren
        } else {
            Serial.println("Bluetooth deaktiviert");
            btStop();  // Bluetooth deaktivieren
        }
    } else if (selected == "ledStrip") {
        // Relais umschalten. Konfiguration aktualisieren
        config.led_enable = !config.led_enable;
        if (config.led_enable) {
            Serial.println("Relais aktiviert");
            digitalWrite(17, HIGH); // Aktivierung des Relais, ggf. HIGH oder LOW, je nach Modul
        } else {
            Serial.println("Relais deaktiviert");
            digitalWrite(17, LOW);  // Deaktivierung des Relais
        }
    }

    // Konfiguration speichern
    if (!configManager.saveConfig()) {
        Serial.println("Fehler beim Speichern der Konfiguration!");
    }

    // Menü neu zeichnen, um aktualisierten Zustand zu zeigen
    draw();
}

void SubMenu::draw() {
    Config& config = configManager.getConfig();

    display.clearDisplay();
    display.setTextSize(1);

    int menuLength = items.size();
    int visibleEnd = startIndex + maxVisibleItems;
    if (visibleEnd > menuLength) {
        visibleEnd = menuLength;
    }

    for (int i = startIndex; i < visibleEnd; i++) {
        int row = i - startIndex;
        String item = items[i];

        // Zustand hinzufügen
        if (item == "bluetooth") item += config.bluetooth ? ": ON" : ": OFF";
        else if (item == "ledStrip") item += config.led_enable ? ": ON" : ": OFF";

        // Hervorhebung des ausgewählten Eintrags
        if (i == selectedIndex) {
            display.fillRect(0, row * 10, SCREEN_WIDTH, 10, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }

        display.setCursor(5, row * 10 + 1);
        display.print(item);
    }

    drawNavBar("Navigate", "Select");
    display.display();
}
