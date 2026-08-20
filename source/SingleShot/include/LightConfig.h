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

// Include the intended LED driver: Adafruit NeoPixel
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
#ifdef ESP32
  #define BARREL_LED_PIN 41 // Data pin for the addressable LEDs used by the barrel and mini-cyclotron.
  #define TOP_LED_PIN 42 // Data pin for the addressable LEDs to the vent light.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#else
  #define BARREL_LED_PIN 10 // Data pin for the addressable LEDs used by the barrel and mini-cyclotron.
  #define TOP_LED_PIN 12 // Data pin for the addressable LEDs to the vent light, and top blinking light when RGB vent light is disabled.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#endif
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect

/*
 * Counts for segments of special LED chains
 * Note that these are in the expected physical order in the chain
 */
#define CYCLOTRON_LED_COUNT 7 // GPStar 7-LED Jewel, though only the outer 6 LEDs are used
#define BARREL_LED_COUNT 7 // GPStar 7-LED Jewel, located behind the lens
#define SYSTEM_LED_COUNT (CYCLOTRON_LED_COUNT + BARREL_LED_COUNT) // Sum of cyclotron and barrel LEDs.
#define VENT_LED_COUNT 2 // The maximum number of LEDs for the vent lights. Main vent + top Clip Lite.

/**
 * Timings for LED updates
 */
#define DEVICE_REFRESH_MS 16 // Refresh rate for the addressable LEDs (in milliseconds)
/*
 * Pre-calculated indices for segments of the system LED chain (Cyclotron + Barrel)
 */
const uint8_t i_barrel_led = 6; // This will be the index of the light (#7), not the count
const uint8_t i_num_barrel_leds = BARREL_LED_COUNT; // This will be the number of barrel LEDs
const uint8_t i_num_cyclotron_leds = CYCLOTRON_LED_COUNT; // This will be the number of cyclotron LEDs
const uint8_t i_cyclotron_led_start = i_num_barrel_leds; // The first element (index) for the cyclotron.

/*
 * RGB Vent Light Control
 */
const uint16_t i_vent_light_update_interval = 150; // Update interval specifically for the top/vent LEDs.
bool b_vent_lights_changed = false; // Check for whether there was actually a change to prevent superfluous calls to showLeds().

// ============================================================================
// LED DEVICE CONFIGURATION
// ============================================================================

/*
 * Addressable LED Chains
 */
#define DEVICE_SLOTS 2 // Number of device slots for the Lighting library
enum LED_CHAIN {
  CHAIN_SYSTEM = 0, // Barrel + Cyclotron
  CHAIN_VENT = 1 // Vent Lights
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
 * manager for a specific chain of LEDs, referred to as a "deviceSlot":
 *   LightingManager::getInstance(<deviceSlot>);
 *
 * INTERFACE:
 * - initializeDriver() — Sets up the driver library and hardware pins
 * - show() — Updates physical LEDs with current buffer state
 * - lightsOff() — Blanks all LEDs
 * - setBrightness(brightness) — Controls global brightness (0-255)
 * - setPixelColor(index, ColorID, brightness) — Set a single LED to a color with automatic color order
 * - getPixelColor(index) — Read a single LED's current color as LED_RGB
 * - setCustomColorHSV(hsv) — Store custom HSV color in the Lighting library
 * - setColorOrder(colorOrder) — Set color channel order for the device
 * - fillPalette(palette, speedMultiplier) — Fill all LEDs with palette animation using Lighting library
 */
class LightingManager {
private:
  static LightingManager* instances[DEVICE_SLOTS]; // Array of singleton instances for each device slot.
  static Lighting lightingLib; // Shared Lighting library instance across all slots for animation state.
  Adafruit_NeoPixel systemLEDs;
  Adafruit_NeoPixel ventLEDs;
  const LED_CHAIN assignedSlot; // The device slot assigned to this instance of the LightingManager.

  // Private constructor - called once per slot by getInstance()
  // Initializes the Lighting library as lightingLib with 1 device slot, and initializes the
  // Adafruit_NeoPixel object as a variable "pixels" with the NEO_GBR color order by default.
  LightingManager(LED_CHAIN slot) :
    systemLEDs(SYSTEM_LED_COUNT, BARREL_LED_PIN, NEO_GRB + NEO_KHZ800),
    ventLEDs(VENT_LED_COUNT, RGB_VENT_PIN, NEO_GRB + NEO_KHZ800),
    assignedSlot(slot) {
    lightingLib.setColorOrder(assignedSlot, ORDER_RGB); // Set the logical order for RGB triplets.
  }

  // Helper: Returns the physical strip object for the given device slot.
  Adafruit_NeoPixel& getDevicePixels(LED_CHAIN slot) {
    switch(slot) {
      case CHAIN_VENT:
        return ventLEDs;

      case CHAIN_SYSTEM:
      default:
        return systemLEDs;
    }
  }

  // Helper: Returns the LED count for the given device slot.
  uint16_t getCount(LED_CHAIN slot) {
    switch(slot) {
      case CHAIN_VENT:
        return VENT_LED_COUNT;

      case CHAIN_SYSTEM:
      default:
        return SYSTEM_LED_COUNT;
    }
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
  static LightingManager& getInstance(LED_CHAIN deviceSlot) {
    if(instances[deviceSlot] == nullptr) {
      instances[deviceSlot] = new LightingManager(deviceSlot);
    }
    return *instances[deviceSlot];
  }

  // Initialize LED driver
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.begin();
    pixels.setBrightness(DEVICE_MAX_BRIGHTNESS);
    pixels.show();
  }

  // Turn off LEDs on the current device slot
  void lightsOff() {
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.clear(); // Set all to black (off).
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.setBrightness(brightness);
  }

  // Set custom color HSV values in the Lighting library
  void setCustomColorHSV(const LED_HSV &hsv) {
    lightingLib.setCustomColorHSV(hsv, assignedSlot);
  }

  // Set color order for a device with standard enum mapping
  void setColorOrder(ColorOrder newColorOrder = ORDER_RGB) {
    lightingLib.setColorOrder(assignedSlot, newColorOrder);
  }

  // Update LED display
  void show() {
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.show(); // Pass through to the LED driver library to update LED states.
  }

  // Returns a pixel's current color as LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    auto& pixels = getDevicePixels(assignedSlot);
    if(index >= 0 && index < pixels.numPixels()) {
      return unpackColor(pixels.getPixelColor(index));
    }
    return LED_RGB_BLACK; // Return black if index is out of bounds.
  }

  // Set a pixel color by ColorID and automatically apply stored color order.
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    auto& pixels = getDevicePixels(assignedSlot);
    if(index >= 0 && index < pixels.numPixels()) {
      // Get color as HSV
      LED_HSV hsv;
      if(Lighting::isColorDynamic(colorEnum)) {
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
    //   - i_curr_led = physical LED index (0..N-1, where N = DEVICE_MAX_LEDS)
    //   - i_phase = palette phase offset for that LED (resolution: 0..255)
    //
    // The resulting "phase" is the interpolation between the two neighboring palette
    // entries. When spread, the palette phase is the color state across the strand.
    // This gives each LED a slightly different color in the palette timeline while
    // the animation itself still advances at the same speed.

    // Since multiple devices are used, we must first obtain the correct list of pixels.
    auto& pixels = getDevicePixels(assignedSlot);
    uint16_t i_slot_leds = getCount(assignedSlot);

    // Iterate over the pixels and set the color according to the device's current state.
    for(uint16_t i_curr_led = 0; i_curr_led < i_slot_leds; i_curr_led++) {
      // Calculate position offset for this LED (0-255 distributed across strand)
      uint8_t i_phase = (i_curr_led * 255 / i_slot_leds);

      // Get interpolated palette color for the device with this LED's calculated phase.
      LED_RGB rgb = lightingLib.getPaletteColor(assignedSlot, // Device slot for this instance
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
