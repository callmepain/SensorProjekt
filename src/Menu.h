#ifndef MENU_H
#define MENU_H

#include <vector>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Externe Variablen
extern Adafruit_SSD1306 display;

void drawNavBar(const char* left, const char* right);

extern struct Settings {
    bool bluetooth;
    bool ledStrip;
    bool test1;
    bool test2;
    bool test3;
    bool test5;
} settings;

// Basisklasse Menu
class Menu {
protected:
    std::vector<String> items;
    int selectedIndex;
    int startIndex;
    int maxVisibleItems;

public:
    Menu(const std::vector<String>& entries, int maxVisible = 5)
        : items(entries), selectedIndex(0), startIndex(0), maxVisibleItems(maxVisible) {}

    virtual ~Menu() {}

    void navigate(const String& direction);
    virtual void draw();
    virtual String getSelectedItem();  // Gibt das aktuell ausgewählte Item zurück
    int getSelectedIndex() { return selectedIndex; }
};

// SubMenu: erbt von Menu
class SubMenu : public Menu {
public:
    using Menu::Menu;  // Konstruktor übernehmen
    void toggleSelectedItem();  // Toggelt den Zustand eines Items
    void draw() override;
};

// View-Klasse
class View {
private:
    void (*drawFunction)();  // Funktion, die gezeichnet wird

public:
    View(void (*func)()) : drawFunction(func) {}
    void show() {
        display.clearDisplay();
        if (drawFunction) drawFunction();
        display.display();
    }
};

#endif // MENU_H
