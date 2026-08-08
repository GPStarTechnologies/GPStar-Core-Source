/**
 *   GPStar Single-Shot Blaster
 *   Copyright (C) 2024-2026 Michael Rajotte <contact@gpstartechnologies.com
 *                    & Dustin Grau <dustin.grau@gmail.com>
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
// LOCAL LIGHTING VARIABLES
// ============================================================================

/*
 * Counts for segments of special LED chains
 * Note that these are in the expected physical order in the chain
 */
#define CYCLOTRON_LED_COUNT 7 // GPStar 7-LED Jewel
#define BARREL_LED_COUNT 7 // GPStar 7-LED Jewel

/*
 * Pin for Addressable LEDs
 * Assumes WS2812B addressable LEDs (NeoPixel compatible)
 */
#ifdef ESP32
  #define SYSTEM_LED_PIN 41
  #define SYSTEM_LED_COUNT (CYCLOTRON_LED_COUNT + BARREL_LED_COUNT)
  #define TOP_LED_PIN 42 // RGB Vent light only for ESP32.
  #define VENT_LED_COUNT 2 // The maximum number of LEDs for the vent lights. Main vent + top Cliplite.
#else
  #define SYSTEM_LED_PIN 10
  #define SYSTEM_LED_COUNT (CYCLOTRON_LED_COUNT + BARREL_LED_COUNT)
  #define VENT_LED_COUNT 0 // Not applicable for ATMega devices.
#endif
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for optimal effect

/*
 * Addressable LED Chains
 */
// CHAIN IDENTIFIERS (required for LocalLightingManager to know which buffer you're accessing)
enum LED_CHAIN {
  CHAIN_SYSTEM = 0, // Barrel + Cyclotron
  CHAIN_VENT = 1 // Top Vent (ESP32 Only)
};

/*
 * Delay for LED driver to update the addressable LEDs.
 */
#define LED_DRIVER_UPDATE_MS 3
uint8_t i_led_update_delay = LED_DRIVER_UPDATE_MS;
millisDelay ms_led_driver;

/*
 * Pre-calculated indices for segments of the system LED chain (Cyclotron + Barrel)
 */
const uint8_t i_barrel_led = 6; // This will be the index of the light (#7), not the count
const uint8_t i_num_barrel_leds = BARREL_LED_COUNT; // This will be the number of barrel LEDs
const uint8_t i_num_cyclotron_leds = CYCLOTRON_LED_COUNT; // This will be the number of cyclotron LEDs
const uint8_t i_cyclotron_led_start = i_num_barrel_leds; // The first element (index) for the cyclotron.

/*
 * RGB Vent Light Control (ESP32 Only)
 */
const uint16_t i_vent_light_update_interval = 150; // FastLED update interval specifically for the top/vent LEDs.
bool b_vent_lights_changed = false; // Check for whether there was actually a change to prevent superfluous calls to showLeds().

// ============================================================================
// LIGHTING LIBRARY CONFIGURATION & INITIALIZATION
// ============================================================================

/**
 * LocalLightingManager - Abstraction Layer for LED Driver Operations
 *
 * PURPOSE:
 * This class provides a driver-agnostic interface for all LED operations.
 * Instead of directly calling FastLED functions throughout the codebase,
 * all LED control flows through this manager. This design allows us to
 * swap the underlying LED driver without touching application logic.
 *
 * PATTERN:
 * LocalLightingManager uses the SINGLETON pattern. There is only ONE
 * instance of this class for the entire program. Access it via:
 *   LocalLightingManager::getInstance()
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
class LocalLightingManager {
private:
  static LocalLightingManager* instance;
  Lighting lightingLib;
  CRGB systemLEDs[SYSTEM_LED_COUNT];
  CRGB ventLEDs[VENT_LED_COUNT];

  // Private constructor - called only once by getInstance()
  LocalLightingManager() : lightingLib(1) {
    // Initialize with 1 device (PRIMARY_LED)
  }

public:
  // Singleton instance
  static LocalLightingManager& getInstance() {
    if(instance == nullptr) {
      instance = new LocalLightingManager();
    }
    return *instance;
  }

  // Initialize LED driver
  // Sets up addressable LED communication and default brightness for both chains
  void initializeDriver() {
    FastLED.addLeds<NEOPIXEL, SYSTEM_LED_PIN>(systemLEDs, SYSTEM_LED_COUNT).setCorrection(TypicalLEDStrip);
    #ifdef ESP32
      FastLED.addLeds<NEOPIXEL, TOP_LED_PIN>(ventLEDs, VENT_LED_COUNT).setCorrection(TypicalLEDStrip);
    #endif
    FastLED.setMaxRefreshRate(0); // Disable FastLED's blocking 2.5ms delay.
    FastLED.setBrightness(DEVICE_MAX_BRIGHTNESS);
    FastLED.show(); // Update all addressable LEDs to prevent stale LED states.
  }

  // Get color as RGB based on LED chain and color enum
  CRGB getColorRGB(LED_CHAIN chain, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    return CRGB(rgb.r, rgb.g, rgb.b);
  }

  // Get color as GRB based on LED chain and color enum
  CRGB getColorGRB(LED_CHAIN chain, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GRB: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.r, rgb.b }
    return CRGB(rgb.g, rgb.r, rgb.b);
  }

  // Get color as GBR based on LED chain and color enum
  CRGB getColorGBR(LED_CHAIN chain, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GBR: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.b, rgb.r }
    return CRGB(rgb.g, rgb.b, rgb.r);
  }

  // Update LED display
  void show() {
    FastLED.show(); // Pass through to the LED driver library to update LED states.
  }

  // Turn off LEDs on specified chain
  void lightsOff(LED_CHAIN chain = CHAIN_SYSTEM) {
    if(chain == CHAIN_SYSTEM) {
      fill_solid(systemLEDs, SYSTEM_LED_COUNT, CRGB::Black); // Set all to black (off).
    }
    else if(chain == CHAIN_VENT && VENT_LED_COUNT > 0) {
      fill_solid(ventLEDs, VENT_LED_COUNT, CRGB::Black); // Set all to black (off).
    }
  }

  // Get a pointer to the LED array for specified chain (for palette rendering and direct access)
  CRGB* getLEDs(LED_CHAIN chain = CHAIN_SYSTEM) {
    if(chain == CHAIN_SYSTEM) {
      return systemLEDs;
    }
    else if(chain == CHAIN_VENT) {
      return ventLEDs;
    }
    return systemLEDs; // Default fallback
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
 * The actual LocalLightingManager object is NOT created here—it's created lazily on
 * the FIRST call to getInstance(), which checks if instance is nullptr, creates it if
 * needed, then returns a reference to it. Subsequent calls return the same instance.
 *
 * This ensures only ONE LocalLightingManager exists for the entire program.
 */
LocalLightingManager* LocalLightingManager::instance = nullptr;
