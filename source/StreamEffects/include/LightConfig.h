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
#define DEVICE_SLOTS 1 // Number of device slots for the Lighting library
#define DEVICE_REFRESH_MS 8 // Refresh rate for the addressable LEDs (in milliseconds)
#define DEVICE_MAX_LEDS 500 // Set a hard max for allocating the array of LEDs
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect
uint16_t i_num_leds = 250; // Default is 50 LEDs per meter, with a length of 5 meters (eg. 250)

/*
 * Addressable LED Devices
 */
enum STRAND_LED : uint8_t {
  PRIMARY_LED = 0
};

/*
 * LED colour order type for device (stored in Preferences/NVS)
 * Defaults to COLOR_ORDER_RGB for the type recommended for the build: https://a.co/d/dlDyCkz
 * NOTE: These enum values will be mapped via the LightingManager to the proper ColorOrder ENUM.
 */
enum LED_COLOR_ORDER : uint8_t {
  COLOR_ORDER_RGB = 1,
  COLOR_ORDER_GRB = 2,
  COLOR_ORDER_GBR = 3
};
LED_COLOR_ORDER LED_COLOR_TYPE = COLOR_ORDER_RGB;

/*
 * LED Animation Control Settings (stored in Preferences/NVS)
 */
uint8_t i_max_brightness = 255; // Maximum brightness (0-255), default 100%
bool b_invert_direction = false; // Invert animation direction, default false
uint8_t i_default_wand_power = 1; // Default wandPower level (1-5), default 1 (for testing)

/*
 * Define Color Options & Timers
 */
LED_Palette16 paletteWhite;
LED_Palette16 paletteProton;
LED_Palette16 paletteSlime;
LED_Palette16 paletteStasis;
LED_Palette16 paletteMeson;
LED_Palette16 paletteSpectral;
LED_Palette16 paletteHalloween;
LED_Palette16 paletteChristmas;
LED_Palette16 paletteBrass;
LED_Palette16 cp_StreamPalette; // Current colour palette in use.
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
  uint8_t currentDeviceSlot; // Track device slot for an instance.

  // Private constructor - called only once by getInstance()
  // Initializes the Lighting library as lightingLib with 1 device slot,
  // and initializes the Adafruit_NeoPixel object as a variable "pixels".
  LightingManager() :
    lightingLib(DEVICE_SLOTS, DEVICE_REFRESH_MS),
    pixels(DEVICE_MAX_LEDS, DEVICE_LED_PIN, NEO_RGB + NEO_KHZ800),
    currentDeviceSlot(0) {}

  // Helper: Map user preference color order values (1,2,3) to Lighting ColorOrder enum (0,1,2)
  // Handles conversion from device-specific enum to Lighting library enum
  ColorOrder mapColorOrder(uint8_t userPref) const {
    switch(userPref) {
      case 1:  // COLOR_ORDER_RGB → ORDER_RGB
        return ORDER_RGB;
      case 2:  // COLOR_ORDER_GRB → ORDER_GRB
        return ORDER_GRB;
      case 3:  // COLOR_ORDER_GBR → ORDER_GBR
        return ORDER_GBR;
      default: // Fallback to RGB
        return ORDER_RGB;
    }
  }

public:
  // Singleton instance
  static LightingManager& getInstance(uint8_t deviceSlot = 0) {
    if(instance == nullptr) {
      instance = new LightingManager();
    }
    instance->currentDeviceSlot = deviceSlot; // Set context for this call
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
  LED_RGB getColorRGB(uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(currentDeviceSlot, (ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    return LED_RGB(rgb.r, rgb.g, rgb.b);
  }

  LED_RGB getColorGRB(uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(currentDeviceSlot, (ColorID)colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV((ColorID)colorEnum, brightness);
    }
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GRB: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.r, rgb.b }
    return LED_RGB(rgb.g, rgb.r, rgb.b);
  }

  LED_RGB getColorGBR(uint8_t colorEnum, uint8_t brightness = 255) {
    LED_HSV hsv;
    if(isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(currentDeviceSlot, (ColorID)colorEnum, brightness);
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

  // Set a pixel color by ColorID and automatically apply stored color order
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    if(index >= 0 && index < pixels.numPixels()) {
      // Get color as HSV
      LED_HSV hsv;
      if(isColorDynamic(colorEnum)) {
        hsv = lightingLib.getDynamicColorHSV(currentDeviceSlot, colorEnum, brightness);
      } else {
        hsv = lightingLib.getColorHSV(colorEnum, brightness);
      }

      // Convert to RGB
      LED_RGB rgb = Lighting::hsv2rgb(hsv);

      // Apply stored color order for this device
      ColorOrder order = lightingLib.getColorOrder(currentDeviceSlot);
      LED_RGB ordered = Lighting::applyColorOrder(rgb, order);

      // Set the pixel
      pixels.setPixelColor(index, pixels.Color(ordered.r, ordered.g, ordered.b));
    }
  }

  // Set a pixel with raw RGB color (color order already applied)
  void setPixelColorRGB(uint16_t index, const LED_RGB &rgb) {
    if(index >= 0 && index < pixels.numPixels()) {
      pixels.setPixelColor(index, pixels.Color(rgb.r, rgb.g, rgb.b));
    }
  }

  // Get an interpolated palette color with smooth animation and speed control
  LED_RGB getPaletteColor(const LED_Palette16& palette, float speedMultiplier = 1.0, uint8_t positionOffset = 0, uint8_t brightness = 255, bool reverse = false) {
    return lightingLib.getPaletteColor(currentDeviceSlot, palette, speedMultiplier, positionOffset, brightness, reverse);
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
  void setCustomColorHSV(const LED_HSV &hsv) {
    lightingLib.setCustomColorHSV(hsv, currentDeviceSlot);
  }

  // Set color order with automatic enum mapping
  // Converts device color order values to Lighting library enum
  void setColorOrder(uint8_t deviceSlot, uint8_t userPrefValue) {
    ColorOrder mappedOrder = mapColorOrder(userPrefValue);
    lightingLib.setColorOrder(deviceSlot, mappedOrder);
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
LED_Palette16 createPaletteProton() {
  return {{
    C_AQUA, C_RED, C_RED, C_ORANGE, C_ORANGE, C_RED4, C_RED4, C_BLACK,
    C_AQUA, C_RED, C_RED, C_ORANGE, C_ORANGE, C_RED4, C_RED4, C_BLACK
  }};
}

// Create Slime stream palette: Green, LimeGreen, Black
LED_Palette16 createPaletteSlime() {
  return {{
    C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_CHARTREUSE, C_CHARTREUSE, C_BLACK, C_BLACK,
    C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_CHARTREUSE, C_CHARTREUSE, C_BLACK, C_BLACK
  }};
}

// Create Stasis stream palette: Blue, Indigo, Black
LED_Palette16 createPaletteStasis() {
  return {{
    C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_NAVY_BLUE, C_NAVY_BLUE, C_BLACK, C_BLACK,
    C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_NAVY_BLUE, C_NAVY_BLUE, C_BLACK, C_BLACK
  }};
}

// Create Meson stream palette: Yellow, Orange, Black
LED_Palette16 createPaletteMeson() {
  return {{
    C_YELLOW, C_YELLOW, C_YELLOW, C_YELLOW, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK,
    C_YELLOW, C_YELLOW, C_YELLOW, C_YELLOW, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK
  }};
}

// Create Spectral stream palette: Full rainbow cycle
LED_Palette16 createPaletteSpectral() {
  return {{
    C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_BLUE, C_NAVY_BLUE, C_PURPLE, C_BLACK,
    C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_BLUE, C_NAVY_BLUE, C_PURPLE, C_BLACK
  }};
}

// Create Halloween palette: Orange and Purple with Black
LED_Palette16 createPaletteHalloween() {
  return {{
    C_ORANGE, C_ORANGE, C_ORANGE, C_ORANGE, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK,
    C_BLACK, C_BLACK, C_PURPLE, C_PURPLE, C_PURPLE, C_PURPLE, C_PURPLE, C_PURPLE
  }};
}

// Create Christmas palette: Red and Green with Black
LED_Palette16 createPaletteChristmas() {
  return {{
    C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_BLACK, C_BLACK,
    C_BLACK, C_BLACK, C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_GREEN
  }};
}

// Create Brass palette: Chartreuse and Orange with Black
LED_Palette16 createPaletteBrass() {
  return {{
    C_CHARTREUSE, C_CHARTREUSE, C_CHARTREUSE, C_CHARTREUSE, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK,
    C_CHARTREUSE, C_CHARTREUSE, C_CHARTREUSE, C_CHARTREUSE, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK
  }};
}

// Create White palette: GhostWhite and Gainsboro with Black
// Create White palette: White with Black
LED_Palette16 createPaletteWhite() {
  return {{
    C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_BLACK, C_BLACK, C_BLACK, C_BLACK,
    C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_BLACK, C_BLACK, C_BLACK, C_BLACK
  }};
}

// Initialization of all palettes (at startup).
void initializePalettes() {
  paletteProton = createPaletteProton();
  paletteSlime = createPaletteSlime();
  paletteStasis = createPaletteStasis();
  paletteMeson = createPaletteMeson();
  paletteSpectral = createPaletteSpectral();
  paletteHalloween = createPaletteHalloween();
  paletteChristmas = createPaletteChristmas();
  paletteBrass = createPaletteBrass();
  paletteWhite = createPaletteWhite();
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
