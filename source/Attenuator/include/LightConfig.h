/**
 *   GPStar Attenuator
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
 * Assumes WS2812B addressable LEDs (NeoPixel compatible)
 */
#define DEVICE_LED_PIN 23 // Data pin for the addressable LEDs.
#define DEVICE_SLOTS 1 // Number of device slots for the Lighting library
#define DEVICE_REFRESH_MS 5 // Refresh rate for the addressable LEDs (in milliseconds)
#define DEVICE_MAX_LEDS 3 // The maximum number of LEDs (Top, Upper, Lower)
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect
uint16_t i_num_leds = DEVICE_MAX_LEDS;

/*
 * Delay for LED driver to update the addressable LEDs.
 */
uint8_t i_led_update_delay = DEVICE_REFRESH_MS;
millisDelay ms_led_driver;

// ============================================================================
// LED IDENTIFIERS
// ============================================================================

/**
 * Attenuator LED Enumeration
 * 
 * Defines logical names for each LED on the Attenuator device.
 * These names map to animation state in the Lighting library.
 * 
 * Physical layout (from top to bottom):
 * - TOP_LED: Status indicator (connection, menu level)
 * - UPPER_LED: Radiation lens (firing/charging state)
 * - LOWER_LED: Stream mode indicator
 */
enum ATTENUATOR_LED : uint8_t {
  TOP_LED = 0,   // Status indicator LED
  UPPER_LED = 1, // Radiation lens LED
  LOWER_LED = 2  // Stream mode indicator LED
};

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
 * manager for a specific chain of LEDs, refererred to as a "deviceSlot":
 *   LightingManager::getInstance(<deviceSlot>);
 *
 * INTERFACE:
 * - initializeDriver() — Sets up the driver library and hardware pins
 * - getMappedIndex(logicalDeviceId) — Get physical index for a logical LED ID
 * - updateLEDMapping(invert) — Apply LED mapping based on inversion preference
 * - show() — Updates physical LEDs with current buffer state
 * - lightsOff() — Blanks all LEDs
 * - setBrightness(brightness) — Controls global brightness (0-255)
 * - setPixelColor(index, ColorID, brightness) — Set a single LED to a color with automatic color order
 * - getPixelColor(index) — Read a single LED's current color as LED_RGB
 * - setCustomColorHSV(hsv) — Store custom HSV color in the Lighting library
 * - setColorOrder(deviceSlot, colorOrder) — Set color channel order for the device
 * - fillPalette(palette, speedMultiplier) — Fill all LEDs with palette animation using Lighting library
 */
class LightingManager {
private:
  static LightingManager* instance;
  Lighting lightingLib;
  Adafruit_NeoPixel pixels;
  uint8_t currentDeviceSlot; // Track device slot for an instance.

  /**
   * LED Device Ordering - Top, Upper, and Lower
   * Creates a simple byte array of N elements for the ID of each of the 3 LEDs.
   * Due to space constraints, users may have had to install the LEDs in reverse.
   * Therefore, the order of this list may change depending on user preference.
   */
  uint8_t mappedLEDs[DEVICE_MAX_LEDS] = {0, 1, 2}; // Default Order
  bool lastInvertState = false; // Track last invert state to detect changes

  // Private constructor - called only once by getInstance()
  // Initializes the Lighting library as lightingLib with 1 device slot,
  // and initializes the Adafruit_NeoPixel object as a variable "pixels".
  LightingManager() :
    lightingLib(DEVICE_SLOTS, DEVICE_REFRESH_MS),
    pixels(DEVICE_MAX_LEDS, DEVICE_LED_PIN, NEO_RGB + NEO_KHZ800),
    currentDeviceSlot(0) {}

  // Helper: Apply a mapping of LED names based on invert flag.
  void applyMapping(bool invert) {
    if(invert) {
      // Flip the identification of the LEDs
      mappedLEDs[0] = 2; // Top
      mappedLEDs[1] = 1; // Upper
      mappedLEDs[2] = 0; // Lower
    } else {
      // Use the expected order for the LEDs
      mappedLEDs[0] = 0; // Top
      mappedLEDs[1] = 1; // Upper
      mappedLEDs[2] = 2; // Lower
    }
  }

  // Helper: Convert packed uint32_t color to LED_RGB components
  // Internal utility used by getPixelColor()
  LED_RGB unpackColor(uint32_t packedColor) {
    uint8_t r = (packedColor >> 16) & 0xFF;
    uint8_t g = (packedColor >> 8) & 0xFF;
    uint8_t b = packedColor & 0xFF;
    return LED_RGB(r, g, b);
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

  // Get the physical index for a logical device ID
  uint8_t getMappedIndex(uint8_t logicalDeviceId) {
    return mappedLEDs[logicalDeviceId];
  }

  // Update LED mapping based on invert preference
  // Only applies changes if the invert state differs from last call
  // This eliminates redundant recalculation in tight loops
  void updateLEDMapping(bool invert) {
    if(invert != lastInvertState) {
      applyMapping(invert);
      lastInvertState = invert;
    }
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
    lightingLib.setCustomColorHSV(hsv, currentDeviceSlot);
  }

  // Set color order for a device with standard enum mapping
  void setColorOrder(uint8_t deviceSlot, ColorOrder newColorOrder) {
    lightingLib.setColorOrder(deviceSlot, newColorOrder);
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
        hsv = lightingLib.getDynamicColorHSV(currentDeviceSlot, colorEnum, brightness);
      } else {
        hsv = lightingLib.getColorHSV(colorEnum, brightness);
      }

      // Convert the HSV color to RGB triplet.
      LED_RGB rgb = Lighting::hsv2rgb(hsv);

      // Apply the device-specific color order for the RGB values.
      LED_RGB ordered = Lighting::applyColorOrder(rgb, lightingLib.getColorOrder(currentDeviceSlot));

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
      LED_RGB rgb = lightingLib.getPaletteColor(currentDeviceSlot, // Device slot for this instance
                                                palette, // Palette in use for color interpolation
                                                speedMultiplier, // Speed for animation (1.0-10.0)
                                                i_phase); // Calculated interpolation phase for this LED (0-255)

      // Set the current LED to the interpolated color.
      pixels.setPixelColor(i_curr_led, pixels.Color(rgb.r, rgb.g, rgb.b));
    }
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
