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

// Library Header
#include <Lighting.h>
#include <string.h>  // For memset (portable across Arduino platforms)

// Constructor: Initialize Lighting instance for deviceCount devices with specified refresh rate
Lighting::Lighting(uint8_t deviceCount, uint16_t refreshRateMs) 
  : numDevices(deviceCount > 0 ? deviceCount : 1),
    deviceRefreshMs(refreshRateMs >= 5 ? refreshRateMs : 5) {
  // Allocate state arrays for each device
  dynamicCounter = new uint8_t[numDevices];
  dynamicHue = new uint8_t[numDevices];
  dynamicBright = new uint8_t[numDevices];
  dynamicNextBright = new int16_t[numDevices];
  customColorHSV = new LED_HSV[numDevices];
  deviceColorOrder = new ColorOrder[numDevices];
  paletteIndex = new uint16_t[numDevices];
  paletteAnimationCycle = new uint8_t[numDevices];

  // Initialize all arrays to default values
  resetDynamicColors();
}

// Destructor: Clean up dynamically allocated arrays
Lighting::~Lighting() {
  delete[] dynamicCounter;
  delete[] dynamicHue;
  delete[] dynamicBright;
  delete[] dynamicNextBright;
  delete[] customColorHSV;
  delete[] deviceColorOrder;
  delete[] paletteIndex;
  delete[] paletteAnimationCycle;
}

// Reset all dynamic color state to initial values
void Lighting::resetDynamicColors() {
  for(uint8_t i = 0; i < numDevices; i++) {
    dynamicCounter[i] = 1;
    dynamicHue[i] = 0;
    dynamicBright[i] = 0;
    dynamicNextBright[i] = -1;
    customColorHSV[i] = {0, 0, 0};  // Default custom color: black
    deviceColorOrder[i] = ORDER_RGB;  // Default color order
    paletteIndex[i] = 0;  // Start at beginning of palette
    paletteAnimationCycle[i] = 0;  // Start at frame 0
  }
}

// Helper method: Get static color definition (extracted from getColorHSV switch)
LED_HSV Lighting::getStaticColorDefinition(ColorID color) {
  switch(color) {
    case C_WHITE:
      return {100, 0, 255};  // White = no saturation, full brightness
    
    case C_BLACK:
      return {0, 0, 0};  // Black = all zeros
    
    case C_WARM_WHITE:
      return {36, 183, 255};
    
    case C_PINK:
      return {244, 255, 255};
    
    case C_PASTEL_PINK:
      return {244, 128, 255};
    
    case C_RED:
      return {0, 255, 255};
    
    case C_LIGHT_RED:
      return {0, 192, 255};
    
    case C_RED2:
      return {5, 255, 255};
    
    case C_RED3:
      return {10, 255, 255};
    
    case C_RED4:
      return {15, 255, 255};
    
    case C_RED5:
      return {20, 255, 255};
    
    case C_ORANGE:
      return {32, 255, 255};
    
    case C_BEIGE:
      return {43, 128, 255};
    
    case C_YELLOW:
      return {64, 255, 255};
    
    case C_CHARTREUSE:
      return {80, 255, 255};
    
    case C_GREEN:
      return {96, 255, 255};
    
    case C_DARK_GREEN:
      return {96, 255, 128};
    
    case C_MINT:
      return {112, 120, 255};
    
    case C_AQUA:
      return {128, 255, 255};
    
    case C_LIGHT_BLUE:
      return {145, 255, 255};
    
    case C_MID_BLUE:
      return {160, 255, 255};
    
    case C_NAVY_BLUE:
      return {170, 200, 112};
    
    case C_BLUE:
      return {180, 255, 255};
    
    case C_PURPLE:
      return {192, 255, 255};
    
    default:
      // WARNING: Do NOT recursively call getDynamicColorHSV() here as it will cause an infinite loop!
      return {100, 0, 255};  // Default to white
  }
}

// Helper method: Get cycle value for dynamic color animations (frame-based timing lookup)
//
// This centralized lookup table makes it easy to adjust animation speeds across the codebase.
// All animation timing is defined here, using frame-based cycles instead of real-time delays.
//
// TIMING RELATIONSHIP:
// actual_time_between_changes = cycle_value × LED_update_interval
//
// EXAMPLES WITH STANDARDIZED INTERVALS:
// If cycle = 50 and LED updates every:
//   - 5ms  (NeutronaWand, PSTT, ProtonPack):  50 × 5ms = 250ms
//   - 8ms  (Attenuator, StreamEffects):       50 × 8ms = 400ms
//   - 16ms (BeltGizmo, SingleShot):           50 × 16ms = 800ms
//
// CYCLE SPEEDS (frames between animation updates):
// - cycle = 1  (C_BLUE_FADE): Fastest, changes every frame
//   - At 5ms:  5ms   | At 8ms:  8ms    | At 16ms: 16ms
// - cycle = 6  (DEFAULT, C_RAINBOW, C_PASTEL, C_AMBER_PULSE): Fast, changes every 6 calls
//   - At 5ms:  30ms  | At 8ms:  48ms   | At 16ms: 96ms
// - cycle = 8  (C_RED_FADE): Medium fade speed
//   - At 5ms:  40ms  | At 8ms:  64ms   | At 16ms: 128ms
// - cycle = 10 (C_ORANGE_FADE): Slower fade
//   - At 5ms:  50ms  | At 8ms:  80ms   | At 16ms: 160ms
// - cycle = 50 (C_REDGREEN, C_BLUEGREEN, C_ORANGEPURPLE, C_REDPURPLE): Slow alternation
//   - At 5ms: 250ms  | At 8ms: 400ms   | At 16ms: 800ms
//
// CHOOSING CYCLE VALUES:
// - Smaller values (1-4) = rapid changes, best for streaming/smooth fades
// - Medium values (5-10) = perceptible pulses, good for status indicators
// - Large values (50+) = slow alternations, good for calm breathing effects
uint8_t Lighting::getCycleValueForColor(ColorID color) {
  switch(color) {
    case C_REDGREEN:
    case C_BLUEGREEN:
    case C_ORANGEPURPLE:
    case C_REDPURPLE:
      return 50;  // Slowest alternation (250-800ms at standardized intervals)

    case C_ORANGE_FADE:
      return 10;  // Slower fade (50-160ms at standardized intervals)

    case C_RED_FADE:
      return 8;   // Medium fade speed (40-128ms at standardized intervals)

    case C_AMBER_PULSE:
      return 6;   // Fast pulse (30-96ms at standardized intervals)

    case C_RAINBOW:
    case C_PASTEL:
      return 6;   // Fast transitions (30-96ms at standardized intervals)

    case C_BLUE_FADE:
      return 1;   // Fastest - smooth continuous fade (every frame)

    default:
      return 6;   // Default: fast transitions (30-96ms at standardized intervals)
  }
}

// Get HSV color values for standard colors.
LED_HSV Lighting::getColorHSV(ColorID color, uint8_t brightness, uint8_t saturation) {
  // Returns LED_HSV with appropriate hue, saturation, and brightness.
  // Some colors override saturation or brightness with fixed values.

  LED_HSV result = getStaticColorDefinition(color);
  
  // Apply requested brightness and saturation (unless they're fixed by the color definition)
  if(color != C_BLACK) {  // Black always stays black (don't override brightness)
    result.v = brightness;
  }
  
  // Only override saturation for colors that don't have fixed saturation
  switch(color) {
    case C_WHITE:
    case C_PASTEL_PINK:
    case C_LIGHT_RED:
    case C_BEIGE:
    case C_DARK_GREEN:
    case C_MINT:
    case C_NAVY_BLUE:
      // These have fixed saturation - don't override
      break;
    
    default:
      // All other colors use requested saturation
      result.s = saturation;
      break;
  }
  
  return result;
}

// Convert brightness percentage (0-100) to byte value (0-255).
uint8_t Lighting::getBrightness(uint8_t percent) {
  // Brightness as percentage (0-100), converted to range 0-255
  if(percent > 100) {
    percent = 100;
  }
  return (uint8_t)((255 * percent) / 100);
}

// Scale RGB color by brightness factor (0-255).
LED_RGB Lighting::scaleBrightness(const LED_RGB &color, uint8_t brightness) {
  // Scale each channel by brightness factor (0-255)
  LED_RGB result;
  result.r = (uint8_t)((color.r * brightness) / 255);
  result.g = (uint8_t)((color.g * brightness) / 255);
  result.b = (uint8_t)((color.b * brightness) / 255);
  return result;
}

// Color Channel Ordering

// Apply color channel ordering for different LED strip types.
LED_RGB Lighting::applyColorOrder(const LED_RGB &color, ColorOrder order) {
  switch(order) {
    case ORDER_GRB:
      // Swap red and green channels
      return {color.g, color.r, color.b};

    case ORDER_GBR:
      // Rotate channels: G->R, B->G, R->B
      return {color.g, color.b, color.r};

    case ORDER_RGB:
    default:
      return color;
  }
}

// Convert HSV color to RGB color using a standard rainbow algorithm.
LED_RGB Lighting::hsv2rgb(const LED_HSV &hsv) {
  LED_RGB rgb;

  // If saturation is 0, the color is a shade of gray
  if(hsv.s == 0) {
    rgb.r = hsv.v;
    rgb.g = hsv.v;
    rgb.b = hsv.v;
    return rgb;
  }

  // Divide hue into 6 regions (0-5) for the color wheel
  // Each region is 256/6 ≈ 43 hue units
  uint8_t region = hsv.h / 43;
  uint8_t remainder = (hsv.h - (region * 43)) * 6;

  // Calculate intermediate values for smooth transitions
  uint8_t p = (hsv.v * (255 - hsv.s)) >> 8;
  uint8_t q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
  uint8_t t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

  // Map region to RGB values for rainbow effect
  switch(region) {
    case 0:
      rgb.r = hsv.v;
      rgb.g = t;
      rgb.b = p;
    break;
    case 1:
      rgb.r = q;
      rgb.g = hsv.v;
      rgb.b = p;
    break;
    case 2:
      rgb.r = p;
      rgb.g = hsv.v;
      rgb.b = t;
    break;
    case 3:
      rgb.r = p;
      rgb.g = q;
      rgb.b = hsv.v;
    break;
    case 4:
      rgb.r = t;
      rgb.g = p;
      rgb.b = hsv.v;
    break;
    case 5:
    default:
      rgb.r = hsv.v;
      rgb.g = p;
      rgb.b = q;
    break;
  }

  return rgb;
}

// Get HSV color values for dynamic/animated colors
//
// This function uses frame counting instead of real-time delays (avoiding timers).
// Each call to this function = 1 frame. The dynamicCounter[] increments every call.
// When counter reaches the cycle value (from getCycleValueForColor), the animation advances.
// Frames are dependent on the update speed used for "show" of the LEDs.
//
// - No dependency on timing libraries (millis(), millisDelay, etc.)
// - Simpler state management - just increment a counter
// - Self-regulating speed: animation rate = LED update rate
// - Frame-based timing allows consistent behavior across different update intervals
//
// The actual timing for each animation is defined in getCycleValueForColor() lookup table.
// Call this function every time you update a chain of LEDs.
LED_HSV Lighting::getDynamicColorHSV(uint8_t deviceSlot, ColorID color, uint8_t brightness, uint8_t saturation) {
  // Validate deviceSlot is within range, default to 0 if out of bounds
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }

  // Get cycle rate for this color using centralized lookup table
  uint8_t cycle = getCycleValueForColor(color);
  
  // Variable for dynamic brightness adjustments (used by some animations)
  uint8_t dynamicBrightness = brightness;

  switch(color) {
    case C_REDGREEN:
      // Alternate between red (0) and dark green (96)
      if(dynamicHue[deviceSlot] != 0 && dynamicHue[deviceSlot] != 96) {
        dynamicHue[deviceSlot] = 0; // Reset if out of range
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 0) ? 96 : 0;
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      // Use darker brightness for green to match C_DARK_GREEN visual weight.
      dynamicBrightness = (dynamicHue[deviceSlot] == 96) ? (brightness / 2) : brightness;
      return {dynamicHue[deviceSlot], 255, dynamicBrightness};
    // END C_REDGREEN

    case C_ORANGEPURPLE:
      // Alternate between orange (15) and purple (210)
      if(dynamicHue[deviceSlot] != 15 && dynamicHue[deviceSlot] != 210) {
        dynamicHue[deviceSlot] = 15; // Reset if out of range
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 15) ? 210 : 15;
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_ORANGEPURPLE

    case C_BLUEGREEN:
      // Alternate between blue (145) and green (96)
      if(dynamicHue[deviceSlot] != 145 && dynamicHue[deviceSlot] != 96) {
        dynamicHue[deviceSlot] = 145; // Reset if out of range
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 96) ? 145 : 96;
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_BLUEGREEN

    case C_REDPURPLE:
      // Alternate between red (0) and purple (210)
      if(dynamicHue[deviceSlot] != 0 && dynamicHue[deviceSlot] != 210) {
        dynamicHue[deviceSlot] = 0; // Reset if out of range
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 0) ? 210 : 0;
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_REDPURPLE

    case C_AMBER_PULSE:
      // Pulse between amber (24) and orange (32)
      if(dynamicHue[deviceSlot] < 20 || dynamicHue[deviceSlot] > 32) {
        dynamicHue[deviceSlot] = 24; // Reset if out of range
        dynamicNextBright[deviceSlot] = 1; // Start incrementing
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        // Let uint8_t overflow/underflow work naturally for hue (circular color wheel)
        dynamicHue[deviceSlot] += (uint8_t)dynamicNextBright[deviceSlot];

        // Reverse direction at boundaries
        if(dynamicHue[deviceSlot] >= 32) {
          dynamicNextBright[deviceSlot] = -1;
        }
        else if(dynamicHue[deviceSlot] <= 20) {
          dynamicNextBright[deviceSlot] = 1;
        }

        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_AMBER_PULSE

    case C_ORANGE_FADE:
      // Fade brightness on orange hue (28)
      if(dynamicBright[deviceSlot] == 0) {
        dynamicBright[deviceSlot] = 50;
        dynamicNextBright[deviceSlot] = 5; // Start incrementing
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        // Sanity check: use int16_t to prevent overflow, then clamp to valid range
        int16_t newBright = (int16_t)dynamicBright[deviceSlot] + dynamicNextBright[deviceSlot];
        if(newBright > 255) newBright = 255;  // Prevent overflow
        if(newBright < 0) newBright = 0;      // Prevent underflow
        dynamicBright[deviceSlot] = (uint8_t)newBright;

        // Reverse direction at boundaries
        if(dynamicBright[deviceSlot] >= 250) {
          dynamicNextBright[deviceSlot] = -5;
        }
        else if(dynamicBright[deviceSlot] <= 50) {
          dynamicNextBright[deviceSlot] = 5;
        }

        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {28, 255, dynamicBright[deviceSlot]};
    // END C_ORANGE_FADE

    case C_RED_FADE:
      // Fade brightness on red hue (0)
      if(dynamicBright[deviceSlot] == 0) {
        dynamicBright[deviceSlot] = 50;
        dynamicNextBright[deviceSlot] = 5; // Start incrementing
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        // Sanity check: use int16_t to prevent overflow, then clamp to valid range
        int16_t newBright = (int16_t)dynamicBright[deviceSlot] + dynamicNextBright[deviceSlot];
        if(newBright > 255) newBright = 255;  // Prevent overflow
        if(newBright < 0) newBright = 0;      // Prevent underflow
        dynamicBright[deviceSlot] = (uint8_t)newBright;

        // Reverse direction at boundaries
        if(dynamicBright[deviceSlot] >= 250) {
          dynamicNextBright[deviceSlot] = -5;
        }
        else if(dynamicBright[deviceSlot] <= 50) {
          dynamicNextBright[deviceSlot] = 5;
        }

        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {0, 255, dynamicBright[deviceSlot]};
    // END C_RED_FADE

    case C_BLUE_FADE:
      // Fade hue from dark blue (160) to light blue (146)
      // Used by ProtonPack Power Cell (15-LED RGB strip)
      if(dynamicHue[deviceSlot] < 146 || dynamicHue[deviceSlot] > 160) {
        dynamicHue[deviceSlot] = 160; // Reset if out of range
      }

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        // Let uint8_t underflow work naturally for hue (circular color wheel)
        dynamicHue[deviceSlot] -= 1;
        
        // Wrap around if we go below minimum
        if(dynamicHue[deviceSlot] < 146) {
          dynamicHue[deviceSlot] = 160;
        }
        
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_BLUE_FADE

    case C_PASTEL:
      // Cycle through all hues (0-255) at half saturation
      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] + 5) % 256;
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 128, brightness};
    // END C_PASTEL

    case C_RAINBOW:
      // Cycle through all hues (0-255) at full saturation
      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] + 5) % 256;
        dynamicCounter[deviceSlot] = 1;
      }
      else {
        dynamicCounter[deviceSlot]++;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_RAINBOW

    case C_CUSTOM:
      // Return the custom static color stored for this device slot.
      // The deviceSlot parameter defaults to 0 for single-device projects,
      // but multi-device projects can specify which slot to retrieve.
      return getCustomColorHSV(deviceSlot);
    // END C_CUSTOM

    default:
      // Unknown color, fall back to static color definition via getColorHSV()
      // which will safely returns a static color (or defaults to white).
      return getColorHSV(color, brightness, saturation);
  }
}

// Set a custom static color HSV value for the device (default slot 0 for single-device projects).
void Lighting::setCustomColorHSV(const LED_HSV &hsv, uint8_t deviceSlot) {
  // For single-device instances, always use slot 0
  // Multi-device instances should override this behavior if needed
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  customColorHSV[deviceSlot] = hsv;
}

// Get the currently stored custom color HSV value for the device (default slot 0 for single-device projects).
LED_HSV Lighting::getCustomColorHSV(uint8_t deviceSlot) const {
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  return customColorHSV[deviceSlot];
}

// Set the color channel order for a specific device slot.
void Lighting::setColorOrder(uint8_t deviceSlot, ColorOrder order) {
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  deviceColorOrder[deviceSlot] = order;
}

// Get the color channel order for a specific device slot.
ColorOrder Lighting::getColorOrder(uint8_t deviceSlot) const {
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  return deviceColorOrder[deviceSlot];
}

// Get an interpolated palette color with smooth animation and speed control.
LED_RGB Lighting::getPaletteColor(uint8_t deviceSlot,
                                  const LED_Palette16& palette, 
                                  float speedMultiplier,
                                  uint8_t phaseOffset, 
                                  uint8_t brightness,
                                  bool reverse) {
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  // Clamp speed multiplier to reasonable range (0.1x to 10x)
  if(speedMultiplier < 0.1f) speedMultiplier = 0.1f;
  if(speedMultiplier > 10.0f) speedMultiplier = 10.0f;

  // Calculate animation increment using normalization formula
  // baseIncrement = 5 (5ms baseline = 200fps, minimum refresh rate across all devices)
  // refreshRateMs = device refresh rate in ms (5ms, 8ms, 16ms, etc.)
  // increment = (5 * 256 * speedMultiplier) / refreshRateMs
  // We use fixed-point (16-bit) to preserve fractional movement across all refresh rates
  const uint16_t BASE_INCREMENT = 5;  // Reference baseline (5ms = 200fps, minimum device refresh rate)
  uint16_t increment = (uint16_t)((BASE_INCREMENT * 256 * speedMultiplier) / deviceRefreshMs);
  
  // Advance palette index with full fractional precision (16-bit accumulation)
  // This preserves sub-pixel movement even on slower refresh rates
  paletteAnimationCycle[deviceSlot]++;
  if(reverse) {
    paletteIndex[deviceSlot] -= increment;  // Reverse: decrement instead of increment
  } else {
    paletteIndex[deviceSlot] += increment;  // Normal: increment position (full 16-bit value)
  }
  
  // Get current palette position (0-255 maps to 16 colors) with position offset applied
  // Upper 8 bits of paletteIndex map to visible palette position (0-255)
  uint8_t currentPos = (paletteIndex[deviceSlot] >> 8) + phaseOffset;
  
  // Calculate which two palette colors to interpolate between
  uint8_t palettePos = (currentPos >> 4);  // Map 0-255 to 0-15 (16 palette colors)
  uint8_t nextPos = (palettePos + 1) & 0x0F;  // Wrap around at 16
  uint8_t fraction = currentPos & 0x0F;  // Interpolation fraction (0-15)
  
  // Get the two adjacent palette colors as HSV
  LED_HSV hsv1 = getColorHSV(palette.colors[palettePos], brightness);
  LED_HSV hsv2 = getColorHSV(palette.colors[nextPos], brightness);
  
  // Interpolate between the two HSV colors
  // For smooth animation, interpolate all three components
  uint8_t interpH = hsv1.h + ((int16_t)(hsv2.h - hsv1.h) * fraction / 16);
  uint8_t interpS = hsv1.s + ((int16_t)(hsv2.s - hsv1.s) * fraction / 16);
  uint8_t interpV = hsv1.v + ((int16_t)(hsv2.v - hsv1.v) * fraction / 16);
  
  LED_HSV interpolated = {interpH, interpS, interpV};
  
  // Convert to RGB
  LED_RGB rgb = hsv2rgb(interpolated);
  
  // Apply stored color order for this device
  return applyColorOrder(rgb, deviceColorOrder[deviceSlot]);
}

// ============================================================================
// Math Utilities
// ============================================================================

// Scale a byte value by another byte (0-255) with proper rounding.
// Based on FastLED 3.10.3's scale8() with FASTLED_SCALE8_FIXED=1 (default).
// Copyright (c) 2013 FastLED (MIT License)
// Formula: (value * (scale + 1)) >> 8
// This rounding behavior ensures no LED values are lost during scaling.
uint8_t Lighting::nscale8(uint8_t value, uint8_t scale) {
  uint16_t result = (uint16_t)value * ((uint16_t)scale + 1);
  return (uint8_t)(result >> 8);
}

// Scale a byte value with video-safe behavior (FastLED 3.10.3's scale8_video()).
// Based on FastLED 3.10.3's scale8_video() implementation.
// Copyright (c) 2013 FastLED (MIT License)
// Preserves zero values and ensures non-zero values don't drop completely to zero.
// Formula: ((value * scale) >> 8) + ((value && scale) ? 1 : 0)
// This prevents LED flicker/dropout during fade transitions by adding 1 only when
// both value and scale are non-zero, maintaining minimum brightness without
// lighting up completely black pixels.
uint8_t Lighting::scale8_video(uint8_t value, uint8_t scale) {
  // Apply scale with bit-shift, then add 1 if both value and scale are non-zero
  // This ensures LEDs don't fade completely to black during dimming transitions
  uint8_t scaled = (uint16_t)value * (uint16_t)scale >> 8;
  
  // Add 1 if both value and scale are non-zero
  // (This condition matches FastLED's video-safe behavior)
  if(value && scale) {
    scaled++;
  }

  return scaled;
}
