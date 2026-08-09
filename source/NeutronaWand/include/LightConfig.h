/**
 *   GPStar Neutrona Wand
 *   Copyright (C) 2023-2026 GPStar Technologies <contact@gpstartechnologies.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

// Suppress FastLED warnings
#define FASTLED_INTERNAL

// Include the intended LED driver first: FastLED
#include <FastLED.h>

// Include the generalized Lighting library
#include <Lighting.h>

// ============================================================================
// LED CHAIN IDENTIFIERS
// ============================================================================

/*
 * Addressable LED Chains
 */
enum LED_CHAIN {
  CHAIN_BARREL = 0, // Barrel LEDs Only
  CHAIN_VENT = 1 // Vent Lights
};

// ============================================================================
// LOCAL LIGHTING VARIABLES
// ============================================================================

/*
 * Counts for segments of special LED chains
 * Note that these are in the expected physical order in the chain
 */
#define BARREL_LEDS_MAX 50 // The maximum number of barrel LEDs supported (GPStar Neutrona Barrel is 48 + 2 Strobe Tips).
#define VENT_LED_COUNT 2 // The maximum number of LEDs for the vent lights. Main vent + top Clip Lite.

/*
 * Pin for Addressable LEDs
 * Assumes WS2812B addressable LEDs (NeoPixel compatible)
 */
#ifdef ESP32
  #define BARREL_LED_PIN 41 // Data pin for the addressable LEDs in the barrel.
  #define TOP_LED_PIN 42 // Data pin for the addressable LEDs to the vent light.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#else
  #define BARREL_LED_PIN 10 // Data pin for the addressable LEDs in the barrel.
  #define TOP_LED_PIN 12 // Data pin for the addressable LEDs to the vent light, and top blinking light when RGB vent light is disabled.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#endif
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for optimal effect

/*
 * Delay for the LED driver to update the addressable LEDs.
 */
#define LED_DRIVER_UPDATE_MS 3
uint8_t i_led_update_delay = LED_DRIVER_UPDATE_MS;
millisDelay ms_led_driver;

/*
 * The Hasbro Neutrona Wand has 5 LEDs. 0 = Base, 4 = tip. These are addressable with a single pin and are GRB colour order.
 * Support for up to 50 LEDs from the GPStar Neutrona Barrel (body of 48 + 2 strobe tips which are RGB colour order).
 */
// Array of LEDs on the GPStar Neutrona Barrel. LEDs 36 and 37 are the very tips and will not be in this array.
const uint8_t gpstar_barrel[48] PROGMEM = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38};

// Array of LEDs on the GPStar Neutrona Barrel II. LED 36 is the very tip which will not be in this array. There is also one less tip LED due to the inclusion of an IR LED.
const uint8_t gpstar_barrel_ii[48] PROGMEM = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37};

// Array of LEDs on the Frutto Technology Neutrona Barrel. LED 12 is the very tip which will not be in this array.
const uint8_t frutto_barrel[48] PROGMEM = {0, 25, 24, 48, 1, 26, 23, 47, 2, 27, 22, 46, 3, 28, 21, 45, 4, 29, 20, 44, 5, 30, 19, 43, 6, 31, 18, 42, 7, 32, 17, 41, 8, 33, 16, 40, 9, 34, 15, 39, 10, 35, 14, 38, 11, 36, 13, 37};

/*
 * RGB Vent Light Control
 */
const uint16_t i_vent_light_update_interval = 150; // FastLED update interval specifically for the top/vent LEDs.
bool b_vent_lights_changed = false; // Check for whether there was actually a change to prevent superfluous calls to showLeds().

// ============================================================================
// LIGHTING LIBRARY CONFIGURATION & INITIALIZATION
// ============================================================================

/**
 * LightingManager - Abstraction Layer for LED Driver Operations
 *
 * PURPOSE:
 * This class provides a driver-agnostic interface for all LED operations.
 * Instead of directly calling FastLED functions throughout the codebase,
 * all LED control flows through this manager. This design allows us to
 * swap the underlying LED driver without touching application logic.
 *
 * PATTERN:
 * LightingManager uses the SINGLETON pattern. There is only ONE
 * instance of this class for the entire program. Access it via:
 *   LightingManager::getInstance()
 *
 * WHY SINGLETON:
 * LED hardware is a system-wide resource. Having one centralized manager
 * ensures consistent state, prevents multiple initialization calls, and
 * provides a single point of control for all LED operations.
 *
 * INTERFACE:
 * - initializeDriver() — Sets up the driver library and hardware pins
 * - getColorRGB/GRB/GBR() — Converts color enums to CRGB values
 * - show() — Updates physical LEDs with current buffer state
 * - lightsOff() — Blanks all LEDs
 * - setBrightness() — Controls global brightness
 * - getLEDs() — Returns pointer to LED array for direct manipulation
 *
 * DRIVER ABSTRACTION:
 * The methods here wrap driver-specific calls. Whenever we need to
 * support a different LED library, we only modify this class, not the
 * caller code. This keeps the rest of the application clean and portable.
 */
class LightingManager {
private:
  static LightingManager* instance;
  Lighting lightingLib;
  CRGB barrelLEDs[BARREL_LEDS_MAX];
  CRGB ventLEDs[VENT_LED_COUNT];

  // Private constructor - called only once by getInstance()
  LightingManager() : lightingLib(1) {
    // Initialize with 1 device (PRIMARY_LED)
  }

public:
  // Singleton instance
  static LightingManager& getInstance() {
    if(instance == nullptr) {
      instance = new LightingManager();
    }
    return *instance;
  }

  // Initialize LED driver
  // Sets up addressable LED communication and default brightness for both chains
  void initializeDriver() {
    FastLED.addLeds<NEOPIXEL, BARREL_LED_PIN>(barrelLEDs, BARREL_LEDS_MAX).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<NEOPIXEL, RGB_VENT_PIN>(ventLEDs, VENT_LED_COUNT).setCorrection(TypicalLEDStrip);
    FastLED.setMaxRefreshRate(0); // Disable FastLED's blocking 2.5ms delay.
    FastLED.setBrightness(DEVICE_MAX_BRIGHTNESS);
    FastLED.show(); // Update all addressable LEDs to prevent stale LED states.
  }

  // Get color as RGB based on LED chain and color enum
  CRGB getColorRGB(LED_CHAIN chain, uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV((ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    return CRGB(rgb.r, rgb.g, rgb.b);
  }

  // Get color as GRB based on LED chain and color enum
  CRGB getColorGRB(LED_CHAIN chain, uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV((ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GRB: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.r, rgb.b }
    return CRGB(rgb.g, rgb.r, rgb.b);
  }

  // Get color as GBR based on LED chain and color enum
  CRGB getColorGBR(LED_CHAIN chain, uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV((ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GBR: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.b, rgb.r }
    return CRGB(rgb.g, rgb.b, rgb.r);
  }

  // Update LED display
  void show() {
    FastLED.show(); // Pass through to the LED driver library to update LED states.
  }

  // Turn off LEDs on specified chain
  void lightsOff(LED_CHAIN chain = CHAIN_BARREL) {
    switch(chain) {
      case CHAIN_BARREL:
        fill_solid(barrelLEDs, BARREL_LEDS_MAX, CRGB::Black); // Set all to black (off).
      break;

      case CHAIN_VENT:
        fill_solid(ventLEDs, VENT_LED_COUNT, CRGB::Black); // Set all to black (off).
      break;
    }
  }

  // Get a pointer to the LED array for specified chain (for palette rendering and direct access)
  CRGB* getLEDs(LED_CHAIN chain = CHAIN_BARREL) {
    switch(chain) {
      case CHAIN_VENT:
        return ventLEDs;
      break;

      case CHAIN_BARREL:
      default:
        return barrelLEDs;
      break;
    }
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
  }
};

/**
 * SINGLETON PATTERN: Static member variable initialization
 *
 * This line MUST exist outside the class definition for any static member.
 * It allocates memory for the single instance pointer and initializes it to nullptr.
 *
 * The actual LightingManager object is NOT created here—it's created lazily on
 * the FIRST call to getInstance(), which checks if instance is nullptr, creates it if
 * needed, then returns a reference to it. Subsequent calls return the same instance.
 *
 * This ensures only ONE LightingManager exists for the entire program.
 */
LightingManager* LightingManager::instance = nullptr;
