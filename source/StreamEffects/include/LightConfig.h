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
enum LED_COLOR_ORDER : uint8_t {
  COLOR_ORDER_RGB = 1,
  COLOR_ORDER_GRB = 2,
  COLOR_ORDER_GBR = 3
};
LED_COLOR_ORDER LED_COLOR_TYPE = COLOR_ORDER_RGB;

/*
 * Define Color Options & Timers
 */
LED_RGB_Palette16 paletteWhite;
LED_RGB_Palette16 paletteProton;
LED_RGB_Palette16 paletteSlime;
LED_RGB_Palette16 paletteStasis;
LED_RGB_Palette16 paletteMeson;
LED_RGB_Palette16 paletteSpectral;
LED_RGB_Palette16 paletteHalloween;
LED_RGB_Palette16 paletteChristmas;
LED_RGB_Palette16 paletteBrass;
LED_RGB_Palette16 cp_StreamPalette; // Current colour palette in use.
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
    return LED_RGB_BLACK; // Return black if index is out of bounds.
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
 * PALETTE CREATION FUNCTIONS
 * 
 * These functions build LED_RGB_Palette16 palettes using consistent
 * color definitions from the Lighting library. Each palette maps to
 * a thematic set of colors for different stream modes.
 * 
 * All colors are derived from ColorID enum values, ensuring consistency
 * across the application. Each function gets a single LightingManager
 * reference to avoid redundant singleton lookups.
 */

// Create Proton stream palette: Cyan, Red, Orange, Maroon, Black
LED_RGB_Palette16 createPaletteProton() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Repeat pattern fills 16 colors: cyan, red, red, orange, orange, maroon, maroon, black (x2)
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_AQUA);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_RED4);      // Darker red for maroon effect
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_RED4);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_AQUA);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_RED4);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_RED4);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

// Create Slime stream palette: Green, LimeGreen, Black
LED_RGB_Palette16 createPaletteSlime() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: green (x4), lime green (x2), black (x2), repeated
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);  // Bright lime-like green
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

// Create Stasis stream palette: Blue, Indigo, Black
LED_RGB_Palette16 createPaletteStasis() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: blue (x4), indigo (x2), black (x2), repeated
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_NAVY_BLUE);   // Darker blue for indigo
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_NAVY_BLUE);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_NAVY_BLUE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_NAVY_BLUE);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

// Create Meson stream palette: Yellow, Orange, Black
LED_RGB_Palette16 createPaletteMeson() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: yellow (x4), orange (x2), black (x3), repeated
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

// Create Spectral stream palette: Full rainbow cycle
LED_RGB_Palette16 createPaletteSpectral() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: red, orange, yellow, green, blue, indigo, violet, black, repeated
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_NAVY_BLUE);   // Indigo
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_PURPLE);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_YELLOW);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_BLUE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_NAVY_BLUE);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_PURPLE);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

// Create Halloween palette: Orange and Purple with Black
LED_RGB_Palette16 createPaletteHalloween() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: orange (x4), black (x2), purple (x4), black (x2)
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_PURPLE);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_PURPLE);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_PURPLE);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_PURPLE);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  return palette;
}

// Create Christmas palette: Red and Green with Black
LED_RGB_Palette16 createPaletteChristmas() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: red (x4), black (x2), green (x4), black (x2)
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_GREEN);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_RED);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_RED);
  return palette;
}

// Create Brass palette: Chartreuse and Orange with Black
LED_RGB_Palette16 createPaletteBrass() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: chartreuse (x4), orange (x2), black (x2), repeated
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_CHARTREUSE);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_ORANGE);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

// Create White palette: GhostWhite and Gainsboro with Black
LED_RGB_Palette16 createPaletteWhite() {
  LED_RGB_Palette16 palette;
  auto& mgr = LightingManager::getInstance();
  // Pattern: warm white (x2), regular white (x2), black (x3), repeated
  palette.colors[0]  = mgr.getColorRGB(PRIMARY_LED, C_WARM_WHITE);
  palette.colors[1]  = mgr.getColorRGB(PRIMARY_LED, C_WARM_WHITE);
  palette.colors[2]  = mgr.getColorRGB(PRIMARY_LED, C_WHITE);
  palette.colors[3]  = mgr.getColorRGB(PRIMARY_LED, C_WHITE);
  palette.colors[4]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[5]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[6]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[7]  = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[8]  = mgr.getColorRGB(PRIMARY_LED, C_WARM_WHITE);
  palette.colors[9]  = mgr.getColorRGB(PRIMARY_LED, C_WARM_WHITE);
  palette.colors[10] = mgr.getColorRGB(PRIMARY_LED, C_WHITE);
  palette.colors[11] = mgr.getColorRGB(PRIMARY_LED, C_WHITE);
  palette.colors[12] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[13] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[14] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  palette.colors[15] = mgr.getColorRGB(PRIMARY_LED, C_BLACK);
  return palette;
}

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
