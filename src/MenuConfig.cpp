#include "MenuConfig.h"

std::vector<String> mainMenuItems = {
    "Sensorwerte", "Uhr", "Strom", "Batterie", "Lux", "Wifi", "Einstellungen"
};
Menu mainMenu(mainMenuItems);

std::vector<String> settingsItems = {
    "bluetooth", "ledStrip", "Back"
};
SubMenu settingsMenu(settingsItems);

AppState currentState = STATE_MENU;
Menu* currentMenu = &mainMenu;
View* currentView = nullptr;
