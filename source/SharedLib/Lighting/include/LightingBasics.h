/**
 *   Lighting - LED color abstraction utilities for GPStar devices.
 *   Provides LED-library-independent color types and conversions.
 *   Copyright (C) 2023-2026 Michael Rajotte, Dustin Grau, Nomake Wan
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

// Standard library headers for integer type definitions
#include <stdint.h>  // uint8_t, uint16_t, int16_t (portable across Arduino and desktop platforms)

// LED_RGB: Platform-independent RGB color representation.
// Example: LED_RGB red = {255, 0, 0};
struct LED_RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  // Comparison operators for checking if two RGB colors are identical
  bool operator==(const LED_RGB& other) const {
    return r == other.r && g == other.g && b == other.b;
  }
  bool operator!=(const LED_RGB& other) const {
    return !(*this == other);
  }
};

// LED_RGB default constants (defined outside struct to avoid incomplete type issues)
constexpr LED_RGB LED_RGB_BLACK = {0, 0, 0};
constexpr LED_RGB LED_RGB_WHITE = {255, 255, 255};

// ColorOrder: Set the order of the RGB values as they will be sent to the LED device.
// GPStar devices are WS2812 which is set by "NEO_GRB" at the hardware level, but we
// will control the assembly of the LED_RGB triplets via the software for convenience.
enum ColorOrder : uint8_t {
  ORDER_RGB = 0,  // Red-Green-Blue
  ORDER_GRB = 1,  // Green-Red-Blue
  ORDER_GBR = 2,  // Green-Blue-Red
  ORDER_RBG = 3,  // Red-Blue-Green
  ORDER_BRG = 4,  // Blue-Red-Green
  ORDER_BGR = 5   // Blue-Green-Red
};

// ColorID: All colors (static and dynamic) in a single unified enum.
// Static colors: 0-99 (fixed HSV values, no animation state)
// Dynamic colors: 100+ (animated/stateful colors with frame-based animation)
enum ColorID : uint8_t {
  // --- Static colors (0-99) ---
  C_BLACK = 0,
  C_WHITE = 1,
  C_WARM_WHITE = 2,        // 36
  C_PINK = 3,              // 244
  C_PASTEL_PINK = 4,       // 244
  C_RED = 5,               // 0
  C_LIGHT_RED = 6,         // 0
  C_RED2 = 7,              // 5
  C_RED3 = 8,              // 10
  C_RED4 = 9,              // 15
  C_RED5 = 10,             // 20
  C_ORANGE = 11,           // 32
  C_BEIGE = 12,            // 43
  C_YELLOW = 13,           // 64
  C_CHARTREUSE = 14,       // 80
  C_GREEN = 15,            // 96
  C_DARK_GREEN = 16,       // 96
  C_MINT = 17,             // 112
  C_AQUA = 18,             // 128
  C_LIGHT_BLUE = 19,       // 145
  C_MID_BLUE = 20,         // 160
  C_NAVY_BLUE = 21,        // 170
  C_BLUE = 22,             // 180
  C_PURPLE = 23,           // 192
  // --- Dynamic colors (100+) ---
  C_REDGREEN = 100,
  C_ORANGEPURPLE = 101,
  C_BLUEGREEN = 102,
  C_REDPURPLE = 103,
  C_AMBER_PULSE = 104,
  C_BLUE_FADE = 105,
  C_ORANGE_FADE = 106,
  C_RED_FADE = 107,
  C_PASTEL = 108,
  C_RAINBOW = 109,
  // --- Custom color (set per device slot) ---
  C_CUSTOM = 254
};

/**
 * LED_Palette16: A platform-independent 16-color palette.
 * 
 * This provides a simple, driver-agnostic container using the LED_RGB type.
 * Palettes distribute colors across pixels for transitions and lighting effects.
 */
struct LED_Palette16 {
  ColorID colors[16];
};

// LED_HSV: Platform-independent HSV color representation.
// Example: LED_HSV cyan = {128, 255, 200}; (hue, saturation, brightness)
struct LED_HSV {
  uint8_t h; // Hue: 0-255 (0=red, 85=green, 170=blue)
  uint8_t s; // Saturation: 0-255 (0=white, 255=full color)
  uint8_t v; // Value (brightness): 0-255
};

// LED_HSV default constants (defined outside struct to avoid incomplete type issues)
constexpr LED_HSV LED_HSV_BLACK = {0, 0, 0};
constexpr LED_HSV LED_HSV_WHITE = {0, 0, 255};

// AnimationMode: Type of animation behavior for dynamic colors
// Each mode defines how the animation advances and what state variables control the effect
enum AnimationMode : uint8_t {
  ANIM_ALTERNATE,      // Discrete flip between two values (Red ↔ Green, no interpolation)
  ANIM_FADE,           // Continuous fade between min/max brightness or hue (50-255 or value1-value2)
  ANIM_PULSE,          // Fade that reverses at boundaries (breathing effect)
  ANIM_CYCLE_HUE,      // Smooth hue progression through full color wheel (Rainbow, Pastel)
  ANIM_DECAY_HUE       // Hue that moves in one direction with wrap (Blue fade between two hues)
};

/**
 * AnimationConfig: Complete configuration for a dynamic color animation.
 * 
 * Stores all parameters needed to control an animation's behavior and timing.
 * One config per dynamic ColorID, loaded from centralized lookup table.
 * 
 * Fields:
 *   cycleMs: Milliseconds between animation state changes (explicit, device-independent timing)
 *   mode: AnimationMode - How the animation advances (alternate, fade, pulse, cycle, decay)
 *   value1: Primary value (hue or brightness start, or color 1)
 *   value2: Secondary value (hue or brightness end, or color 2 / increment amount)
 *   fixedHue: Optional fixed hue (for ANIM_FADE animations on specific color)
 *   saturation: Optional saturation override (for ANIM_CYCLE_HUE animations)
 *   adjustBrightness: Special flag - darken green on REDGREEN alternation
 * 
 * Example:
 *   {250, ANIM_ALTERNATE, 0, 96, 0, 255, true}  // Red ↔ Green, darken green
 *   {30, ANIM_CYCLE_HUE, 0, 5, 0, 255, false}   // Rainbow, advance +5 hue, full sat
 *   {50, ANIM_FADE, 50, 255, 28, 255, false}    // Fade brightness on orange hue
 */
struct AnimationConfig {
  uint16_t cycleMs;       // Milliseconds per animation state change
  AnimationMode mode;     // Animation type (controls how state advances)
  uint8_t value1;         // Primary value (hue or brightness start, or color 1)
  uint8_t value2;         // Secondary value (hue or brightness end, or increment)
  uint8_t fixedHue;       // Fixed hue for ANIM_FADE (0=red, 28=orange, etc.)
  uint8_t saturation;     // Fixed saturation for ANIM_CYCLE_HUE (128=pastel, 255=vivid)
  bool adjustBrightness;  // Special flag: darken second value (green on REDGREEN)
};

/**
 * AnimationMapping: Explicit pairing of ColorID to its AnimationConfig.
 * Eliminates implicit indexing (color - 100) and makes the relationship clear.
 * Add new dynamic colors by simply adding a new entry with {ColorID, config}.
 */
struct AnimationMapping {
  ColorID colorId;              // Which dynamic color this config applies to
  AnimationConfig config;       // That color's animation parameters
};
