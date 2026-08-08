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
// LOCAL LIGHTING VARIABLES
// ============================================================================

/*
 * Pin for Addressable LEDs
 * Assumes WS2812B addressable LEDs (NeoPixel compatible)
 */
#define DEVICE_LED_PIN 41 // Data pin for the barrel addressable LEDs (BARREL_LED_PIN)
#define DEVICE_MAX_LEDS 52 // Maximum LEDs (50 barrel + 2 vent)
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for optimal effect

/*
 * Delay for LED driver to update the addressable LEDs.
 */
#define LED_DRIVER_UPDATE_MS 3
uint8_t i_led_update_delay = LED_DRIVER_UPDATE_MS;
millisDelay ms_led_driver;

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
  CRGB deviceLEDs[DEVICE_MAX_LEDS];

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
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    FastLED.addLeds<NEOPIXEL, DEVICE_LED_PIN>(deviceLEDs, DEVICE_MAX_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setMaxRefreshRate(0); // Disable FastLED's blocking 2.5ms delay.
    FastLED.setBrightness(DEVICE_MAX_BRIGHTNESS);
    FastLED.show(); // Update all addressable LEDs to prevent stale LED states.
  }

  // Get color as RGB based on device and color enum
  CRGB getColorRGB(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    return CRGB(rgb.r, rgb.g, rgb.b);
  }

  CRGB getColorGRB(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GRB: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.r, rgb.b }
    return CRGB(rgb.g, rgb.r, rgb.b);
  }

  CRGB getColorGBR(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GBR: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.b, rgb.r }
    return CRGB(rgb.g, rgb.b, rgb.r);
  }

  // Update LED display
  void show() {
    FastLED.show(); // Pass through to the LED driver library to update LED states.
  }

  // Turn off all LEDs
  void lightsOff() {
    fill_solid(deviceLEDs, DEVICE_MAX_LEDS, CRGB::Black); // Set all to black (off).
  }

  // Get a pointer to the LED array (for palette rendering and direct access)
  CRGB* getLEDs() {
    return deviceLEDs;
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

// ============================================================================
// DEVICE-SPECIFIC CUSTOM COLORS (if any)
// ============================================================================
