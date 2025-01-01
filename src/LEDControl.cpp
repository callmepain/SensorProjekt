#include "LEDControl.h"
#include "ConfigManager.h"
#include "SensorHandler.h"
#include "ButtonConfig.h"
#include "TelnetLogger.h"

extern SensorHandler sensorHandler;
extern TelnetLogger logger;

LEDControl::LEDControl(Adafruit_NeoPixel& strip)
    : strip(strip), transitionSpeed(1.0f / 60.0f), currentColor(0), currentBrightness(255) {
    ledMutex = xSemaphoreCreateMutex(); // Mutex initialisieren

    // Config laden
    Config& config = configManager.getConfig(); // Zugriff auf die Konfiguration
    lightState = config.led_enable; // lightState aus der Config setzen
}

// Initialize LEDs
void LEDControl::init() {
    Config& config = configManager.getConfig();
    lightState = config.led_enable; // lightState aus der Config setzen
    strip.begin();
    strip.clear();
    strip.setBrightness(currentBrightness);
    strip.show();
    Serial.println("LEDControl initialized with Config:");
    Serial.println("Brightness: " + String(currentBrightness));
    Serial.println("State: " + String(lightState ? "ON" : "OFF"));
}

void LEDControl::startTransition(uint32_t targetColor) {
    if (currentTransition.active) return; // Verhindere, dass ein neuer Übergang startet, während einer aktiv ist

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

// Read potentiometer value with tolerance
int LEDControl::readPotValueWithTolerance(int currentValue, int tolerance) {
    if (abs(currentValue - lastPotValue) > tolerance) {
        lastPotValue = currentValue; // Aktualisieren bei signifikanter Änderung
    }
    return lastPotValue;
}

void LEDControl::runTask() {
    const int frameDelay = 16; // ~30 FPS
    while (true) {
        if (lightState) {
            float temp = sensorHandler.getTemperature();
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
        uint32_t targetColor = getColorForTemperature(temperature);

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
            uint32_t newColor = interpolateColor(currentTransition.startColor, currentTransition.endColor, easedProgress);

            if (newColor != currentColor) { // Update nur bei Änderungen
                currentColor = newColor;

                for (int i = 0; i < strip.numPixels(); i++) {
                    strip.setPixelColor(i, currentColor);
                }
                strip.show();
            }
        }

        xSemaphoreGive(ledMutex);
    }
}

void LEDControl::setProgress(int progress, int totalSteps) {
    if (lightState && xSemaphoreTake(ledMutex, portMAX_DELAY)) {
        strip.clear(); // Alle LEDs löschen

        int ledsToLight = map(progress, 0, totalSteps, 0, strip.numPixels());
        for (int i = 0; i < ledsToLight; i++) {
            strip.setPixelColor(i, strip.Color(255, 0, 255)); // Magenta
        }

        strip.show();
        xSemaphoreGive(ledMutex);
    }
}

void LEDControl::fadeOut(unsigned long duration, int targetFPS) {
    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
        unsigned long fadeStart = millis();
        int frameDelay = 1000 / targetFPS;

        while (millis() - fadeStart < duration) {
            int elapsed = millis() - fadeStart;
            int brightness = map(elapsed, 0, duration, 255, 0); // Helligkeit reduzieren
            strip.setBrightness(brightness);
            strip.show();
            delay(frameDelay);
        }

        strip.clear();
        strip.show();

        lightState = false; // LEDs sind aus
        xSemaphoreGive(ledMutex);
    }
}

// Set transition speed for color changes
void LEDControl::setTransitionSpeed(float speed) {
    transitionSpeed = speed;
}

// Setzt die Helligkeit der LEDs
void LEDControl::setBrightness(int brightness) {
    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) { // Zugriff sichern
        currentBrightness = brightness; // Helligkeit speichern
        strip.setBrightness(brightness); // Helligkeit anwenden
        if (lightState) {
            strip.show(); // Änderungen anzeigen, wenn LEDs eingeschaltet sind
        }
        xSemaphoreGive(ledMutex); // Zugriff freigeben
    }
}
// Turn LEDs on
void LEDControl::turnOn() {
    if (!lightState) { // Nur einschalten, wenn LEDs aus sind
        lightState = true;
        strip.setBrightness(currentBrightness > 0 ? currentBrightness : 255); // Letzte oder Standardhelligkeit
        for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, currentColor); // Letzte Farbe
        }
        strip.show();
    }
}

// Turn LEDs off
void LEDControl::turnOff() {
    if (lightState) { // Nur ausschalten, wenn LEDs an sind
        lightState = false;
        currentBrightness = strip.getBrightness(); // Aktuelle Helligkeit speichern
        strip.clear();
        strip.show();
    }
}

// Check if LEDs are on
bool LEDControl::isOn() const {
    return lightState;
}

// Map temperature to color
uint32_t LEDControl::getColorForTemperature(float temp) {
    // Temperaturbereich auf 18°C - 28°C beschränken
    float ratio = constrain((temp - 18.0) / (28.0 - 18.0), 0.0, 1.0);

    // Farben interpolieren: Magenta (#FF00FF) → Dunkelblau (#000088)
    uint8_t r = (1.0 - ratio) * 255;   // Rot nimmt ab
    uint8_t g = 0;                    // Grün bleibt konstant
    uint8_t b = ratio * 136 + (1.0 - ratio) * 255; // Blau nimmt zu (136 bis 255)

    return strip.Color(r, g, b);
}

// Interpolate between two colors
uint32_t LEDControl::interpolateColor(uint32_t color1, uint32_t color2, float ratio) {
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = round(r1 + (r2 - r1) * ratio);
    uint8_t g = round(g1 + (g2 - g1) * ratio);
    uint8_t b = round(b1 + (b2 - b1) * ratio);

    return (r << 16) | (g << 8) | b;
}

float LEDControl::easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
}