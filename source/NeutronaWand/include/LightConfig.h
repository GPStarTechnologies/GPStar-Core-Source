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

// Include the intended LED driver: Adafruit NeoPixel or NeoPXL8
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
  #define BARREL_LED_PIN 41 // Data pin for the addressable LEDs used by the barrel.
  #define TOP_LED_PIN 42 // Data pin for the addressable LEDs to the vent light.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#else
  #define BARREL_LED_PIN 10 // Data pin for the addressable LEDs used by the barrel.
  #define TOP_LED_PIN 12 // Data pin for the addressable LEDs to the vent light, and top blinking light when RGB vent light is disabled.
  #define RGB_VENT_PIN TOP_LED_PIN // Common name between hardware.
#endif
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect
#define DEVICE_REFRESH_MS 6 // Refresh rate for the addressable LEDs (in milliseconds)

/*
 * Counts for segments of special LED chains
 * Note that these are in the expected physical order in the chain
 */
#define BARREL_LEDS_MAX 50 // The maximum number of barrel LEDs supported (GPStar Neutrona Barrel is 48 + 2 Strobe Tips).
#define VENT_LED_COUNT 2 // The maximum number of LEDs for the vent lights. Main vent + top Clip Lite.

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
const uint16_t i_vent_light_update_interval = 150; // Update interval specifically for the top/vent LEDs.
bool b_vent_lights_changed = false; // Check for whether there was actually a change to prevent superfluous calls to showLeds().

// ============================================================================
// LED DEVICE CONFIGURATION
// ============================================================================

/*
 * Addressable LED Chains - Physical hardware pin assignments for LED control.
 */
enum LED_CHAIN {
  CHAIN_BARREL = 0, // Barrel LEDs
  CHAIN_VENT = 1    // Vent LEDs
};

#define DEVICE_SLOTS 2 // Number of LED chains for the Lighting library

// NeoPXL8 Configuration for ESP32
#ifdef ESP32
  #define BARREL_BUFFER_SIZE BARREL_LEDS_MAX
  #define VENT_BUFFER_SIZE VENT_LED_COUNT
  #define PXL8_WORKAROUND_BUFFER 2  // Addresses a current bug which needs the maximum LED count to increase by 2
  const uint16_t i_max_pxl8_count = max(BARREL_BUFFER_SIZE, VENT_BUFFER_SIZE) + PXL8_WORKAROUND_BUFFER;
  const uint16_t BARREL_BUFFER_OFFSET = 0;                    // Barrel LEDs: indices 0-49
  const uint16_t VENT_BUFFER_OFFSET = BARREL_LEDS_MAX;        // Vent LEDs: indices 50-51

  // NeoPXL8 supports up to 8 hardware pins
  static int8_t pxl8_pins[8] = {
    BARREL_LED_PIN, // Pin 0: Barrel
    RGB_VENT_PIN,   // Pin 1: Vent
    -1, -1, -1, -1, -1, -1 // Pins 2-7 unused
  };
#else
  #define DEVICE_SLOTS_UNUSED 2 // Placeholder for consistency
#endif

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
#ifdef ESP32
  static Adafruit_NeoPXL8 systemLEDs;  // Single NeoPXL8 instance for all pins on ESP32
#else
  static Adafruit_NeoPixel barrelLEDs;
  static Adafruit_NeoPixel ventLEDs;
#endif
  const LED_CHAIN assignedSlot; // The device slot assigned to this instance of the LightingManager.

  // Private constructor - called once per slot by getInstance()
  // Initializes the Lighting library with color order for this slot.
  LightingManager(LED_CHAIN slot) :
    assignedSlot(slot) {
    lightingLib.setColorOrder(assignedSlot, ORDER_RGB); // Set the logical order for RGB triplets. (Hasbro barrels override in System.h)
  }

  // Helper: Returns the physical strip object for the given device slot.
#ifdef ESP32
  Adafruit_NeoPXL8& getDevicePixels(LED_CHAIN slot) {
    return systemLEDs;  // Return reference to shared systemLEDs as pixels
  }

  // Helper: Returns the buffer offset for this slot in the shared NeoPXL8 buffer
  uint16_t getBufferOffset(LED_CHAIN slot) {
    switch(slot) {
      case CHAIN_VENT:
        return VENT_BUFFER_OFFSET;
      case CHAIN_BARREL:
      default:
        return BARREL_BUFFER_OFFSET;
    }
  }
#else
  Adafruit_NeoPixel& getDevicePixels(LED_CHAIN slot) {
    switch(slot) {
      case CHAIN_VENT:
        return ventLEDs;
      case CHAIN_BARREL:
      default:
        return barrelLEDs;
    }
  }
#endif

  // Helper: Returns the LED count for the given device slot.
  uint16_t getCount(LED_CHAIN slot) {
    switch(slot) {
      case CHAIN_VENT:
        return VENT_LED_COUNT;
      case CHAIN_BARREL:
      default:
        return BARREL_LEDS_MAX;
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

  // Turn off LEDs on the chain associated with the current segment.
  // ESP32: Iterates only over the span of LEDs represented by the PXL8 pin (offset to offset+count).
  // ATMega: Calls the natural clear() method on the NeoPixel object associated with the chain.
  void lightsOff() {
    auto& pixels = getDevicePixels(assignedSlot);
  #ifdef ESP32
    uint16_t i_start = getBufferOffset(assignedSlot);
    uint16_t i_end = i_start + getCount(assignedSlot);
    // Clear only this slot's LEDs in the shared buffer
    for(uint16_t i = i_start; i < i_end; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  #else
    pixels.clear(); // Set all to black (off).
  #endif
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

  // Returns a color-ordered RGB color for the Barrel (and possibly the Vent)
  LED_RGB getColorRaw(ColorID colorEnum, uint8_t brightness, ColorOrder colorOrder) {
    // Get the HSV color through normal lighting library
    LED_HSV hsv;
    if(Lighting::isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(assignedSlot, colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV(colorEnum, brightness);
    }

    // Convert HSV to RGB and return in the color order passed from caller
    LED_RGB rgb = Lighting::hsv2rgb(hsv);
    return Lighting::applyColorOrder(rgb, colorOrder);
  }

  // Returns a pixel's current color as LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    auto& pixels = getDevicePixels(assignedSlot);
    if(index >= 0 && index < getCount(assignedSlot)) {
	#ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot) + index;
      return unpackColor(pixels.getPixelColor(buffer_index));
	#else
      return unpackColor(pixels.getPixelColor(index));
	#endif
    }
    return LED_RGB_BLACK; // Return black if index is out of bounds.
  }

  // Set a pixel color by ColorID and automatically apply stored color order.
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    auto& pixels = getDevicePixels(assignedSlot);
    if(index >= 0 && index < getCount(assignedSlot)) {
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
	#ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot) + index;
      pixels.setPixelColor(buffer_index, pixels.Color(ordered.r, ordered.g, ordered.b));
	#else
      pixels.setPixelColor(index, pixels.Color(ordered.r, ordered.g, ordered.b));
	#endif
    }
  }

  // Set a pixel color direct from an RGB triplet.
  void setPixelColor(uint16_t index, LED_RGB colorRGB) {
    auto& pixels = getDevicePixels(assignedSlot);
    if(index >= 0 && index < getCount(assignedSlot)) {
	#ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot) + index;
      pixels.setPixelColor(buffer_index, pixels.Color(colorRGB.r, colorRGB.g, colorRGB.b));
	#else
      pixels.setPixelColor(index, pixels.Color(colorRGB.r, colorRGB.g, colorRGB.b));
	#endif
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
#ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot) + i_curr_led;
      pixels.setPixelColor(buffer_index, pixels.Color(rgb.r, rgb.g, rgb.b));
#else
      pixels.setPixelColor(i_curr_led, pixels.Color(rgb.r, rgb.g, rgb.b));
#endif
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
 */
LightingManager* LightingManager::instances[DEVICE_SLOTS] = {};

/**
 * The Lighting library is shared across all instances so that animation
 * state (palette phase, color tracking) is consistent across all LED chains.
 */
Lighting LightingManager::lightingLib(DEVICE_SLOTS, DEVICE_REFRESH_MS);

/**
 * In order to allow the show() method to be called across segments (devices) the actual pixel
 * chains must be initialized using static class members. This setup step will initialize each
 * Adafruit_NeoPixel or Adafruit_NeoPXL8 object as a static member with the NEO_GRB color order by default.
 */

#ifdef ESP32
  // NeoPXL8 driver using 8 pins with max LED count applied per chain.
  Adafruit_NeoPXL8 LightingManager::systemLEDs(i_max_pxl8_count, pxl8_pins, NEO_GRB + NEO_KHZ800);
#else
  Adafruit_NeoPixel LightingManager::barrelLEDs(BARREL_LEDS_MAX, BARREL_LED_PIN, NEO_GRB + NEO_KHZ800);
  Adafruit_NeoPixel LightingManager::ventLEDs(VENT_LED_COUNT, RGB_VENT_PIN, NEO_GRB + NEO_KHZ800);
#endif
