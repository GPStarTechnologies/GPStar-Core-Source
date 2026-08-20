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

// Library Structs/ENUMs
#include <LightingBasics.h>

/**
 * Lighting: Instance-based utility class for LED color operations.
 *
 * Each device creates its own Lighting instance to manage color state independently.
 *
 * This class provides:
 *   - Standard color definitions used across all GPStar devices
 *   - HSV color lookup for predefined static colors
 *   - HSV color lookup for dynamic/animated colors with per-device state tracking
 *   - HSV to RGB color conversion
 *   - Color channel reordering for different LED strip types
 *   - Brightness percentage conversion
 *   - Custom static color mapping for device-specific colors (user-configured via NVS/Preferences/EEPROM)
 *
 * Color Types (all in ColorID enum):
 *   - Static colors (0-99): 24 static named colors (C_RED, C_BLUE, etc.)
 *   - Dynamic colors (100-109): 10 animated patterns (C_RAINBOW, C_REDGREEN, etc.) - state tracked per device
 *   - Custom color (254): Single user-configured HSV value per device slot (C_CUSTOM)
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
 *
 * See Lighting.cpp ANIMATION_CONFIGS[] array for dynamic color configuration.
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

    // Helper method to get animation configuration for a dynamic color
    const AnimationConfig& getAnimationConfig(ColorID color) const;

    // Helper methods for each animation mode
    LED_HSV animateAlternate(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness);
    LED_HSV animateFade(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness);
    LED_HSV animatePulse(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness);
    LED_HSV animateCycleHue(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness);
    LED_HSV animateDecayHue(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness);

  public:
    /**
     * isColorDynamic: Static utility to check if a ColorID is dynamic (animated).
     * Parameters:
     *   colorEnum: ColorID value to test
     * Returns: true if colorEnum >= 100 (dynamic color range), false otherwise
     * Example: if(Lighting::isColorDynamic(C_RAINBOW)) { animated pattern }
     */
    static constexpr bool isColorDynamic(uint8_t colorEnum) {
      return colorEnum >= 100; // Dynamic colors start at 100
    }

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
     *     - 0.5 = half speed
     *     - 1.0 = normal speed (baseline)
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
