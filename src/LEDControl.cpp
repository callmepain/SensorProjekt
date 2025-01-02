#include "LEDControl.h"
#include "ConfigManager.h"
#include "SensorHandler.h"
#include "ButtonConfig.h"
#include "TelnetLogger.h"
#include "LEDConfig.h"

extern SensorHandler sensorHandler;
extern TelnetLogger logger;

LEDControl::LEDControl()
    : transitionSpeed(1.0f / 60.0f), currentColor(CRGB::Black), currentBrightness(255) {
    ledMutex = xSemaphoreCreateMutex(); // Mutex initialisieren

    // Config laden
    Config& config = configManager.getConfig(); // Zugriff auf die Konfiguration
    lightState = config.led_enable; // lightState aus der Config setzen
}

// Initialize LEDs
void LEDControl::init() {
    Config& config = configManager.getConfig();
    lightState = config.led_enable; // lightState aus der Config setzen

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(currentBrightness);
    FastLED.clear();
    Serial.println("LEDControl initialized with Config:");
    Serial.println("Brightness: " + String(currentBrightness));
    Serial.println("State: " + String(lightState ? "ON" : "OFF"));
}

void LEDControl::startTransition(CRGB targetColor) {
    if (currentTransition.active) return; // Verhindere parallele Übergänge

    currentTransition.startColor = currentColor;
    currentTransition.endColor = targetColor;
    currentTransition.progress = 0.0f;
    currentTransition.active = true;
}

// Adjust brightness based on potentiometer value
void LEDControl::adjustBrightness(int potValue) {
    int smoothedValue = readPotValueWithTolerance(potValue); // Toleranz beachten
    int newBrightness = map(smoothedValue, 0, 4095, 0, 255); // Mappe den Wert (0-4095) auf Helligkeit (0-255)

    // Aktualisieren nur, wenn sich die Helligkeit signifikant geändert hat
    if (newBrightness != currentBrightness) {
        setBrightness(newBrightness); // Helligkeit setzen
        Serial.print("Helligkeit angepasst: ");
        Serial.println(newBrightness);
    }
}

void LEDControl::fadeBrightness(int targetBrightness, int duration) {
    int startBrightness = currentBrightness;
    int steps = duration / 16; // Basierend auf 60 FPS

    for (int i = 0; i <= steps; i++) {
        currentBrightness = map(i, 0, steps, startBrightness, targetBrightness);
        FastLED.setBrightness(currentBrightness);
        FastLED.show();
        delay(16);
    }
}

// Read potentiometer value with tolerance
int LEDControl::readPotValueWithTolerance(int currentValue, int tolerance) {
    if (abs(currentValue - lastPotValue) > tolerance) {
        lastPotValue = currentValue; // Aktualisieren bei signifikanter Änderung
    }
    return lastPotValue;
}

void LEDControl::runTask() {
    const int frameDelay = 1000 / desiredFPS; // Dynamisch abhängig von desiredFPS
    while (true) {
        if (lightState) {
            float temp = sensorHandler.getTemperature();
            if (isnan(temp)) {
                Serial.println("Fehler: Ungültige Temperatur");
                temp = 20.0; // Standardwert
            }
            update(temp); // currentTemperature als globaler Wert oder über Setter
            int potValue = analogRead(POT_PIN);
            //adjustBrightness(122);
            adjustBrightness(readPotValueWithTolerance(potValue));
        }
        vTaskDelay(frameDelay / portTICK_PERIOD_MS);
    }
}

// Update LEDs
void LEDControl::update(float temperature) {
    if (!lightState) return;

    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
        CRGB targetColor = getColorForTemperature(temperature);

        if (!currentTransition.active || currentTransition.endColor != targetColor) {
            startTransition(targetColor);
        }

        if (currentTransition.active) {
            currentTransition.progress += transitionSpeed;
            if (currentTransition.progress >= 1.0f) {
                currentTransition.progress = 1.0f;
                currentTransition.active = false;
            }

            float easedProgress = easeInOutQuad(currentTransition.progress);
            currentColor = blend(currentTransition.startColor, currentTransition.endColor, easedProgress * 255);

            fill_solid(leds, NUM_LEDS, currentColor);
            FastLED.show();
        }

        xSemaphoreGive(ledMutex);
    }
}

// Setzt die Helligkeit der LEDs
void LEDControl::setBrightness(int brightness) {
    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) { // Zugriff sichern
        currentBrightness = brightness; // Helligkeit speichern
        FastLED.setBrightness(brightness); // Helligkeit anwenden
        if (lightState) {
            FastLED.show(); // Änderungen anzeigen, wenn LEDs eingeschaltet sind
        }
        xSemaphoreGive(ledMutex); // Zugriff freigeben
    }
}

// Turn LEDs on
void LEDControl::turnOn() {
    if (!lightState) { // Nur einschalten, wenn LEDs aus sind
        lightState = true;
        FastLED.setBrightness(currentBrightness > 0 ? currentBrightness : 255); // Letzte oder Standardhelligkeit
        fill_solid(leds, NUM_LEDS, currentColor); // Alle LEDs mit der letzten Farbe füllen
        FastLED.show(); // LEDs aktualisieren
    }
}

// Turn LEDs off
void LEDControl::turnOff() {
    if (lightState) { // Nur ausschalten, wenn LEDs an sind
        lightState = false;
        currentBrightness = FastLED.getBrightness(); // Aktuelle Helligkeit speichern
        FastLED.clear(); // Alle LEDs ausschalten
        FastLED.show(); // Änderungen anzeigen
    }
}

// Check if LEDs are on
bool LEDControl::isOn() const {
    return lightState;
}

// Map temperature to color
CRGB LEDControl::getColorForTemperature(float temp) {
    float ratio = constrain((temp - 18.0) / (28.0 - 18.0), 0.0, 1.0);
    uint8_t hue = map(ratio * 255, 0, 255, 160, 200); // Hue von Blau nach Magenta
    return CHSV(hue, 255, 255);
}

float LEDControl::easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
}