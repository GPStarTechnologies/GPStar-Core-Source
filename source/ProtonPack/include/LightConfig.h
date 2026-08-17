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

// Forward declarations for variables from Configuration.h
extern uint8_t i_powercell_num_leds;
extern uint8_t i_cyclotron_num_leds;

// ============================================================================
// LOCAL LIGHTING VARIABLES
// ============================================================================

/*
 * Counts for segments of special LED chains
 */
#define DEVICE_REFRESH_MS 5 // Refresh rate for the addressable LEDs (in milliseconds)

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
 * Total number of LEDs in the optional inner cyclotron configuration.
 * Max 64 LEDs is possible before degradation of serial communications!
 * - Up to 8 LEDs for the inner panel by Frutto Technology.
 * - Up to 36 LEDs for the largest ring provided by GPStar kits.
 * - Optionally, up to 20 LEDs for the "sparking" effect in the cavity.
 */
const uint8_t i_max_inner_cyclotron_leds = INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX;

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
 * Legacy LED Buffer: Proton Pack Power Cell and Cyclotron lid LED pin.
 */
//LED_RGB pack_leds[MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT];

/*
 * Legacy LED Buffer: Inner Cyclotron LEDs (optional).
 * Max number of LEDs supported = 64.
 * Maximum expected LEDs for the Inner Switch Panel is 8.
 * Maximum allowed LEDs for the Inner Cyclotron Cake is 36.
 * Maximum allowed LEDs for the Inner Cyclotron Cavity is 20.
 */
//LED_RGB cyclotron_leds[INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX];

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
#define DEVICE_REFRESH_MS 5
uint8_t i_led_update_delay = DEVICE_REFRESH_MS;
millisDelay ms_led_driver;

// ============================================================================
// LED CHAIN IDENTIFIERS
// ============================================================================

/*
 * Addressable LED Chains
 * 
 * ProtonPack has up to 4 independent LED chains:
 * - CHAIN_PACK: Power Cell + Cyclotron Outer (lid) + N-Filter Jewel (PACK_LED_PIN)
 * - CHAIN_CYCLOTRON: Inner Panel + Cake + Cavity (CYCLOTRON_LED_PIN)
 * - CHAIN_EXP1: Expansion port 1 (EXPANSION1_LED_PIN, ESP32 only)
 * - CHAIN_EXP2: Expansion port 2 (EXPANSION2_LED_PIN, ESP32 only)
 *
 * DEVICE_SLOTS = Total number of segments across all chains.
 * Each segment maps to a Lighting library slot for tracking custom colors and animation state.
 */
#ifdef ESP32
  #define DEVICE_SLOTS 6 // Total segments: 3 (PACK) + 3 (CYCLOTRON) for Lighting library
  #define PACK_LED_COUNT (MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT)  // Max 62 LEDs
  #define CYCLOTRON_LED_COUNT (INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX)  // Max 64 LEDs
  #define EXP1_LED_COUNT 60  // Default expansion port 1 LED count
  #define EXP2_LED_COUNT 60  // Default expansion port 2 LED count
#else
  #define DEVICE_SLOTS 6 // Total segments: 3 (PACK) + 3 (CYCLOTRON) for Lighting library
  #define PACK_LED_COUNT (MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT)  // Max 62 LEDs
  #define CYCLOTRON_LED_COUNT (INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX)  // Max 64 LEDs
#endif

enum LED_CHAIN {
  CHAIN_PACK = 0,      // Power Cell + Outer Cyclotron + N-Filter on PACK_LED_PIN
  CHAIN_CYCLOTRON = 1, // Inner Panel + Cake + Cavity on CYCLOTRON_LED_PIN
  CHAIN_EXP1 = 2,      // Expansion port 1 on EXPANSION1_LED_PIN (ESP32 only)
  CHAIN_EXP2 = 3       // Expansion port 2 on EXPANSION2_LED_PIN (ESP32 only)
};

/*
 * LED Segment Identifiers
 *
 * Each segment represents a distinct logical device or section of LEDs within a physical chain.
 * Segments are mapped to Lighting library slots (0..DEVICE_SLOTS-1) by LightingManager.
 *
 * Segment to Lighting Library Device Slot Mapping:
 *   SEGMENT_POWERCELL      = slot 0 (CHAIN_PACK, offset 0)
 *   SEGMENT_CYCLOTRON_LID  = slot 1 (CHAIN_PACK, offset MAX_POWERCELL_LED_COUNT)
 *   SEGMENT_NFILTER        = slot 2 (CHAIN_PACK, offset i_powercell_num_leds + i_cyclotron_num_leds)
 *   SEGMENT_INNER_PANEL    = slot 3 (CHAIN_CYCLOTRON, offset 0)
 *   SEGMENT_INNER_CAKE     = slot 4 (CHAIN_CYCLOTRON, offset INNER_CYCLOTRON_LED_PANEL_MAX)
 *   SEGMENT_INNER_CAVITY   = slot 5 (CHAIN_CYCLOTRON, offset PANEL_MAX + CAKE_MAX)
 */
enum LED_SEGMENT : uint8_t {
  // CHAIN_PACK segments
  SEGMENT_POWERCELL = 0,
  SEGMENT_CYCLOTRON_LID = 1,
  SEGMENT_NFILTER = 2,

  // CHAIN_CYCLOTRON segments
  SEGMENT_INNER_PANEL = 3,
  SEGMENT_INNER_CAKE = 4,
  SEGMENT_INNER_CAVITY = 5
};

/**
 * Visualization of the device and segment structure:
 * 
  {
    CHAINS:{
      CHAIN_PACK: {
        HARDWARE_PIN: PACK_LED_PIN,
        LED_COUNT: MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT,
        SEGMENTS: {
          POWERCELL: {
            COUNT_MAX: MAX_POWERCELL_LED_COUNT,
            COUNT_CONFIG: i_powercell_num_leds,
            SEGMENT_RANGE: [0, i_powercell_num_leds - 1],
            CUSTOM_COLOR: <led_rgb>, // Currently, CUSTOM_POWERCELL
          },
          CYCLOTRON_LID: {
            COUNT_MAX: OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT,
            COUNT_CONFIG: i_cyclotron_num_leds + i_nfilter_jewel_leds,
            SEGMENT_RANGE: [i_powercell_num_leds, MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT - 1],
            CUSTOM_COLOR: <led_rgb>, // Currently, CUSTOM_CYC_LID
          }
        }
      },
      CHAIN_CYCLOTRON: {
        HARDWARE_PIN: CYCLOTRON_LED_PIN,
        LED_COUNT: INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX,
        SEGMENTS: {
          INNER_CYLOTRON_PANEL: {
            COUNT_MAX: INNER_CYCLOTRON_LED_PANEL_MAX,
            COUNT_CONFIG: i_inner_cyclotron_panel_num_leds, // INNER_CYCLOTRON_LED_PANEL_MAX
            SEGMENT_RANGE: [0, INNER_CYCLOTRON_LED_PANEL_MAX - 1]
          },
          INNER_CYCLOTRON_CAKE: {
            COUNT_MAX: INNER_CYCLOTRON_CAKE_LED_MAX,
            COUNT_CONFIG: i_inner_cyclotron_cake_num_leds,
            SEGMENT_RANGE: [i_ic_panel_end + 1, i_ic_cake_start + INNER_CYCLOTRON_CAKE_LED_MAX - 1],
            CUSTOM_COLOR: <led_rgb>, // Currently, CUSTOM_CYC_CAKE
          },
          INNER_CYCLOTRON_CAVITY: {
            COUNT_MAX: INNER_CYCLOTRON_CAVITY_LED_MAX,
            COUNT_CONFIG: i_inner_cyclotron_cavity_num_leds,
            SEGMENT_RANGE: [i_ic_cake_end + 1, i_ic_cavity_start + INNER_CYCLOTRON_CAVITY_LED_MAX - 1]
          }
        }
      }
    }
  }
*/

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
 *   LightingManager::getInstance(SEGMENT_POWERCELL).setPixelColor(...)
 *   LightingManager::getInstance(SEGMENT_INNER_CAKE).setCustomColorHSV(...)
 *
 * Segment → Device Slot Mapping:
 *   SEGMENT_POWERCELL      = slot 0
 *   SEGMENT_CYCLOTRON_LID  = slot 1
 *   SEGMENT_NFILTER        = slot 2
 *   SEGMENT_INNER_PANEL    = slot 3
 *   SEGMENT_INNER_CAKE     = slot 4
 *   SEGMENT_INNER_CAVITY   = slot 5
 *
 * Segment → Hardware Chain Mapping:
 *   SEGMENT_POWERCELL, CYCLOTRON_LID, NFILTER → CHAIN_PACK
 *   SEGMENT_INNER_PANEL, CAKE, CAVITY         → CHAIN_CYCLOTRON
 *
 * INTERFACE:
 * - initializeDriver() — Sets up the driver library and hardware pins
 * - show() — Updates physical LEDs with current buffer state
 * - lightsOff() — Blanks all LEDs on the current segment
 * - setBrightness(brightness) — Controls global brightness (0-255)
 * - setPixelColor(index, ColorID, brightness) — Set LED with automatic color order
 * - getPixelColor(index) — Read a single LED's current color as LED_RGB
 * - setCustomColorHSV(hsv) — Store custom HSV for this segment's custom color slot
 * - setColorOrder(colorOrder) — Set color channel order for this segment
 * - fillPalette(palette, speedMultiplier) — Animate segment with palette
 */
class LightingManager {
private:
  static LightingManager* instance;
  Lighting lightingLib;
  #ifdef ESP32
    static inline int8_t pxl8Pins[8] = {CYCLOTRON_LED_PIN, PACK_LED_PIN, EXPANSION1_LED_PIN, EXPANSION2_LED_PIN, -1, -1, -1, -1};
    Adafruit_NeoPXL8 cyclotronLEDs;
    Adafruit_NeoPXL8 packLEDs;
    Adafruit_NeoPXL8 exp1LEDs;
    Adafruit_NeoPXL8 exp2LEDs;
  #else
    Adafruit_NeoPixel cyclotronLEDs;
    Adafruit_NeoPixel packLEDs;
  #endif
  uint8_t currentDeviceSlot; // Lighting library slot (0..DEVICE_SLOTS-1)
  LED_CHAIN currentChain;    // Physical hardware chain context

  // Private constructor - called only once by getInstance()
  // Initializes the Lighting library as lightingLib with DEVICE_SLOTS total segments.
  LightingManager() :
    lightingLib(DEVICE_SLOTS, DEVICE_REFRESH_MS),
    #ifdef ESP32
      cyclotronLEDs(CYCLOTRON_LED_COUNT, pxl8Pins, NEO_RGB + NEO_KHZ800),
      packLEDs(PACK_LED_COUNT, pxl8Pins, NEO_RGB + NEO_KHZ800),
      exp1LEDs(EXP1_LED_COUNT, pxl8Pins, NEO_RGB + NEO_KHZ800),
      exp2LEDs(EXP2_LED_COUNT, pxl8Pins, NEO_RGB + NEO_KHZ800),
    #else
      cyclotronLEDs(CYCLOTRON_LED_COUNT, CYCLOTRON_LED_PIN, NEO_RGB + NEO_KHZ800),
      packLEDs(PACK_LED_COUNT, PACK_LED_PIN, NEO_RGB + NEO_KHZ800),
    #endif
    currentDeviceSlot(0), currentChain(CHAIN_PACK) {}

  // Helper: Maps an LED_SEGMENT to its physical hardware CHAIN
  LED_CHAIN segmentToChain(LED_SEGMENT segment) const {
    switch(segment) {
      case SEGMENT_POWERCELL:
      case SEGMENT_CYCLOTRON_LID:
      case SEGMENT_NFILTER:
        return CHAIN_PACK;

      case SEGMENT_INNER_PANEL:
      case SEGMENT_INNER_CAKE:
      case SEGMENT_INNER_CAVITY:
        return CHAIN_CYCLOTRON;

      default:
        return CHAIN_PACK;
    }
  }

  // Helper: Returns the physical strip object for the given hardware chain.
  #ifdef ESP32
    Adafruit_NeoPXL8& getDevicePixels(LED_CHAIN chain) {
  #else
    Adafruit_NeoPixel& getDevicePixels(LED_CHAIN chain) {
  #endif
    switch(chain) {
      case CHAIN_CYCLOTRON:
        return cyclotronLEDs;

      case CHAIN_PACK:
      default:
        return packLEDs;

    #ifdef ESP32
      case CHAIN_EXP1:
        return exp1LEDs;

      case CHAIN_EXP2:
        return exp2LEDs;
    #endif
    }
  }

  // Helper: Returns the LED count for the given hardware chain.
  uint16_t getCount(LED_CHAIN chain) const {
    switch(chain) {
      case CHAIN_CYCLOTRON:
        return CYCLOTRON_LED_COUNT;

      case CHAIN_PACK:
        return PACK_LED_COUNT;

    #ifdef ESP32
      case CHAIN_EXP1:
        return EXP1_LED_COUNT;

      case CHAIN_EXP2:
        return EXP2_LED_COUNT;
    #endif

      default:
        return PACK_LED_COUNT;
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
  // Singleton access via LED_SEGMENT identifier
  // Maps the segment to its lighting library slot and hardware chain,
  // returning a manager instance configured for that segment.
  static LightingManager& getInstance(LED_SEGMENT segment) {
    if(instance == nullptr) {
      instance = new LightingManager();
    }
    instance->currentDeviceSlot = segment;                          // Segment value IS the slot (0-5)
    instance->currentChain = instance->segmentToChain(segment);     // Map segment to hardware chain
    return *instance;
  }

  // Initialize LED driver
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    auto& pixels = getDevicePixels(currentChain);
    pixels.begin();
    pixels.setBrightness(DEVICE_MAX_BRIGHTNESS);
    pixels.show();
  }

  // Turn off LEDs on the current segment
  void lightsOff() {
    auto& pixels = getDevicePixels(currentChain);
    pixels.clear(); // Set all to black (off).
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    auto& pixels = getDevicePixels(currentChain);
    pixels.setBrightness(brightness);
  }

  // Set custom color HSV values in the Lighting library
  void setCustomColorHSV(const LED_HSV &hsv) {
    lightingLib.setCustomColorHSV(hsv, currentDeviceSlot);
  }

  // Set color order for this segment with standard enum mapping
  void setColorOrder(ColorOrder newColorOrder) {
    lightingLib.setColorOrder(currentDeviceSlot, newColorOrder);
  }

  // Update LED display on current chain
  void show() {
    auto& pixels = getDevicePixels(currentChain);
    pixels.show(); // Pass through to the LED driver library to update LED states.
  }

  // Returns a pixel's current color as LED_RGB
  LED_RGB getPixelColor(uint16_t index) {
    auto& pixels = getDevicePixels(currentChain);
    if(index >= 0 && index < pixels.numPixels()) {
      return unpackColor(pixels.getPixelColor(index));
    }
    return LED_RGB_BLACK; // Return black if index is out of bounds.
  }

  // Set a pixel color by ColorID and automatically apply stored color order.
  void setPixelColor(uint16_t index, ColorID colorEnum, uint8_t brightness = 255) {
    auto& pixels = getDevicePixels(currentChain);
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

  // Set a pixel color direct from an RGB triplet.
  void setPixelColor(uint16_t index, LED_RGB colorRGB) {
    auto& pixels = getDevicePixels(currentChain);
    if(index >= 0 && index < pixels.numPixels()) {
      pixels.setPixelColor(index, pixels.Color(colorRGB.r, colorRGB.g, colorRGB.b));
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
    auto& pixels = getDevicePixels(currentChain);
    uint16_t i_slot_leds = getCount(currentChain);

    // Iterate over the pixels and set the color according to the device's current state.
    for(uint16_t i_curr_led = 0; i_curr_led < i_slot_leds; i_curr_led++) {
      // Calculate position offset for this LED (0-255 distributed across strand)
      uint8_t i_phase = (i_curr_led * 255 / i_slot_leds);

      // Get interpolated palette color for the device with this LED's calculated phase.
      LED_RGB rgb = lightingLib.getPaletteColor(currentDeviceSlot, // Device slot for this instance
                                                palette, // Palette in use for color interpolation
                                                speedMultiplier, // Speed for animation (1.0-10.0)
                                                i_phase); // Calculated interpolation phase for this LED (0-255)

      // Apply the device-specific color order for the RGB values.
      LED_RGB ordered = Lighting::applyColorOrder(rgb, lightingLib.getColorOrder(currentDeviceSlot));

      // Set the given LED to the calculated, ordered RGB value.
      pixels.setPixelColor(i_curr_led, pixels.Color(ordered.r, ordered.g, ordered.b));
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
