#ifndef OLED_MENU_H
#define OLED_MENU_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include <functional>
#include <memory>
#include "DisplayConfig.h"

// Menüeintrag mit optionalem Submenü
struct MenuEntry {
    String name;                                    // Anzeigename des Menüpunkts
    std::function<void()> rightFunction;           // Rechte Button-Funktion
    std::shared_ptr<class OLEDMenu> subMenu;       // Submenü (optional)
};

class OLEDMenu {
public:
    OLEDMenu(std::shared_ptr<Adafruit_SSD1306> display, std::shared_ptr<OLEDMenu> parent = nullptr)
        : display(display), parentMenu(parent), selectedItem(0), scrollOffset(0), maxVisibleEntries(5) {}

    // Menüeintrag hinzufügen
    void addEntry(const String& name, std::function<void()> rightFunction = nullptr) {
        entries.push_back({name, rightFunction, nullptr});
    }

    // Submenü hinzufügen
    void addSubMenu(const String& name, std::shared_ptr<OLEDMenu> subMenu) {
        subMenu->parentMenu = shared_from_this(); // Setze Elternmenü
        entries.push_back({name, nullptr, subMenu});
    }

    // Menü zeichnen
    void drawMenu() {
        if (!isDisplayOn()) return;
        display->clearDisplay();
        display->setTextSize(1);

        for (size_t i = 0; i < maxVisibleEntries; ++i) {
            size_t entryIndex = scrollOffset + i;
            if (entryIndex >= entries.size()) break;

            if (entryIndex == selectedItem) {
                display->fillRect(0, i * 10, 128, 10, SSD1306_WHITE);
                display->setTextColor(SSD1306_BLACK);
            } else {
                display->setTextColor(SSD1306_WHITE);
            }

            display->setCursor(5, i * 10 + 1);
            display->println(entries[entryIndex].name);
        }

        drawButtons();
        display->display();
    }

    // Navigiere nach oben
    void navigateUp() {
        if (selectedItem > 0) {
            selectedItem--;
            if (selectedItem < scrollOffset) {
                scrollOffset--;
            }
        }
        drawMenu();
    }

    // Navigiere nach unten
    void navigateDown() {
        if (selectedItem < entries.size() - 1) {
            selectedItem++;
            if (selectedItem >= scrollOffset + maxVisibleEntries) {
                scrollOffset++;
            }
        }
        drawMenu();
    }

    // Auswahl treffen
    void select() {
        auto& entry = entries[selectedItem];
        if (entry.subMenu) {
            entry.subMenu->drawMenu();
        } else if (entry.rightFunction) {
            entry.rightFunction();
        }
    }

    // Zurück zum übergeordneten Menü
    void goBack() {
        if (parentMenu) {
            parentMenu->drawMenu();
        }
    }

private:
    std::shared_ptr<Adafruit_SSD1306> display;
    std::vector<MenuEntry> entries;
    size_t selectedItem;
    size_t scrollOffset;
    const size_t maxVisibleEntries;
    std::shared_ptr<OLEDMenu> parentMenu;

    void drawButtons() {
        display->fillRect(0, 52, 128, 12, SSD1306_WHITE);
        display->drawLine(64, 52, 64, 63, SSD1306_BLACK);

        display->setTextColor(SSD1306_BLACK);
        display->setCursor(5, 54);
        display->println("Back");
        display->setCursor(69, 54);
        display->println("Select");
    }
};

#endif
