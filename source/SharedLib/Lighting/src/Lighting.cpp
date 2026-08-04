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
#include <cstring>  // For memset

// Constructor: Initialize Lighting instance for deviceCount devices
Lighting::Lighting(uint8_t deviceCount) : numDevices(deviceCount) {
  // Allocate state arrays for each device
  dynamicCounter = new uint8_t[numDevices];
  dynamicHue = new uint8_t[numDevices];
  dynamicBright = new uint8_t[numDevices];
  dynamicNextBright = new int16_t[numDevices];
  customColorHSV = new LED_HSV[numDevices];

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
}

// Reset all dynamic color state to initial values
void Lighting::resetDynamicColors() {
  for(uint8_t i = 0; i < numDevices; i++) {
    dynamicCounter[i] = 1;
    dynamicHue[i] = 0;
    dynamicBright[i] = 0;
    dynamicNextBright[i] = -1;
    customColorHSV[i] = {0, 0, 0};  // Default custom color: black
  }
}

// Helper method: Get static color definition (extracted from getColorHSV switch)
LED_HSV Lighting::getStaticColorDefinition(SingleColor color) {
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
      return {100, 0, 255};  // Default to white
  }
}

// Get HSV color values for standard colors.
LED_HSV Lighting::getColorHSV(SingleColor color, uint8_t brightness, uint8_t saturation) {
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
// When counter reaches the cycle value (counter % cycle == 0), the animation advances.
// Frames will be dependent on the update speed used for "show" of the LEDs.
//
// - No dependency on timing libraries (millis(), millisDelay, etc.)
// - Simpler state management - just increment a counter
// - Self-regulating speed: animation rate = LED update rate
// - If you call this every 10ms, a cycle of 50 = 500ms between changes
//
// CYCLE SPEEDS (frames between animation updates):
// - cycle = 2  (C_RAINBOW, C_PASTEL): Very fast, changes every 2 calls
// - cycle = 5  (C_AMBER_PULSE): Fast pulse
// - cycle = 7  (C_ORANGEPURPLE, C_REDPURPLE): Medium speed alternation
// - cycle = 8  (C_RED_FADE): Medium fade speed
// - cycle = 10 (C_ORANGE_FADE): Slower fade
// - cycle = 50 (C_REDGREEN, C_BLUEGREEN): Slow alternation
//
// Call this function every time you update a chain of LEDs.
LED_HSV Lighting::getDynamicColorHSV(DynamicColor color, uint8_t brightness, uint8_t saturation) {
  // For single-device instances, always use slot 0
  // For multi-device instances, this method only works for slot 0
  // Multi-device instances should create separate Lighting objects per device or use modified method
  uint8_t deviceSlot = 0;

  // Cycle rate for counter-based timing (frames between changes).
  uint8_t cycle = 2; // Initial value, each pattern sets its own cycle rate.

  switch(color) {
    case C_REDGREEN:
      // Alternate between red (0) and green (96)
      if(dynamicHue[deviceSlot] != 0 && dynamicHue[deviceSlot] != 96) {
        dynamicHue[deviceSlot] = 0; // Reset if out of range
      }

      cycle = 50;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 0) ? 96 : 0;
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_REDGREEN

    case C_ORANGEPURPLE:
      // Alternate between orange (15) and purple (210)
      if(dynamicHue[deviceSlot] != 15 && dynamicHue[deviceSlot] != 210) {
        dynamicHue[deviceSlot] = 15; // Reset if out of range
      }

      cycle = 7;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 15) ? 210 : 15;
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_ORANGEPURPLE

    case C_BLUEGREEN:
      // Alternate between blue (145) and green (96)
      if(dynamicHue[deviceSlot] != 145 && dynamicHue[deviceSlot] != 96) {
        dynamicHue[deviceSlot] = 145; // Reset if out of range
      }

      cycle = 50;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 96) ? 145 : 96;
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_BLUEGREEN

    case C_REDPURPLE:
      // Alternate between red (0) and purple (210)
      if(dynamicHue[deviceSlot] != 0 && dynamicHue[deviceSlot] != 210) {
        dynamicHue[deviceSlot] = 0; // Reset if out of range
      }

      cycle = 7;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == 0) ? 210 : 0;
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_REDPURPLE

    case C_AMBER_PULSE:
      // Pulse between amber (24) and orange (32)
      if(dynamicHue[deviceSlot] < 20 || dynamicHue[deviceSlot] > 32) {
        dynamicHue[deviceSlot] = 24; // Reset if out of range
        dynamicNextBright[deviceSlot] = 1; // Start incrementing
      }

      cycle = 5;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] += dynamicNextBright[deviceSlot];

        // Reverse direction at boundaries
        if(dynamicHue[deviceSlot] >= 32) {
          dynamicNextBright[deviceSlot] = -1;
        }
        else if(dynamicHue[deviceSlot] <= 20) {
          dynamicNextBright[deviceSlot] = 1;
        }

        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_AMBER_PULSE

    case C_ORANGE_FADE:
      // Fade brightness on orange hue (28)
      if(dynamicBright[deviceSlot] == 0) {
        dynamicBright[deviceSlot] = 50;
        dynamicNextBright[deviceSlot] = 5; // Start incrementing
      }

      cycle = 10;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicBright[deviceSlot] += dynamicNextBright[deviceSlot];

        // Reverse direction at boundaries
        if(dynamicBright[deviceSlot] >= 250) {
          dynamicNextBright[deviceSlot] = -5;
        }
        else if(dynamicBright[deviceSlot] <= 50) {
          dynamicNextBright[deviceSlot] = 5;
        }

        dynamicCounter[deviceSlot] = 1;
      }

      return {28, 255, dynamicBright[deviceSlot]};
    // END C_ORANGE_FADE

    case C_RED_FADE:
      // Fade brightness on red hue (0)
      if(dynamicBright[deviceSlot] == 0) {
        dynamicBright[deviceSlot] = 50;
        dynamicNextBright[deviceSlot] = 5; // Start incrementing
      }

      cycle = 8;
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicBright[deviceSlot] += dynamicNextBright[deviceSlot];

        // Reverse direction at boundaries
        if(dynamicBright[deviceSlot] >= 250) {
          dynamicNextBright[deviceSlot] = -5;
        }
        else if(dynamicBright[deviceSlot] <= 50) {
          dynamicNextBright[deviceSlot] = 5;
        }

        dynamicCounter[deviceSlot] = 1;
      }

      return {0, 255, dynamicBright[deviceSlot]};
    // END C_RED_FADE

    case C_BLUE_FADE:
      // Fade hue from dark blue (160) to light blue (146)
      // Used by ProtonPack Power Cell (15-LED RGB strip)
      if(dynamicHue[deviceSlot] < 146 || dynamicHue[deviceSlot] > 160) {
        dynamicHue[deviceSlot] = 160; // Reset if out of range
      }

      cycle = 1; // Decrement hue every frame for smooth fade
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot]--;
        
        // Wrap around if we go below minimum
        if(dynamicHue[deviceSlot] < 146) {
          dynamicHue[deviceSlot] = 160;
        }
        
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_BLUE_FADE

    case C_PASTEL:
      // Cycle through all hues (0-255) at half saturation
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] + 5) % 256;
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 128, brightness};
    // END C_PASTEL

    case C_RAINBOW:
    default:
      // Cycle through all hues (0-255) at full saturation
      dynamicCounter[deviceSlot]++;

      if(dynamicCounter[deviceSlot] % cycle == 0) {
        dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] + 5) % 256;
        dynamicCounter[deviceSlot] = 1;
      }

      return {dynamicHue[deviceSlot], 255, brightness};
    // END C_RAINBOW
  }
}

// Set a custom static color HSV value for the device
void Lighting::setCustomColorHSV(uint8_t deviceSlot, CustomColor color, const LED_HSV &hsv) {
  // For single-device instances, always use slot 0
  // Multi-device instances should override this behavior if needed
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  customColorHSV[deviceSlot] = hsv;
}

// Get the currently stored custom color HSV value for the device
LED_HSV Lighting::getCustomColorHSV(uint8_t deviceSlot) const {
  if(deviceSlot >= numDevices) {
    deviceSlot = 0;
  }
  
  return customColorHSV[deviceSlot];
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
