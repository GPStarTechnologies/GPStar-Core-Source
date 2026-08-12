/**
 *   GPStar Stream Effects - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024-2026 Dustin Grau <dustin.grau@gmail.com>
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

// Include the intended LED driver first: Adafruit Neopixel
#include <Adafruit_NeoPixel.h>

// Include the generalized Lighting library
#include <Lighting.h>

// ============================================================================
// LOCAL LIGHTING VARIABLES
// ============================================================================

/*
 * Pin for Addressable LEDs
 * Assumes 50 LEDs per meter using default lighting: https://a.co/d/dlDyCkz
 */
#define DEVICE_LED_PIN 4
#define DEVICE_MAX_LEDS 500 // Set a hard max for allocating the array of LEDs
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect
uint16_t i_num_leds = 250; // Default is 50 LEDs per meter, with a length of 5 meters (eg. 250)

/*
 * Addressable LED Devices
 */
enum device : uint8_t {
  PRIMARY_LED = 0
};

/*
 * LED colour order type for device
 * Defaults to RGB for the type recommended for the build: https://a.co/d/dlDyCkz
 */
enum LED_COLOR_TYPES : uint8_t {
  LED_RGB = 1,
  LED_GRB = 2,
  LED_GBR = 3
};
LED_COLOR_TYPES LED_COLOR_TYPE = LED_RGB;

/*
 * Define Color Options & Timers
 * @todo: Redefine the FastLED CRGBPalette16 with a new custom implementation.
 */
CRGBPalette16 paletteWhite;
CRGBPalette16 paletteProton;
CRGBPalette16 paletteSlime;
CRGBPalette16 paletteStasis;
CRGBPalette16 paletteMeson;
CRGBPalette16 paletteSpectral;
CRGBPalette16 paletteHalloween;
CRGBPalette16 paletteChristmas;
CRGBPalette16 paletteBrass;
CRGBPalette16 cp_StreamPalette; // Current colour palette in use.
static const uint8_t i_palette_count = 9; // Total number of palettes available.
static const uint16_t i_selftest_interval = 2000; // 2 seconds between palette changes.
millisDelay ms_selftest_cycle; // Timer for self-test cycling using an interval.
uint8_t i_selftest_palette = 0; // Current palette index for cycling in self-test.

extern uint8_t i_spectral_custom_colour;
extern uint8_t i_spectral_custom_saturation;

// ============================================================================
// LIGHTING LIBRARY CONFIGURATION & INITIALIZATION
// ============================================================================

/**
 * LightingManager - Abstraction Layer for LED Driver Operations
 *
 * PURPOSE:
 * This class provides a driver-agnostic interface for all LED operations.
 * Instead of directly calling LED driver functions throughout the codebase,
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
 * - setPixelColor(index, CRGB) — Set a single LED to a color
 * - getPixelColor(index) — Read a single LED's current color as CRGB
 * - unpackColor(uint32_t) — Convert packed color integer to CRGB
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
  Adafruit_NeoPixel pixels;

  // Private constructor - called only once by getInstance()
  // Initializes the Lighting library as lightingLib with 1 device slot,
  // and initializes the Adafruit_NeoPixel object as a variable "pixels".
  LightingManager() :
    lightingLib(1),
    pixels(DEVICE_MAX_LEDS, DEVICE_LED_PIN, NEO_RGB + NEO_KHZ800) {
    // Initialize with 1 device (up to DEVICE_MAX_LEDS, but limited by i_num_leds)
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
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    pixels.begin();
    pixels.setBrightness(DEVICE_MAX_BRIGHTNESS);
    pixels.show();
  }

  // Get color as RGB based on device and color enum
  LED_RGB getColorRGB(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(device, (ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    return LED_RGB(rgb.r, rgb.g, rgb.b);
  }

  LED_RGB getColorGRB(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(device, (ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GRB: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.r, rgb.b }
    return LED_RGB(rgb.g, rgb.r, rgb.b);
  }

  LED_RGB getColorGBR(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(device, (ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GBR: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.b, rgb.r }
    return LED_RGB(rgb.g, rgb.b, rgb.r);
  }

  // Update LED display
  void show() {
    pixels.show(); // Pass through to the LED driver library to update LED states.
  }

  // Turn off all LEDs
  void lightsOff() {
    pixels.clear(); // Set all to black (off).
  }

  // Helper: Convert packed uint32_t color to LED_RGB components
  LED_RGB unpackColor(uint32_t packedColor) {
    uint8_t r = (packedColor >> 16) & 0xFF;
    uint8_t g = (packedColor >> 8) & 0xFF;
    uint8_t b = packedColor & 0xFF;
    return LED_RGB(r, g, b);
  }

  // Set a pixel color by index using LED_RGB
  void setPixelColor(uint16_t index, LED_RGB color) {
    if(index >= 0 && index < pixels.numPixels()) {
      // Convert LED_RGB to uint32_t packed color
      pixels.setPixelColor(index, pixels.Color(color.r, color.g, color.b));
    }
  }

  // Returns a pixel color using LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    if(index >= 0 && index < pixels.numPixels()) {
      return unpackColor(pixels.getPixelColor(index));
    }
    return LED_RGB::BLACK; // Return black if index is out of bounds.
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    pixels.setBrightness(brightness);
  }

  // Set custom color HSV values in the Lighting library
  void setCustomColorHSV(const LED_HSV &hsv, uint8_t deviceSlot = 0) {
    lightingLib.setCustomColorHSV(hsv, deviceSlot);
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
