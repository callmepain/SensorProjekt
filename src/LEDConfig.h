#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include <FastLED.h>

// LED-Pins
#define LED_PIN 4      // Pin für die LED-Datenleitung
#define NUM_LEDS 15    // Anzahl der LEDs
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B

extern CRGB leds[NUM_LEDS];

#endif
