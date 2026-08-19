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

// Include the intended LED driver first: Adafruit NeoPixel
#include <Adafruit_NeoPixel.h>

// Include the generalized Lighting library
#include <Lighting.h>

// Include global palette definitions
#include <LightingPalettes.h>

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
 * LED Color Order Type (for device), intended to be stored in Preferences/NVS.
 * Defaults to COLOR_ORDER_GRB for the type recommended for the build: https://a.co/d/dlDyCkz
 * NOTE: These enum values will be mapped via the LightingManager to the proper ColorOrder ENUM.
 */
ColorOrder LED_COLOR_TYPE = ORDER_GRB;

/*
 * LED Animation Control Settings, intended to be stored in Preferences/NVS.
 */
uint8_t i_max_brightness = 255; // Maximum brightness (0-255), default 100%
bool b_invert_direction = false; // Invert animation direction, default false

/*
 * Define Color Options & Timers
 * Note: Global palette instances are available from LightingPalettes.h
 * (g_paletteWhite, g_paletteProton, g_paletteSlime, etc.)
 */
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
 * This class provides a driver-agnostic interface for direct LED operations.
 * Instead of calling driver functions throughout the codebase, all LED control
 * MUST flow through this manager. This design allows us to swap the underlying
 * LED driver without touching application logic. This manager should exist as
 * the only location where the driver library is directly referenced.
 *
 * PATTERN:
 * LightingManager uses a SINGLETON pattern which returns an instance of the
 * manager for a specific chain of LEDs, referred to as a "deviceSlot":
 *   LightingManager::getInstance(<deviceSlot>);
 *
 * INTERFACE:
 * - initializeDriver() — Sets up the driver library and hardware pins
 * - show() — Updates physical LEDs with current buffer state
 * - lightsOff() — Blanks all LEDs
 * - setBrightness() — Controls global brightness
 * - setPixelColor(index, ColorID, brightness) — Set a single LED to a color with automatic color order
 * - getPixelColor(index) — Read a single LED's current color as LED_RGB
 * - setCustomColorHSV(hsv) — Store custom HSV color in the Lighting library
 * - setColorOrder(deviceSlot, userPref) — Set color channel order for the device
 * - fillPalette(palette, speedMultiplier) — Fill all LEDs with palette animation using Lighting library
 */
class LightingManager {
private:
  static LightingManager* instances[DEVICE_SLOTS]; // Array of singleton instances for each device slot.
  static Lighting lightingLib; // Shared Lighting library instance across all slots for animation state.
  Adafruit_NeoPixel pixels; // The single chain of LEDs associated with a hardware pin.
  const uint8_t assignedSlot; // The device slot assigned to this instance of the LightingManager.

  // Private constructor - called only once per slot by getInstance()
  // Initializes the Lighting library as lightingLib with 1 device slot,
  // and initializes the Adafruit_NeoPixel object as a variable "pixels".
  LightingManager() :
    pixels(DEVICE_MAX_LEDS, DEVICE_LED_PIN, NEO_RGB + NEO_KHZ800),
    assignedSlot(0) {
    lightingLib.setColorOrder(assignedSlot, ORDER_GRB); // This device specifically defaults to GRB.
  }

  // Helper: Convert packed uint32_t color to LED_RGB components
  // Internal utility used by getPixelColor()
  LED_RGB unpackColor(uint32_t packedColor) {
    uint8_t r = (packedColor >> 16) & 0xFF;
    uint8_t g = (packedColor >> 8) & 0xFF;
    uint8_t b = packedColor & 0xFF;
    return LED_RGB{r, g, b};
  }

public:
  // Singleton instances per slot
  static LightingManager& getInstance() {
    if(instances[0] == nullptr) {
      instances[0] = new LightingManager();
    }
    return *instances[0];
  }

  // Initialize LED driver
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    pixels.begin();
    pixels.setBrightness(DEVICE_MAX_BRIGHTNESS);
    pixels.show();
  }

  // Turn off all LEDs
  void lightsOff() {
    pixels.clear(); // Set all to black (off).
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    pixels.setBrightness(brightness);
  }

  // Set custom color HSV values in the Lighting library
  void setCustomColorHSV(const LED_HSV &hsv) {
    lightingLib.setCustomColorHSV(hsv, assignedSlot);
  }

  // Set color order for a device with automatic enum mapping
  void setColorOrder(ColorOrder userPrefValue) {
    lightingLib.setColorOrder(assignedSlot, userPrefValue);
  }

  // Update LED display
  void show() {
    pixels.show(); // Pass through to the LED driver library to update LED states.
  }

  // Returns a pixel's current color as LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    if(index >= 0 && index < pixels.numPixels()) {
      return unpackColor(pixels.getPixelColor(index));
    }
    return LED_RGB_BLACK; // Return black if index is out of bounds.
  }

  // Set a pixel color by ColorID and automatically apply stored color order.
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    if(index >= 0 && index < pixels.numPixels()) {
      // Get color as HSV
      LED_HSV hsv;
      if(isColorDynamic(colorEnum)) {
        hsv = lightingLib.getDynamicColorHSV(assignedSlot, colorEnum, brightness);
      } else {
        hsv = lightingLib.getColorHSV(colorEnum, brightness);
      }

      // Convert the HSV color to RGB triplet.
      LED_RGB rgb = Lighting::hsv2rgb(hsv);

      // Apply the device-specific color order for the RGB values.
      LED_RGB ordered = Lighting::applyColorOrder(rgb, lightingLib.getColorOrder(assignedSlot));

      // Set the given LED to the calculated, ordered RGB value.
      pixels.setPixelColor(index, pixels.Color(ordered.r, ordered.g, ordered.b));
    }
  }

  // Animates the LEDs using the palette system for smooth colour transitions.
  void fillPalette(const LED_Palette16& palette, float speedMultiplier = 1.0f) {
    // The palette itself is not indexed by LED count; it is indexed by a 0..255 (256)
    // phase value so the library can interpolate smoothly between adjacent palette
    // entries. This is a palette phase, not the physical LED position on the strand.
    //
    // We assign each LED a different starting phase so the animation appears
    // to flow along the strip as a traveling wave. In other words:
    //   - i_curr_led = physical LED index (0..N-1, where N = i_num_leds)
    //   - i_phase = palette phase offset for that LED (resolution: 0..255)
    //
    // The resulting "phase" is the interpolation between the two neighboring palette
    // entries. When spread, the palette phase is the color state across the strand.
    // This gives each LED a slightly different color in the palette timeline while
    // the animation itself still advances at the same speed.
    for(uint16_t i_curr_led = 0; i_curr_led < i_num_leds; i_curr_led++) {
      // Calculate position offset for this LED (0-255 distributed across strand)
      uint8_t i_phase = (i_curr_led * 255 / i_num_leds);

      // Get interpolated palette color for the device with this LED's calculated phase.
      // Parameters: palette, speed, offset, brightness, reverse
      LED_RGB rgb = lightingLib.getPaletteColor(assignedSlot, // Device slot for this instance
                                                palette, // Palette in use for color interpolation
                                                speedMultiplier, // Speed for animation (1.0-10.0)
                                                i_phase, // Calculated interpolation phase for this LED (0-255)
                                                i_max_brightness, // User-defined maximum brightness (26-255)
                                                b_invert_direction); // User-defined animation inversion flag

      // Set the current LED to the interpolated color.
      pixels.setPixelColor(i_curr_led, pixels.Color(rgb.r, rgb.g, rgb.b));
    }
  }
};

/**
 * SINGLETON PATTERN: Static member variable initialization
 *
 * This array MUST exist outside the class definition for any static member.
 * It allocates memory for instance pointers per slot and initializes them to nullptr.
 *
 * Each LightingManager object is created lazily on the FIRST call to getInstance(slot),
 * which checks if instances[slot] is nullptr, creates it if needed with that slot bound,
 * then returns a reference to it. Subsequent calls for that slot return the same instance.
 *
 * This ensures each slot has exactly ONE LightingManager instance, with no state mutation.
 *
 * Additionally, the Lighting library is shared across all instances so that animation
 * state (palette phase, color tracking) is consistent across all LED chains.
 */
LightingManager* LightingManager::instances[DEVICE_SLOTS] = {};
Lighting LightingManager::lightingLib(DEVICE_SLOTS, DEVICE_REFRESH_MS);
