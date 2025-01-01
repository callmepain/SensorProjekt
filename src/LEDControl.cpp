#include "LEDControl.h"
#include "ConfigManager.h"

LEDControl::LEDControl(Adafruit_NeoPixel& strip)
    : strip(strip), transitionSpeed(0.1f), currentColor(0), currentBrightness(255) {
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

// Update LEDs
void LEDControl::update(float temperature) {
    if (!lightState) return;

    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
        uint32_t targetColor = getColorForTemperature(temperature);

        // Ziel-Farbe in RGB-Werte aufteilen
        uint8_t targetRed = (targetColor >> 16) & 0xFF;
        uint8_t targetGreen = (targetColor >> 8) & 0xFF;
        uint8_t targetBlue = targetColor & 0xFF;

        // Aktuelle Farbe in RGB-Werte aufteilen
        uint8_t currentRed = (currentColor >> 16) & 0xFF;
        uint8_t currentGreen = (currentColor >> 8) & 0xFF;
        uint8_t currentBlue = currentColor & 0xFF;

        // Easing-Faktor berechnen
        static float transitionProgress = 0.0f;
        transitionProgress += transitionSpeed;
        if (transitionProgress > 1.0f) transitionProgress = 1.0f;

        float easedProgress = easeInOutQuad(transitionProgress);

        // Farbwerte mit Easing interpolieren
        uint8_t newRed = currentRed + (targetRed - currentRed) * easedProgress;
        uint8_t newGreen = currentGreen + (targetGreen - currentGreen) * easedProgress;
        uint8_t newBlue = currentBlue + (targetBlue - currentBlue) * easedProgress;

        // Neue Farbe setzen
        currentColor = strip.Color(newRed, newGreen, newBlue);

        // LEDs aktualisieren
        for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, currentColor);
        }
        strip.show();

        // Zurücksetzen des Übergangs bei Abschluss
        if (transitionProgress >= 1.0f) {
            transitionProgress = 0.0f;
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
    if (temp < 22.0) {
        return strip.Color(0, 0, 255); // Blue
    } else if (temp < 25.0) {
        float ratio = (temp - 22.0) / (25.0 - 22.0);
        uint8_t r = ratio * 0;
        uint8_t g = ratio * 255;
        uint8_t b = 255 - ratio * 255;
        return strip.Color(r, g, b); // Blue to Green
    } else if (temp < 28.0) {
        float ratio = (temp - 25.0) / (28.0 - 25.0);
        uint8_t r = ratio * 255;
        uint8_t g = 255 - ratio * 255;
        uint8_t b = 0;
        return strip.Color(r, g, b); // Green to Yellow
    } else {
        return strip.Color(255, 0, 0); // Red
    }
}

// Interpolate between two colors
uint32_t LEDControl::interpolateColor(uint32_t color1, uint32_t color2, float ratio) {
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = r1 + ratio * (r2 - r1);
    uint8_t g = g1 + ratio * (g2 - g1);
    uint8_t b = b1 + ratio * (b2 - b1);

    return (r << 16) | (g << 8) | b;
}

float LEDControl::easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
}