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

// Include the intended LED driver first: Adafruit NeoPixel (ATMega) or NeoPXL8 (for ESP32-S3)
#ifdef ESP32
  #include <Adafruit_NeoPXL8.h>
#else
  #include <Adafruit_NeoPixel.h>
#endif

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
  #define BARREL_LED_PIN 41 // Data pin for the addressable LEDs used by the barrel and cyclotron.
  #define TOP_LED_PIN 42 // Data pin for the addressable LEDs to the vent light.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#else
  #define BARREL_LED_PIN 10 // Data pin for the addressable LEDs used by the barrel and cyclotron.
  #define TOP_LED_PIN 12 // Data pin for the addressable LEDs to the vent light, and top blinking light when RGB vent light is disabled.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#endif
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect

/*
 * Counts for segments of special LED chains
 * Note that these are in the expected physical order in the chain
 */
#define DEVICE_REFRESH_MS 16 // Refresh rate for the addressable LEDs (in milliseconds)
#define CYCLOTRON_LED_COUNT 7 // GPStar 7-LED Jewel
#define BARREL_LED_COUNT 7 // GPStar 7-LED Jewel
#define SYSTEM_LED_COUNT (CYCLOTRON_LED_COUNT + BARREL_LED_COUNT) // Sum of cyclotron and barrel LEDs.
#define VENT_LED_COUNT 2 // The maximum number of LEDs for the vent lights. Main vent + top Clip Lite.

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
const uint16_t i_vent_light_update_interval = 150; // FastLED update interval specifically for the top/vent LEDs.
bool b_vent_lights_changed = false; // Check for whether there was actually a change to prevent superfluous calls to showLeds().

// ============================================================================
// LED CHAIN IDENTIFIERS
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
 * manager for a specific chain of LEDs, refererred to as a "deviceSlot":
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
 * - setColorOrder(deviceSlot, colorOrder) — Set color channel order for the device
 * - fillPalette(palette, speedMultiplier) — Fill all LEDs with palette animation using Lighting library
 */
class LightingManager {
private:
  static LightingManager* instance;
  Lighting lightingLib;
  #ifdef ESP32
    static inline int8_t pxl8Pins[8] = {BARREL_LED_PIN, RGB_VENT_PIN, -1, -1, -1, -1, -1, -1};
    Adafruit_NeoPXL8 systemLEDs;
    Adafruit_NeoPXL8 ventLEDs;
  #else
    Adafruit_NeoPixel systemLEDs;
    Adafruit_NeoPixel ventLEDs;
  #endif
  uint8_t currentDeviceSlot; // Track device slot for an instance.

  // Private constructor - called only once by getInstance()
  // Initializes the Lighting library as lightingLib with 2 device slots.
  LightingManager() :
    lightingLib(DEVICE_SLOTS, DEVICE_REFRESH_MS),
    #ifdef ESP32
      systemLEDs(SYSTEM_LED_COUNT, pxl8Pins, NEO_GRB + NEO_KHZ800),
      ventLEDs(VENT_LED_COUNT, pxl8Pins, NEO_GRB + NEO_KHZ800),
    #else
      systemLEDs(SYSTEM_LED_COUNT, BARREL_LED_PIN, NEO_GRB + NEO_KHZ800),
      ventLEDs(VENT_LED_COUNT, RGB_VENT_PIN, NEO_GRB + NEO_KHZ800),
    #endif
    currentDeviceSlot(0) {}

  // Helper: Returns the physical strip object for the given device slot.
  #ifdef ESP32
    Adafruit_NeoPXL8& getDevicePixels(uint8_t slot) {
  #else
    Adafruit_NeoPixel& getDevicePixels(uint8_t slot) {
  #endif
    switch(slot) {
      case CHAIN_VENT:
        return ventLEDs;

      case CHAIN_SYSTEM:
      default:
        return systemLEDs;
    }
  }

  // Helper: Returns the LED count for the given device slot.
  uint16_t getCount(uint8_t slot) {
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
    auto& pixels = getDevicePixels(currentDeviceSlot);
    pixels.begin();
    pixels.setBrightness(DEVICE_MAX_BRIGHTNESS);
    pixels.show();
  }

  // Turn off all LEDs
  void lightsOff() {
    auto& pixels = getDevicePixels(currentDeviceSlot);
    pixels.clear(); // Set all to black (off).
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    auto& pixels = getDevicePixels(currentDeviceSlot);
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
    auto& pixels = getDevicePixels(currentDeviceSlot);
    pixels.show(); // Pass through to the LED driver library to update LED states.
  }

  // Returns a pixel's current color as LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    auto& pixels = getDevicePixels(currentDeviceSlot);
    if(index >= 0 && index < pixels.numPixels()) {
      return unpackColor(pixels.getPixelColor(index));
    }
    return LED_RGB_BLACK; // Return black if index is out of bounds.
  }

  // Set a pixel color by ColorID and automatically apply stored color order.
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    auto& pixels = getDevicePixels(currentDeviceSlot);
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
    //   - i_curr_led = physical LED index (0..N-1, where N = DEVICE_MAX_LEDS)
    //   - i_phase = palette phase offset for that LED (resolution: 0..255)
    //
    // The resulting "phase" is the interpolation between the two neighboring palette
    // entries. When spread, the palette phase is the color state across the strand.
    // This gives each LED a slightly different color in the palette timeline while
    // the animation itself still advances at the same speed.

	// Since multiple devices are used, we must first obtain the correct list of pixels.
    auto& pixels = getDevicePixels(currentDeviceSlot);
    uint16_t i_slot_leds = getCount(currentDeviceSlot);

	// Iterate over the pixels and set the color according to the device's current state.
    for(uint16_t i_curr_led = 0; i_curr_led < i_slot_leds; i_curr_led++) {
      // Calculate position offset for this LED (0-255 distributed across strand)
      uint8_t i_phase = (i_curr_led * 255 / i_slot_leds);

      // Get interpolated palette color for the device with this LED's calculated phase.
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
