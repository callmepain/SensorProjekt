#ifndef STATE_H
#define STATE_H

#include "Menu.h"  // Enthält die vollständige Definition von View und Menu

// App-Zustände
enum AppState { STATE_MENU, STATE_VIEW };

// Globale Variablen
extern AppState currentState;
extern Menu* currentMenu;
extern View* currentView;

#endif // STATE_H
