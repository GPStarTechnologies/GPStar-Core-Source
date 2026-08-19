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

// Standard library includes for integer type definitions
#include <stdint.h>  // Provides uint8_t, uint16_t, etc.

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

// Metadata: Identifies which ColorID enum values are dynamic
constexpr bool isColorDynamic(uint8_t colorEnum) {
  return colorEnum >= 100; // Dynamic colors start at 100
}

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

/**
 * Lighting: Instance-based utility class for LED color operations.
 *
 * Each device creates its own Lighting instance to manage color state independently.
 *
 * This class provides:
 * - Standard color definitions used across all GPStar devices
 * - HSV color lookup for predefined static colors
 * - HSV color lookup for dynamic/animated colors with per-device state tracking
 * - HSV to RGB color conversion
 * - Color channel reordering for different LED strip types
 * - Brightness percentage conversion
 * - Custom static color mapping for device-specific colors (user-configured via NVS/Preferences/EEPROM)
 *
 * Color Types (all in ColorID enum):
 *   - Static colors (0-99): 24 static named colors (C_RED, C_BLUE, etc.)
 *   - Dynamic colors (100-109): 10 animated patterns (C_RAINBOW, C_REDGREEN, etc.) - state tracked per device
 *   - CustomColor: 5 device-specific colors (C_CUSTOM, C_CUSTOM_POWERCELL, etc.) - user-configured HSV values
 *
 * Example usage:
 *   // Create Lighting instance to manage 6 devices with a 6ms refresh rate (~167fps)
 *   Lighting lighting(6, 6);
 *
 *   // Static colors (no animation, no state):
 *   LED_HSV red_hsv = lighting.getColorHSV(C_RED, 255, 255);
 *   LED_RGB red_rgb = lighting.hsv2rgb(red_hsv);
 *
 *   // Dynamic/animated colors (with per-device state tracking):
 *   LED_HSV rainbow = lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
 *   LED_RGB rgb = lighting.hsv2rgb(rainbow);
 *
 *   // Custom static colors (user-configured HSV values, no animation):
 *   LED_HSV custom_hsv_color = {192, 200, 255};  // From NVS/Preferences/EEPROM
 *   lighting.setCustomColorHSV(custom_hsv_color);           // Uses default device slot 0
 *   lighting.setCustomColorHSV(custom_hsv_color, 1);        // Uses device slot 1
 *   LED_HSV custom = lighting.getCustomColorHSV(0);  // Retrieve later
 *
 *   // Apply color ordering for GRB strips:
 *   LED_RGB grb = lighting.applyColorOrder(rgb, ORDER_GRB);
 */
class Lighting {
  private:
    // Instance variables for device count and state tracking
    uint8_t numDevices;
    uint16_t deviceRefreshMs;     // Device refresh rate in milliseconds (6ms, 8ms, 16ms, etc.)
    uint8_t* dynamicCounter;      // Array[numDevices] - frame counter for animated colors
    uint8_t* dynamicHue;          // Array[numDevices] - current hue for dynamic patterns
    uint8_t* dynamicBright;       // Array[numDevices] - current brightness for fading effects
    int16_t* dynamicNextBright;   // Array[numDevices] - target brightness for fade animations
    LED_HSV* customColorHSV;      // Array[numDevices] - user-configured HSV values for custom static colors (C_CUSTOM, C_CUSTOM_POWERCELL, etc.)
    ColorOrder* deviceColorOrder; // Array[numDevices] - color channel order for each device (RGB, GRB, GBR)
    uint16_t* paletteIndex;       // Array[numDevices] - smooth position through palette (0-65535) for sub-pixel interpolation
    uint8_t* paletteAnimationCycle; // Array[numDevices] - frame counter for palette animation timing

    // Helper method to get static color definitions
    LED_HSV getStaticColorDefinition(ColorID color) const;

    // Helper method to get cycle value for dynamic color animations (frame-based timing lookup)
    uint8_t getCycleValueForColor(ColorID color);

  public:
    /**
     * Constructor: Initialize Lighting instance for a set number of devices.
     * Parameters:
     *   deviceCount: Number of independent devices this Lighting object manages (1-6 typical)
     *   refreshRateMs: Device refresh rate in milliseconds (default: 6ms for Wand/Pack, ~167fps)
     * Examples:
     *   Lighting lighting(6, 8);  // ProtonPack with 6 devices, 8ms refresh rate
     *   Lighting lighting(1);     // Single device, defaults to 6ms (Wand/Pack)
     */
    Lighting(uint8_t deviceCount = 1, uint16_t refreshRateMs = 6);

    /**
     * Destructor: Clean up dynamically allocated state arrays.
     */
    ~Lighting();

    /**
     * Reset all dynamic color state to initial values for all devices.
     */
    void resetDynamicColors();

    /**
     * Get HSV color values for standard (static) colors.
     * Parameters:
     *   color: ColorID enum value
     *   brightness: 0-255 (default: 255 = full brightness)
     *   saturation: 0-255 (default: 255 = full saturation)
     * Returns: LED_HSV with hue, saturation, and brightness
     * Example: LED_HSV blue = lighting.getColorHSV(C_BLUE, 200, 255);
     */
    LED_HSV getColorHSV(ColorID color, uint8_t brightness = 255, uint8_t saturation = 255) const;

    /**
     * Get HSV color values for dynamic (animated) colors.
     * Parameters:
     *   color: ColorID - Which animation pattern (values 100-109 for dynamic colors)
     *   brightness: [0-255] - Target brightness (may be overridden by fade effects)
     *   deviceSlot: [0-255] - Device slot for multi-device instances (default: 0)
     *   color: ColorID enum for the dynamic color animation
     *   brightness: [0-255] - LED brightness (default: 255)
     *   saturation: [0-255] - Color saturation (default: 255)
     * Returns: LED_HSV for the current animation frame
     * Note: Each Lighting instance manages animations independently.
     * For multi-device setups, each device should pass its own deviceSlot to maintain independent state.
     * Example: LED_HSV hsv = lighting.getDynamicColorHSV(deviceSlot, C_RAINBOW, 255, 255);
     */
    LED_HSV getDynamicColorHSV(uint8_t deviceSlot = 0, ColorID color = C_WHITE, uint8_t brightness = 255, uint8_t saturation = 255);

    /**
     * Set a custom static color HSV value for a device slot.
     * Used for user-configured custom colors where the actual HSV values come from device preferences/NVS/EEPROM.
     * These are NOT animated - they are static user-configured colors.
     * Parameters:
     *   hsv: LED_HSV - The static HSV value to use for this device
     *   deviceSlot: [0..numDevices-1] - Which device slot (default: 0 for single-slot projects)
     * Example:
     *   LED_HSV barrel_color = {32, 200, 255};  // From NVS preferences
     *   lighting.setCustomColorHSV(barrel_color);     // Uses device 0
     *   lighting.setCustomColorHSV(barrel_color, 2);  // Uses device 2
     */
    void setCustomColorHSV(const LED_HSV &hsv, uint8_t deviceSlot = 0);

    /**
     * Get the currently stored custom static color HSV value for a device slot.
     * Returns: LED_HSV value previously set by setCustomColorHSV()
     */
    LED_HSV getCustomColorHSV(uint8_t deviceSlot = 0) const;

    /**
     * Set the color channel order for a specific device slot.
     * Different LED strips use different channel orderings (RGB, GRB, GBR).
     * Parameters:
     *   deviceSlot: [0..numDevices-1] - Which device slot
     *   order: ColorOrder enum value (ORDER_RGB, ORDER_GRB, ORDER_GBR)
     * Example:
     *   lighting.setColorOrder(0, ORDER_RGB);  // Device 0 uses RGB order
     *   lighting.setColorOrder(1, ORDER_GRB);  // Device 1 uses GRB order
     */
    void setColorOrder(uint8_t deviceSlot = 0, ColorOrder order = ORDER_RGB);

    /**
     * Get the color channel order for a specific device slot.
     * Returns: ColorOrder enum value (ORDER_RGB, ORDER_GRB, ORDER_GBR)
     * Example:
     *   ColorOrder order = lighting.getColorOrder(0);
     */
    ColorOrder getColorOrder(uint8_t deviceSlot = 0) const;

    /**
     * Get an interpolated palette color with smooth animation and speed control.
     * 
     * Provides frame-based palette animation with automatic interpolation between
     * adjacent palette colors. The palette index advances each frame with fractional
     * precision, based on device refresh rate and a speed multiplier.
     * 
     * Parameters:
     *   deviceSlot: [0..numDevices-1] - Which device to animate
     *   palette: LED_Palette16 - 16-color palette to cycle through
     *   speedMultiplier: [0.1-10.0] - Animation speed factor (default: 1.0)
     *     - 1.0 = normal speed (baseline)
     *     - 0.5 = half speed
     *     - 2.0 = double speed
     *   phaseOffset: [0-255] - Starting interpolation offset in palette for this LED (default: 0)
     *     - Used to distribute palette across multiple LEDs (e.g., LED index scaled to 0-255)
     *   brightness: [0-255] - Overall brightness level (default: 255)
     *   reverse: [true/false] - Cycle palette backwards (default: false)
     * 
     * Returns: LED_RGB color with device's stored ColorOrder already applied
     * 
     * How it works:
     * 1. Reads current paletteIndex[deviceSlot] and adds phaseOffset
     * 2. Calculates two adjacent palette color indices and interpolation fraction
     * 3. Converts both colors to HSV, interpolates between them smoothly
     * 4. Advances paletteIndex based on refresh rate and speedMultiplier (or decrements if reverse)
     * 5. Applies stored ColorOrder for device and returns final RGB
     * 
     * Example:
     *   LED_Palette16 myPalette = {{C_RED, C_GREEN, C_BLUE, ...}};
     *   LED_RGB color = lighting.getPaletteColor(0, myPalette, 1.5);  // 1.5x speed
     *   LED_RGB color = lighting.getPaletteColor(0, myPalette, 1.0, 64);  // with phase
     *   LED_RGB reversed = lighting.getPaletteColor(0, myPalette, 1.0, 0, 255, true); // reverse
     */
    LED_RGB getPaletteColor(uint8_t deviceSlot,
                            const LED_Palette16& palette,
                            float speedMultiplier = 1.0,
                            uint8_t phaseOffset = 0, 
                            uint8_t brightness = 255,
                            bool reverse = false);

    /**
     * Convert HSV color to RGB using rainbow algorithm for smooth color transitions.
     * Example: LED_RGB rgb = lighting.hsv2rgb({128, 255, 200});
     */
    static LED_RGB hsv2rgb(const LED_HSV &hsv);

    /**
     * Returns reordered RGB channels of a single color value (eg. RGB to GRB).
     * Example: LED_RGB grb = lighting.applyColorOrder(rgb, ORDER_GRB);
     */
    static LED_RGB applyColorOrder(const LED_RGB &color, ColorOrder order);

    /**
     * Convert brightness percentage (0-100) to byte value (0-255).
     * Example: uint8_t val = lighting.getBrightness(50); // Returns 127
     */
    static uint8_t getBrightness(uint8_t percent);

    /**
     * Scale RGB color by brightness factor (0-255).
     * Example: LED_RGB dimmed = lighting.scaleBrightness(rgb, 128); // 50% brightness
     */
    static LED_RGB scaleBrightness(const LED_RGB &color, uint8_t brightness);

    // Math Utilities for brightness/saturation scaling
    // These are platform-agnostic implementations from FastLED

    /**
     * Scale a byte value by another byte (0-255) with proper rounding.
     * Based on FastLED 3.10.3 scale8() with FASTLED_SCALE8_FIXED=1 (default).
     * Copyright (c) 2013 FastLED (MIT License)
     * Formula: (value * (scale + 1)) >> 8
     * Result is in range [0, 255].
     * This rounding behavior ensures no LED values are lost during scaling.
     * Example: nscale8(200, 255) → 200, nscale8(200, 128) → 100
     */
    static uint8_t nscale8(uint8_t value, uint8_t scale);

    /**
     * Scale a byte value with video-safe behavior (FastLED 3.10.3 scale8_video()).
     * Based on FastLED 3.10.3 implementation.
     * Copyright (c) 2013 FastLED (MIT License)
     * Formula: ((value * scale) >> 8) + ((value && scale) ? 1 : 0)
     * Result is in range [0, 255].
     * Preserves zero values and ensures non-zero values don't drop completely to zero,
     * preventing LED flicker/dropout during fade transitions.
     * The +1 is added only when BOTH value and scale are non-zero.
     * Example: scale8_video(200, 255) → 200, scale8_video(0, 255) → 0
     */
    static uint8_t scale8_video(uint8_t value, uint8_t scale);
};
