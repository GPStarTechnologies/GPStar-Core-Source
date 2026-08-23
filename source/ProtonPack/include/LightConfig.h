/**
 *   GPStar Proton Pack
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

// Include the intended LED driver: Adafruit NeoPixel
#ifdef ESP32
  #include <Adafruit_NeoPXL8.h>
#else
  #include <Adafruit_NeoPixel.h>
#endif

// Include the generalized Lighting library
#include <Lighting.h>

// Include global palette definitions
#include <LightingPalettes.h>

// Forward declarations for variables from Configuration.h
extern uint8_t i_powercell_num_leds;
extern uint8_t i_cyclotron_num_leds;

// ============================================================================
// LOCAL LIGHTING VARIABLES
// ============================================================================

/*
 * The HasLab Power Cell has 13 LEDs.
 */
#define HASLAB_POWERCELL_LED_COUNT 13

/*
 * The GPStar and Frutto Power Cell has 15 LEDs.
 */
#define MAX_POWERCELL_LED_COUNT 15

/*
 * Support for 4-LED DIY packs.
 */
 #define QUAD_CYCLOTRON_LED_COUNT 4

/*
 * The HasLab Cyclotron Lid has 12 LEDs (4x3)
 * However, configuration is subject to the stock pack type:
 * - Afterlife: Each of the 4 lenses has 3 LEDs in an arc.
 * - 1984: Each of the 4 lenses has 3 LEDs in a triangle.
 */
#define HASLAB_CYCLOTRON_LED_COUNT 12

/*
 * The Frutto Cyclotron Lid has 20 LEDs.
 */
#define FRUTTO_CYCLOTRON_LED_COUNT 20

/*
 * The GPStar and Frutto Max Cyclotron Lid has 36 LEDs.
 */
#define MAX_CYCLOTRON_LED_COUNT 36

/*
 * Set the number of steps for the Outer Cyclotron (lid).
 */
#define OUTER_CYCLOTRON_LED_MAX 40

/*
 * Set the number of LEDs for the optional Inner Cyclotron panel board.
 * This is not the single traditional LEDs, but the optional board with 8 pixels instead.
 */
#define INNER_CYCLOTRON_LED_PANEL_MAX 8

/*
 * Set the number of steps for the Inner Cyclotron (cake).
 */
#define INNER_CYCLOTRON_CAKE_LED_MAX 36

/*
 * Set the number of steps for the Inner Cyclotron (cavity).
 */
#define INNER_CYCLOTRON_CAVITY_LED_MAX 20

/*
 * The gpstar N-Filter expects 7 LEDs.
 */
#define JEWEL_NFILTER_LED_COUNT 7

/*
 * Pin for Addressable LEDs
 * Assumes WS2812B addressable LEDs (NeoPixel compatible)
 * ProtonPack uses 2 separate LED pins for pack and cyclotron
 * GPStar II devices add 2 pins for expansion ports
 */
#ifdef ESP32
  #define CYCLOTRON_LED_PIN 4 // Data pin for Cyclotron LED panel and LED ring in the cake (+cavity LEDs).
  #define PACK_LED_PIN 5 // Data pin for the Power Cell and Outer Cyclotron (lid) addressable LEDs.
  #define EXPANSION1_LED_PIN 41 // Data pin for addressable LEDs as future expansion.
  #define EXPANSION2_LED_PIN 42 // Data pin for addressable LEDs as future expansion.
#else
  #define CYCLOTRON_LED_PIN 13 // Data pin for Cyclotron LED panel and LED ring in the cake (+cavity LEDs).
  #define PACK_LED_PIN 53 // Data pin for the Power Cell and Outer Cyclotron addressable LEDs.
#endif
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for the optimal effect

/**
 * In the case of the Proton Pack we have 2 chains of addressable LEDs:
 *  1) The "pack" lights which consist of the Powercell, Cyclotron, and N-Filter.
 *  2) The inner cyclotron "cake" plus anything beyond that point.
 *
 * So for every 100 LEDs at 30μs each to update, that's 3ms of interrupt disruption. For
 * a microcontroller that's a lot of time so we need to keep those updates to a minimum.
 * The best way to do that while still providing all of the lights desired is to keep those
 * chains of lights to a minimum where possible. Thus, we only support a certain # of LEDs.
 *
 * Total number of LEDs in the standard Proton Pack configuration.
 * Power Cell and Cyclotron Lid LEDs + optional N-Filter NeoPixel.
 *    25 LEDs in the stock HasLab kit: 13 in the Power Cell and 12 in the Cyclotron lid.
 *    Add 7 (now 32 in total) for a NeoPixel jewel that you can put into the N-Filter (optional)
 *    That jewel chains off Cyclotron lens assembly #4 in the lid (top left lens).
 * Max 62 LEDs: 15 for the Power Cell, 40 for the Cyclotron lid, and 7 for the jewel.
 */
const uint8_t i_max_pack_leds = MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX;
const uint8_t i_nfilter_jewel_leds = JEWEL_NFILTER_LED_COUNT;

/*
 * Updated count of all the LEDs plus the N-Filter jewel.
 * This gets updated by the system if the wand changes the led count in the EEPROM menu system.
 */
uint8_t i_pack_num_leds = i_powercell_num_leds + i_cyclotron_num_leds + i_nfilter_jewel_leds;

/*
 * Which LED the N-Filter jewel LEDs start.
 * This gets updated by the system if the wand changes the LED count in the EEPROM menu system.
 */
uint8_t i_vent_light_start = i_powercell_num_leds + i_cyclotron_num_leds;

/*
 * Delay to update the addressable LEDs.
 * We have up to 126 addressable LEDs if using NeoPixel jewel in the N-Filter, a ring
 * for the Inner Cyclotron, and the optional "sparking" cyclotron cavity LEDs.
 * 0.0312 ms to update each LED, then a 0.05 ms resting period once all are updated.
 * So 4 ms should be okay. Let's bump it up to 5 just in case.
 * For cyclotrons with high density LEDs, increase this based on the cyclotron speed multiplier to simulate a faster spinning cyclotron.
 * This works by "skipping frames" in the animation, which can be done up until about 15 ms.
 * After 15ms it will become painfully obvious to most people that the animation is not smooth.
 */
#define DEVICE_REFRESH_MS 6 // Refresh rate for the addressable LEDs (in milliseconds)
uint8_t i_led_update_delay = DEVICE_REFRESH_MS;
millisDelay ms_led_driver;

// ============================================================================
// LED DEVICE CONFIGURATION
// ============================================================================

#ifdef ESP32
  #define DEVICE_SLOTS 8 // Total segments: 3 (PACK) + 3 (CYCLOTRON) + 2 (EXPANSION) for Lighting library
  #define PACK_LED_COUNT (MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT)  // Max 62 LEDs
  #define CYCLOTRON_LED_COUNT (INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX)  // Max 64 LEDs
  #define EXP1_LED_COUNT 60  // Default expansion port 1 LED count
  #define EXP2_LED_COUNT 60  // Default expansion port 2 LED count
#else
  #define DEVICE_SLOTS 6 // Total segments: 3 (PACK) + 3 (CYCLOTRON) for Lighting library
  #define PACK_LED_COUNT (MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT)  // Max 62 LEDs
  #define CYCLOTRON_LED_COUNT (INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX)  // Max 64 LEDs
#endif

/*
 * Total number of LEDs in the optional inner cyclotron configuration.
 * Max 64 LEDs is possible before degradation of serial communications!
 * - Up to 8 LEDs for the inner panel by Frutto Technology.
 * - Up to 36 LEDs for the largest ring provided by GPStar kits.
 * - Optionally, up to 20 LEDs for the "sparking" effect in the cavity.
 */
const uint8_t i_max_inner_cyclotron_leds = CYCLOTRON_LED_COUNT;

/*
 * Set the maximum number of LEDs which may be contained on any chain of pixels.
 * Necessary for the Adafruit_NeoPXL8 library and related calculations for chains.
 */
#define PXL8_WORKAROUND_BUFFER 2 // Addresses a current bug which needs the LED count to increase by 2.
const uint16_t i_max_pxl8_count = max(PACK_LED_COUNT, CYCLOTRON_LED_COUNT) + PXL8_WORKAROUND_BUFFER;

/*
 * Addressable LED Chains - Physical lengths of LEDs driven by a hardware pin.
 */
enum LED_CHAIN {
  CHAIN_PACK = 0,      // Power Cell + Outer Cyclotron + N-Filter on PACK_LED_PIN
  CHAIN_CYCLOTRON = 1, // Inner Panel + Cake + Cavity on CYCLOTRON_LED_PIN
  CHAIN_EXP1 = 2,      // Expansion port 1 on EXPANSION1_LED_PIN (ESP32 only)
  CHAIN_EXP2 = 3       // Expansion port 2 on EXPANSION2_LED_PIN (ESP32 only)
};

/*
 * LED Segment Identifiers - Logical groupings of LEDs within a physical chain.
 */
enum LED_SEGMENT : uint8_t {
#ifdef ESP32
  // CHAIN_PACK segments
  DEVICE_POWERCELL = 0,
  DEVICE_CYCLOTRON_LID = 1,
  DEVICE_NFILTER = 2,

  // CHAIN_CYCLOTRON segments
  DEVICE_INNER_PANEL = 3,
  DEVICE_INNER_CAKE = 4,
  DEVICE_INNER_CAVITY = 5,

  // Expansion segments
  DEVICE_EXP1 = 6,
  DEVICE_EXP2 = 7
#else
  // CHAIN_PACK segments
  DEVICE_POWERCELL = 0,
  DEVICE_CYCLOTRON_LID = 1,
  DEVICE_NFILTER = 2,

  // CHAIN_CYCLOTRON segments
  DEVICE_INNER_PANEL = 3,
  DEVICE_INNER_CAKE = 4,
  DEVICE_INNER_CAVITY = 5
#endif
};


/*
 * Lighting Devices - Registry of segments to chains (w/ pin).
 */
struct LightingDevice {
  LED_SEGMENT SegmentID; // LED device segment identifier.
  LED_CHAIN ChainID; // Identifier for the associated chain.
  uint8_t HWPin; // Hardware pin for the physical LED chain
};

/*
 * Lighting Registry - source of truth for segment→chain→pin mapping
 */
static constexpr LightingDevice lighting_devices[DEVICE_SLOTS] = {
#ifdef ESP32
  {DEVICE_POWERCELL,     CHAIN_PACK,      PACK_LED_PIN},
  {DEVICE_CYCLOTRON_LID, CHAIN_PACK,      PACK_LED_PIN},
  {DEVICE_NFILTER,       CHAIN_PACK,      PACK_LED_PIN},
  {DEVICE_INNER_PANEL,   CHAIN_CYCLOTRON, CYCLOTRON_LED_PIN},
  {DEVICE_INNER_CAKE,    CHAIN_CYCLOTRON, CYCLOTRON_LED_PIN},
  {DEVICE_INNER_CAVITY,  CHAIN_CYCLOTRON, CYCLOTRON_LED_PIN},
  {DEVICE_EXP1,          CHAIN_EXP1,      EXPANSION1_LED_PIN},
  {DEVICE_EXP2,          CHAIN_EXP2,      EXPANSION2_LED_PIN}
#else
  {DEVICE_POWERCELL,     CHAIN_PACK,      PACK_LED_PIN},
  {DEVICE_CYCLOTRON_LID, CHAIN_PACK,      PACK_LED_PIN},
  {DEVICE_NFILTER,       CHAIN_PACK,      PACK_LED_PIN},
  {DEVICE_INNER_PANEL,   CHAIN_CYCLOTRON, CYCLOTRON_LED_PIN},
  {DEVICE_INNER_CAKE,    CHAIN_CYCLOTRON, CYCLOTRON_LED_PIN},
  {DEVICE_INNER_CAVITY,  CHAIN_CYCLOTRON, CYCLOTRON_LED_PIN}
#endif
};

// ============================================================================
// NEOPXL8 PIN CONFIGURATION (ESP32 ONLY)
// ============================================================================

#ifdef ESP32
/*
 * NeoPXL8 supports up to 8 hardware pins. Configure pins for GPStar devices:
 * - Pin 0: PACK_LED_PIN (CHAIN_PACK)
 * - Pin 1: CYCLOTRON_LED_PIN (CHAIN_CYCLOTRON)
 * - Pin 2: EXPANSION1_LED_PIN (CHAIN_EXP1)
 * - Pin 3: EXPANSION2_LED_PIN (CHAIN_EXP2)
 * - Pins 4-7: Unused (set to -1)
 */
static int8_t pxl8_pins[8] = {
  PACK_LED_PIN,       // Pin 0
  CYCLOTRON_LED_PIN,  // Pin 1
  EXPANSION1_LED_PIN, // Pin 2
  EXPANSION2_LED_PIN, // Pin 3
  -1, -1, -1, -1      // Pins 4-7 unused
};
#endif

// ============================================================================
// LIGHTING LIBRARY CONFIGURATION & INITIALIZATION
// ============================================================================

/**
 * LightingManager - Abstraction Layer for LED Driver Operations
 *
 * PURPOSE:
 * This class provides a driver-agnostic interface for direct LED operations.
 * It translates logical segments (LED_SEGMENT) into:
 *   1. Physical hardware chains (LED_CHAIN) for pixel operations
 *   2. Lighting library slots (0..DEVICE_SLOTS-1) for color tracking and animation
 *
 * PATTERN:
 * LightingManager uses a SINGLETON pattern accessed via segment identifier:
 *   LightingManager::getInstance(DEVICE_POWERCELL).setPixelColor(...)
 *   LightingManager::getInstance(DEVICE_INNER_CAKE).setCustomColorHSV(...)
 *
 * INTERFACE:
 * - initializeDriver() — Sets up the driver library and hardware pins
 * - show() — Updates physical LEDs with current buffer state
 * - lightsOff() — Blanks all LEDs on the current segment

 * - setPixelColor(index, ColorID, brightness) — Set LED with automatic color order
 * - getPixelColor(index) — Read a single LED's current color as LED_RGB
 * - setCustomColorHSV(hsv) — Store custom HSV for this segment's custom color slot
 * - setColorOrder(colorOrder) — Set color channel order for this segment
 * - fillPalette(palette, speedMultiplier) — Animate segment with palette
 */
class LightingManager {
private:
  static LightingManager* instances[DEVICE_SLOTS]; // Array of singleton instances for each device slot.
  static Lighting lightingLib; // Shared Lighting library instance across all slots for animation state.
#ifdef ESP32
  static Adafruit_NeoPXL8 systemLEDs;  // Single NeoPXL8 instance for all 8 pins
#else
  static Adafruit_NeoPixel packLEDs;
  static Adafruit_NeoPixel cyclotronLEDs;
#endif
  const LED_SEGMENT assignedSlot; // The device slot assigned to this instance of the LightingManager.

  // Private constructor - called once per segment by getInstance()
  // Initializes the driver objects for this segment.
  LightingManager(LED_SEGMENT segment) :
    assignedSlot(segment) {
    lightingLib.setColorOrder(assignedSlot, ORDER_RGB); // Set the logical order for RGB triplets.
  }

  // Helper: Convert packed uint32_t color to LED_RGB components
  // Internal utility used by getPixelColor()
  LED_RGB unpackColor(uint32_t packedColor) {
    uint8_t r = (packedColor >> 16) & 0xFF;
    uint8_t g = (packedColor >> 8) & 0xFF;
    uint8_t b = packedColor & 0xFF;
    return LED_RGB{r, g, b};
  }

  // Helper: Maps an LED_SEGMENT to its physical hardware CHAIN using the registry
  // Looks up the segment in the lighting_devices registry and returns its associated chain.
  // Falls back to CHAIN_PACK if segment is not found (should not happen in normal operation).
  LED_CHAIN segmentToChain(LED_SEGMENT segment) const {
    for(const auto& device : lighting_devices) {
      if(device.SegmentID == segment) {
        return device.ChainID;
      }
    }
    return CHAIN_PACK; // fallback
  }

  // Helper: Returns the LED count for the given segment.
  // Internally converts the segment to its hardware chain and returns the LED count.
  uint16_t getCount(LED_SEGMENT segment) const {
    LED_CHAIN chain = segmentToChain(segment);
    switch(chain) {
      case CHAIN_PACK:
      default:
        return PACK_LED_COUNT;

      case CHAIN_CYCLOTRON:
        return CYCLOTRON_LED_COUNT;

    #ifdef ESP32
      case CHAIN_EXP1:
        return EXP1_LED_COUNT;

      case CHAIN_EXP2:
        return EXP2_LED_COUNT;
    #endif
    }
  }

#ifdef ESP32
  // Helper: Returns the physical strip object for the given segment.
  // Internally converts the segment to its hardware chain and returns the corresponding pixels object.
  Adafruit_NeoPXL8& getDevicePixels(LED_SEGMENT segment) {
    return systemLEDs; // Only 1 object instance for this hardware.
  }

  // Helper: Calculate buffer offset for pixel access in NeoPXL8
  // For ESP32: Each chain gets a reserved section of i_max_pxl8_count LEDs
  //            Offset = (chain_id * i_max_pxl8_count) + pixel_index
  uint16_t getBufferOffset(LED_SEGMENT segment, uint16_t pixel_index) const {
    LED_CHAIN chain = segmentToChain(segment);
    return ((uint16_t)chain * i_max_pxl8_count) + pixel_index;
  }
#else
  // Helper: Returns the physical strip object for the given segment.
  // Internally converts the segment to its hardware chain and returns the corresponding pixels object.
  Adafruit_NeoPixel& getDevicePixels(LED_SEGMENT segment) {
    LED_CHAIN chain = segmentToChain(segment);
    switch(chain) {
      case CHAIN_CYCLOTRON:
        return cyclotronLEDs;

      case CHAIN_PACK:
      default:
        return packLEDs;
    }
  }
#endif

public:
  // Singleton instances per segment/slot
  // Maps the segment to its lighting library slot and hardware chain,
  // returning a manager instance configured for that segment.
  static LightingManager& getInstance(LED_SEGMENT segment) {
    if(instances[segment] == nullptr) {
      instances[segment] = new LightingManager(segment);
    }
    return *instances[segment];
  }

  // Initialize LED driver
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.begin();
    pixels.setBrightness(DEVICE_MAX_BRIGHTNESS);
    pixels.show();
  }

  // Turn off LEDs on the current segment
  void lightsOff() {
  #ifdef ESP32
    uint16_t i_slot_leds = getCount(assignedSlot);
    for(uint16_t i = 0; i < i_slot_leds; i++) {
      setPixelColor(i, LED_RGB_BLACK);
    }
  #else
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.clear(); // Set all to black (off).
  #endif
  }

  // Set custom color HSV values in the Lighting library
  void setCustomColorHSV(const LED_HSV &hsv) {
    lightingLib.setCustomColorHSV(hsv, assignedSlot);
  }

  // Set color order for this segment with standard enum mapping
  void setColorOrder(ColorOrder newColorOrder) {
    lightingLib.setColorOrder(assignedSlot, newColorOrder);
  }

  // Update LED display on current chain
  void show() {
    auto& pixels = getDevicePixels(assignedSlot);
    pixels.show(); // Pass through to the LED driver library to update LED states.
  }

  // Returns a pixel's current color as LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    auto& pixels = getDevicePixels(assignedSlot);
    uint16_t i_slot_leds = getCount(assignedSlot);
    if(index >= 0 && index < i_slot_leds) {
    #ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot, index);
      return unpackColor(pixels.getPixelColor(buffer_index));
    #else
      return unpackColor(pixels.getPixelColor(index));
    #endif
    }
    return LED_RGB_BLACK; // Return black if index is out of bounds.
  }

  // Get an RGB color by ColorID and automatically apply stored color order.
  // NOTE: This does not actually set the color in a pixel, only returns the intended LED_RGB value.
  LED_RGB getColorRGB(ColorID colorEnum, uint8_t brightness = 255) {
    // Get color as HSV
    LED_HSV hsv;
    if(Lighting::isColorDynamic(colorEnum)) {
      hsv = lightingLib.getDynamicColorHSV(assignedSlot, colorEnum, brightness);
    } else {
      hsv = lightingLib.getColorHSV(colorEnum, brightness);
    }

    // Convert HSV to RGB and return in the color order as set for the device
    LED_RGB rgb = Lighting::hsv2rgb(hsv);
    return Lighting::applyColorOrder(rgb, lightingLib.getColorOrder(assignedSlot));
  }

  // Set a pixel color by ColorID and automatically apply stored color order.
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    auto& pixels = getDevicePixels(assignedSlot);
    uint16_t i_slot_leds = getCount(assignedSlot);
    if(index >= 0 && index < i_slot_leds) {
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
      uint16_t buffer_index = getBufferOffset(assignedSlot, index);
      pixels.setPixelColor(buffer_index, pixels.Color(ordered.r, ordered.g, ordered.b));
    #else
      pixels.setPixelColor(index, pixels.Color(ordered.r, ordered.g, ordered.b));
    #endif
    }
  }

  // Set a pixel color direct from an RGB triplet.
  void setPixelColor(uint16_t index, LED_RGB colorRGB) {
    auto& pixels = getDevicePixels(assignedSlot);
    uint16_t i_slot_leds = getCount(assignedSlot);
    if(index >= 0 && index < i_slot_leds) {
    #ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot, index);
      pixels.setPixelColor(buffer_index, pixels.Color(colorRGB.r, colorRGB.g, colorRGB.b));
    #else
      pixels.setPixelColor(index, pixels.Color(colorRGB.r, colorRGB.g, colorRGB.b));
    #endif
    }
  }

  // Uniformly scale the brightness of a pixel, while attempting to retain the RGB color.
  void maximizeBrightness(uint16_t index, uint8_t brightness_limit = 255) {
    auto& pixels = getDevicePixels(assignedSlot);
    if(index >= 0 && index < pixels.numPixels()) {
      // Get current pixel's RGB color
      LED_RGB current_rgb = getPixelColor(index);
      LED_RGB scaled_rgb = LED_RGB_BLACK;
      
      // Find the maximum component of the RGB triplet.
      uint8_t max_component = max(current_rgb.r, max(current_rgb.g, current_rgb.b));

      // Scale all components proportionally so max becomes brightness_limit
      if(max_component > 0) {
        uint16_t scale_factor = ((uint16_t)(brightness_limit) * 256) / max_component;
        scaled_rgb.r = (uint8_t)(current_rgb.r * scale_factor);
        scaled_rgb.g = (uint8_t)(current_rgb.g * scale_factor);
        scaled_rgb.b = (uint8_t)(current_rgb.b * scale_factor);
      }

      setPixelColor(index, scaled_rgb); // Apply the new color back to the same pixel.
    }
  }

  // Scale pixel brightness by a fade amount, preserving color ratio (mimics FastLED fadeToBlackBy).
  void scalePixelBrightness(uint16_t index, uint8_t fade_amount) {
    // No fade needed if fade_amount is 0
    if(fade_amount == 0) {
      return;
    }
    
    LED_RGB current_rgb = getPixelColor(index);
    
    // Convert fade_amount (0-255 scale) to a brightness retention factor (0.0-1.0)
    // fade_amount is the intensity of the fade; higher values fade more
    // Example: fade_amount=1 keeps 99.6% brightness; fade_amount=255 keeps 0%
    float fade_factor = (255.0f - fade_amount) / 255.0f;
    
    // Scale each component proportionally to preserve color ratio
    LED_RGB scaled_rgb;
    scaled_rgb.r = (uint8_t)(current_rgb.r * fade_factor);
    scaled_rgb.g = (uint8_t)(current_rgb.g * fade_factor);
    scaled_rgb.b = (uint8_t)(current_rgb.b * fade_factor);
    
    // Apply the scaled color back
    setPixelColor(index, scaled_rgb);
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

      // Apply the device-specific color order for the RGB values.
      LED_RGB ordered = Lighting::applyColorOrder(rgb, lightingLib.getColorOrder(assignedSlot));

      // Set the given LED to the calculated, ordered RGB value.
    #ifdef ESP32
      uint16_t buffer_index = getBufferOffset(assignedSlot, i_curr_led);
      pixels.setPixelColor(buffer_index, pixels.Color(ordered.r, ordered.g, ordered.b));
    #else
      pixels.setPixelColor(i_curr_led, pixels.Color(ordered.r, ordered.g, ordered.b));
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
  Adafruit_NeoPixel LightingManager::packLEDs(PACK_LED_COUNT, PACK_LED_PIN, NEO_GRB + NEO_KHZ800);
  Adafruit_NeoPixel LightingManager::cyclotronLEDs(CYCLOTRON_LED_COUNT, CYCLOTRON_LED_PIN, NEO_GRB + NEO_KHZ800);
#endif
