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

// Suppress FastLED warnings
#define FASTLED_INTERNAL

// Include the intended LED driver first: FastLED
#include <FastLED.h>

// Include the generalized Lighting library
#include <Lighting.h>

// Forward declarations for variables from Configuration.h
extern uint8_t i_powercell_num_leds;
extern uint8_t i_cyclotron_num_leds;

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
 */
enum LED_CHAIN {
  CHAIN_PACK = 0,           // Power Cell + Outer Cyclotron + N-Filter on PACK_LED_PIN
  CHAIN_CYCLOTRON = 1,      // Inner Panel + Cake + Cavity on CYCLOTRON_LED_PIN
  CHAIN_EXP1 = 2,           // Expansion port 1 on EXPANSION1_LED_PIN (ESP32 only)
  CHAIN_EXP2 = 3            // Expansion port 2 on EXPANSION2_LED_PIN (ESP32 only)
};

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
#define DEVICE_MAX_BRIGHTNESS 255 // Use full-brightness for optimal effect

/*
 * The FastLED library disables interrupts when it changes values on addressable LEDs.
 * This takes ~30μs (about 30 microseconds) per LED for changes for common 3-wire circuits,
 * such as the WS281* type LEDs which are commonly used across this kit. Essentially this
 * means "mo' lights, mo' problems" in terms of disrupting things like serial data.
 * https://github.com/FastLED/FastLED/wiki/Interrupt-problems
 *
 * In the case of the Proton Pack we have 2 chains of addressable LEDs:
 *  1) The "pack" lights which consist of the Powercell, Cyclotron, and N-Filter.
 *  2) The inner cyclotron "cake" plus anything beyond that point.
 *
 * So for every 100 LEDs at 30μs each to update, that's 3ms of interrupt disruption. For
 * a microcontroller that's a lot of time so we need to keep those updates to a minimum.
 * The best way to do that while still providing all of the lights desired is to keep those
 * chains of lights to a minimum where possible. Thus, we only support a certain # of LEDs.
 */

/*
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
 * Proton Pack Power Cell and Cyclotron lid LED pin.
 */
CRGB pack_leds[MAX_POWERCELL_LED_COUNT + OUTER_CYCLOTRON_LED_MAX + JEWEL_NFILTER_LED_COUNT];

/*
 * Inner Cyclotron LEDs (optional).
 * Max number of LEDs supported = 64.
 * Maximum expected LEDs for the Inner Switch Panel is 8.
 * Maximum allowed LEDs for the Inner Cyclotron Cake is 36.
 * Maximum allowed LEDs for the Inner Cyclotron Cavity is 20.
 */
CRGB cyclotron_leds[INNER_CYCLOTRON_LED_PANEL_MAX + INNER_CYCLOTRON_CAKE_LED_MAX + INNER_CYCLOTRON_CAVITY_LED_MAX];

/*
 * Delay for fastled to update the addressable LEDs.
 * We have up to 126 addressable LEDs if using NeoPixel jewel in the N-Filter, a ring
 * for the Inner Cyclotron, and the optional "sparking" cyclotron cavity LEDs.
 * 0.0312 ms to update each LED, then a 0.05 ms resting period once all are updated.
 * So 4 ms should be okay. Let's bump it up to 5 just in case.
 * For cyclotrons with high density LEDs, increase this based on the cyclotron speed multiplier to simulate a faster spinning cyclotron.
 * This works by "skipping frames" in the animation, which can be done up until about 15 ms.
 * After 15ms it will become painfully obvious to most people that the animation is not smooth.
 */
#define LED_DRIVER_UPDATE_MS 5
uint8_t i_led_update_delay = LED_DRIVER_UPDATE_MS;
millisDelay ms_led_driver;

// ============================================================================
// LIGHTING LIBRARY CONFIGURATION & INITIALIZATION
// ============================================================================

/**
 * LocalLightingManager - Abstraction Layer for LED Driver Operations
 *
 * PURPOSE:
 * This class provides a driver-agnostic interface for all LED operations.
 * Instead of directly calling FastLED functions throughout the codebase,
 * all LED control flows through this manager. This design allows us to
 * swap the underlying LED driver without touching application logic.
 *
 * PATTERN:
 * LocalLightingManager uses the SINGLETON pattern. There is only ONE
 * instance of this class for the entire program. Access it via:
 *   LocalLightingManager::getInstance()
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
 * - getLEDs() — Returns pointer to LED array for direct manipulation
 *
 * DRIVER ABSTRACTION:
 * The methods here wrap driver-specific calls. Whenever we need to
 * support a different LED library, we only modify this class, not the
 * caller code. This keeps the rest of the application clean and portable.
 */
class LocalLightingManager {
private:
  static LocalLightingManager* instance;
  Lighting lightingLib;

  // Private constructor - called only once by getInstance()
  LocalLightingManager() : lightingLib(6) {
    // Initialize with 6 devices (POWERCELL, CYCLOTRON_OUTER, CYCLOTRON_INNER, CYCLOTRON_CAVITY, CYCLOTRON_PANEL, VENT_LIGHT)
  }

public:
  // Singleton instance
  static LocalLightingManager& getInstance() {
    if(instance == nullptr) {
      instance = new LocalLightingManager();
    }
    return *instance;
  }

  // Initialize LED driver for dual chains
  // Sets up addressable LED communication and default brightness
  void initializeDriver() {
    // PACK chain: Power Cell + Outer Cyclotron + N-Filter Jewel
    FastLED.addLeds<NEOPIXEL, PACK_LED_PIN>(pack_leds, i_pack_num_leds).setCorrection(TypicalLEDStrip);
    
    // CYCLOTRON chain: Inner Panel + Cake + Cavity
    #ifdef ESP32
      FastLED.addLeds<NEOPIXEL, CYCLOTRON_LED_PIN>(cyclotron_leds, i_max_inner_cyclotron_leds).setCorrection(TypicalLEDStrip);
    #else
      FastLED.addLeds<NEOPIXEL, CYCLOTRON_LED_PIN>(cyclotron_leds, i_max_inner_cyclotron_leds).setCorrection(TypicalLEDStrip);
    #endif
    
    FastLED.setMaxRefreshRate(0); // Disable FastLED's blocking 2.5ms delay.
    FastLED.setBrightness(DEVICE_MAX_BRIGHTNESS);
    FastLED.show(); // Update all addressable LEDs to prevent stale LED states.
  }

  // Get color as RGB based on device and color enum
  CRGB getColorRGB(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    return CRGB(rgb.r, rgb.g, rgb.b);
  }

  CRGB getColorGRB(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GRB: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.r, rgb.b }
    return CRGB(rgb.g, rgb.r, rgb.b);
  }

  CRGB getColorGBR(uint8_t device, uint8_t colorEnum, uint8_t brightness = 255) {
    auto hsv = lightingLib.getColorHSV((SingleColor)colorEnum, brightness);
    auto rgb = Lighting::hsv2rgb(hsv);
    // Swap to GBR: { rgb.r, rgb.g, rgb.b } -> { rgb.g, rgb.b, rgb.r }
    return CRGB(rgb.g, rgb.b, rgb.r);
  }

  // Update LED display
  void show() {
    FastLED.show(); // Pass through to the LED driver library to update LED states.
  }

  // Turn off all LEDs on specified chain
  void lightsOff(LED_CHAIN chain = CHAIN_PACK) {
    switch(chain) {
      case CHAIN_PACK:
        fill_solid(pack_leds, i_pack_num_leds, CRGB::Black);
      break;

      case CHAIN_CYCLOTRON:
        fill_solid(cyclotron_leds, i_max_inner_cyclotron_leds, CRGB::Black);
      break;

      case CHAIN_EXP1:
      case CHAIN_EXP2:
        // Expansion chains not yet configured; add support when buffers are defined
      break;
    }
  }

  // Get a pointer to the LED array for specified chain
  CRGB* getLEDs(LED_CHAIN chain = CHAIN_PACK) {
    switch(chain) {
      case CHAIN_CYCLOTRON:
        return cyclotron_leds;
      break;

      case CHAIN_EXP1:
      case CHAIN_EXP2:
        // Expansion chains not yet configured; return nullptr for now
        return nullptr;
      break;

      case CHAIN_PACK:
      default:
        return pack_leds;
      break;
    }
  }

  // Set brightness
  void setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
  }
};

/**
 * SINGLETON PATTERN: Static member variable initialization
 *
 * This line MUST exist outside the class definition for any static member.
 * It allocates memory for the single instance pointer and initializes it to nullptr.
 *
 * The actual LocalLightingManager object is NOT created here—it's created lazily on
 * the FIRST call to getInstance(), which checks if instance is nullptr, creates it if
 * needed, then returns a reference to it. Subsequent calls return the same instance.
 *
 * This ensures only ONE LocalLightingManager exists for the entire program.
 */
LocalLightingManager* LocalLightingManager::instance = nullptr;
