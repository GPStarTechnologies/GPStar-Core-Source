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

// Standard library headers
#include <stdint.h>  // uint8_t, uint16_t, int16_t (portable across Arduino and desktop platforms)
#include <string.h>  // memset

// Library Header
#include <Lighting.h>

// Constructor: Initialize Lighting instance for deviceCount devices with specified refresh rate (default: 5ms).
Lighting::Lighting(uint8_t deviceCount, uint16_t refreshRateMs) 
  : numDevices(deviceCount > 0 ? deviceCount : 1),
    deviceRefreshMs(refreshRateMs >= 5 ? refreshRateMs : 5) {
  // Allocate state arrays for each device slot.
  customColorHSV = new LED_HSV[numDevices];
  deviceColorOrder = new ColorOrder[numDevices];
  dynamicCounter = new uint8_t[numDevices];
  dynamicHue = new uint8_t[numDevices];
  dynamicBright = new uint8_t[numDevices];
  dynamicNextBright = new int16_t[numDevices];
  paletteIndex = new uint16_t[numDevices];
  paletteAnimationCycle = new uint8_t[numDevices];

  // Initialize all arrays to default values
  resetDynamicColors();
}

// Destructor: Clean up dynamically allocated arrays
Lighting::~Lighting() {
  delete[] customColorHSV;
  delete[] deviceColorOrder;
  delete[] dynamicCounter;
  delete[] dynamicHue;
  delete[] dynamicBright;
  delete[] dynamicNextBright;
  delete[] paletteIndex;
  delete[] paletteAnimationCycle;
}

// Reset all dynamic color state to initial values
void Lighting::resetDynamicColors() {
  for(uint8_t i = 0; i < numDevices; i++) {
    customColorHSV[i] = {0, 0, 0};  // Default custom color: black
    deviceColorOrder[i] = ORDER_RGB;  // Default color order: RGB (for 3-wire WS2811/WS2812)
    dynamicCounter[i] = 0;  // Frame counter starts at 0 (animations advance on first call)
    dynamicHue[i] = 0;  // Default hue: red (0) (animations set on first call)
    dynamicBright[i] = 0;  // Default brightness: none (animations set on first call)
    dynamicNextBright[i] = -1;  // Direction for brightness fading: uninitialized
    paletteIndex[i] = 0;  // Start at beginning of palette
    paletteAnimationCycle[i] = 0;  // Start at frame 0
  }
}

// Helper method: Get static color definition (extracted from getColorHSV switch)
LED_HSV Lighting::getStaticColorDefinition(ColorID color) const {
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

// Animation config for all dynamic colors. Each entry explicitly maps a ColorID to its animation config.
//
// Mode Enumeration Meanings:
//   ANIM_ALTERNATE:  value1/value2 = hues to flip between
//   ANIM_FADE:       value1/value2 = brightness min/max, fixedHue = hue to fade on
//   ANIM_PULSE:      value1/value2 = hue min/max, reverses at boundaries
//   ANIM_CYCLE_HUE:  value2 = hue increment/frame, saturation = fixed sat level
//   ANIM_DECAY_HUE:  value1 = start hue, value2 = end hue, decays then wraps
//
// Special Field Meanings:
//   fixedHue: Only used by ANIM_FADE — which hue (0-255) to fade brightness on
//   saturation: Only used by ANIM_CYCLE_HUE — fixed saturation level (128=pastel, 255=vivid)
//   adjustBrightness: Only used by ANIM_ALTERNATE — darken value2 by 50% (e.g., green in C_REDGREEN)
//
static constexpr AnimationMapping ANIMATION_CONFIGS[] = {
// {colorId,       {cycleMs, mode,          value1, value2, fixedHue, saturation, adjustBright}},
  {C_REDGREEN,     {240,  ANIM_ALTERNATE,        0,     96,       0,       255,        true}},
  {C_ORANGEPURPLE, {240,  ANIM_ALTERNATE,       15,    210,       0,       255,       false}},
  {C_BLUEGREEN,    {240,  ANIM_ALTERNATE,      145,     96,       0,       255,       false}},
  {C_REDPURPLE,    {240,  ANIM_ALTERNATE,        0,    210,       0,       255,       false}},
  {C_AMBER_PULSE,  { 48,  ANIM_PULSE,           24,     32,       0,       255,       false}},
  {C_BLUE_FADE,    {  6,  ANIM_DECAY_HUE,      160,    146,       0,       255,       false}},
  {C_ORANGE_FADE,  { 48,  ANIM_FADE,            50,    255,      28,       255,       false}},
  {C_RED_FADE,     { 48,  ANIM_FADE,            50,    255,       0,       255,       false}},
  {C_PASTEL,       { 48,  ANIM_CYCLE_HUE,        0,      5,       0,       128,       false}},
  {C_RAINBOW,      { 48,  ANIM_CYCLE_HUE,        0,      5,       0,       255,       false}},
};

// Helper method: Get animation configuration for a dynamic color
const AnimationConfig& Lighting::getAnimationConfig(ColorID color) const {
  // Search through the explicit ColorID-to-AnimationConfig mappings
  for(const auto& mapping : ANIMATION_CONFIGS) {
    if(mapping.colorId == color) {
      return mapping.config;
    }
  }
  // Not found, return C_RAINBOW as default (should not happen in production)
  for(const auto& mapping : ANIMATION_CONFIGS) {
    if(mapping.colorId == C_RAINBOW) {
      return mapping.config;
    }
  }
  // Fallback (should never reach here)
  return ANIMATION_CONFIGS[0].config;
}

// Get HSV color values for standard colors.
LED_HSV Lighting::getColorHSV(ColorID color, uint8_t brightness, uint8_t saturation) const {
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
    case ORDER_RGB:
    default:
      return color;

    case ORDER_GRB:
      // Swap red and green channels
      return {color.g, color.r, color.b};

    case ORDER_GBR:
      // Rotate channels: G->R, B->G, R->B
      return {color.g, color.b, color.r};

    case ORDER_RBG:
      // Swap blue and green channels
      return {color.r, color.b, color.g};

    case ORDER_BRG:
      // Rotate channels: B->R, R->G, G->B
      return {color.b, color.r, color.g};

    case ORDER_BGR:
      // Reverse channels: B->R, G stays, R->B
      return {color.b, color.g, color.r};
  }
}

// Animation Helper Methods
// Each method handles one animation mode. All follow the same frame-based counter pattern:
// if(dynamicCounter[slot] % cycle == 0) { advance state; reset counter to 1; }
// else { increment counter; }

// ANIM_ALTERNATE: Discrete flip between two values (no interpolation)
LED_HSV Lighting::animateAlternate(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness) {
  if(dynamicHue[deviceSlot] != cfg.value1 && dynamicHue[deviceSlot] != cfg.value2) {
    dynamicHue[deviceSlot] = cfg.value1;  // Initialize
  }

  if(dynamicCounter[deviceSlot] % cycle == 0) {
    dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] == cfg.value1) ? cfg.value2 : cfg.value1;
    dynamicCounter[deviceSlot] = 1;
  } else {
    dynamicCounter[deviceSlot]++;
  }

  uint8_t displayBrightness = brightness;
  if(cfg.adjustBrightness && dynamicHue[deviceSlot] == cfg.value2) {
    displayBrightness = brightness / 2;  // Darken green on C_REDGREEN
  }

  return {dynamicHue[deviceSlot], 255, displayBrightness};
}

// ANIM_FADE: Continuous fade between value1 and value2, reversing at boundaries
LED_HSV Lighting::animateFade(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness) {
  if(dynamicBright[deviceSlot] == 0) {
    dynamicBright[deviceSlot] = cfg.value1;
    dynamicNextBright[deviceSlot] = (cfg.value2 > cfg.value1) ? 5 : -5;
  }

  if(dynamicCounter[deviceSlot] % cycle == 0) {
    int16_t newBright = (int16_t)dynamicBright[deviceSlot] + dynamicNextBright[deviceSlot];
    if(newBright > cfg.value2) { newBright = cfg.value2; dynamicNextBright[deviceSlot] = -5; }
    if(newBright < cfg.value1) { newBright = cfg.value1; dynamicNextBright[deviceSlot] = 5; }
    dynamicBright[deviceSlot] = (uint8_t)newBright;
    dynamicCounter[deviceSlot] = 1;
  } else {
    dynamicCounter[deviceSlot]++;
  }

  // Use fixed hue from config (e.g., 28 for orange, 0 for red)
  return {cfg.fixedHue, 255, dynamicBright[deviceSlot]};
}

// ANIM_PULSE: Fade that reverses at boundaries (breathing effect)
LED_HSV Lighting::animatePulse(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness) {
  if(dynamicHue[deviceSlot] < cfg.value1 || dynamicHue[deviceSlot] > cfg.value2) {
    dynamicHue[deviceSlot] = cfg.value1;
    dynamicNextBright[deviceSlot] = 1;  // Increment direction
  }

  if(dynamicCounter[deviceSlot] % cycle == 0) {
    dynamicHue[deviceSlot] += (uint8_t)dynamicNextBright[deviceSlot];

    // Reverse direction at boundaries
    if(dynamicHue[deviceSlot] >= cfg.value2) {
      dynamicNextBright[deviceSlot] = -1;
    } else if(dynamicHue[deviceSlot] <= cfg.value1) {
      dynamicNextBright[deviceSlot] = 1;
    }

    dynamicCounter[deviceSlot] = 1;
  } else {
    dynamicCounter[deviceSlot]++;
  }

  return {dynamicHue[deviceSlot], 255, brightness};
}

// ANIM_CYCLE_HUE: Smooth hue progression through full color wheel
LED_HSV Lighting::animateCycleHue(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness) {
  if(dynamicCounter[deviceSlot] % cycle == 0) {
    dynamicHue[deviceSlot] = (dynamicHue[deviceSlot] + cfg.value2) % 256;  // value2 = hue increment per cycle
    dynamicCounter[deviceSlot] = 1;
  } else {
    dynamicCounter[deviceSlot]++;
  }

  // Use saturation from config (128 for PASTEL, 255 for RAINBOW)
  return {dynamicHue[deviceSlot], cfg.saturation, brightness};
}

// ANIM_DECAY_HUE: Hue that moves in one direction with wrap (smooth fade between two hues)
LED_HSV Lighting::animateDecayHue(uint8_t deviceSlot, const AnimationConfig& cfg, uint8_t cycle, uint8_t brightness) {
  if(dynamicHue[deviceSlot] < cfg.value2 || dynamicHue[deviceSlot] > cfg.value1) {
    dynamicHue[deviceSlot] = cfg.value1;  // Reset if out of range
  }

  if(dynamicCounter[deviceSlot] % cycle == 0) {
    dynamicHue[deviceSlot] -= 1;  // Decay hue downward
    
    // Wrap around if we go below minimum
    if(dynamicHue[deviceSlot] < cfg.value2) {
      dynamicHue[deviceSlot] = cfg.value1;
    }
    
    dynamicCounter[deviceSlot] = 1;
  } else {
    dynamicCounter[deviceSlot]++;
  }

  return {dynamicHue[deviceSlot], 255, brightness};
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
// When counter reaches the cycle value (calculated from millisecond config), the animation advances.
// Frames are dependent on the update speed used for "show" of the LEDs.
//
// - No dependency on timing libraries (millis(), millisDelay, etc.)
// - Simpler state management - just increment a counter
// - Self-regulating speed: animation rate = LED update rate
// - Frame-based timing allows consistent behavior across different update intervals
//
// The actual timing for each animation is defined in ANIMATION_CONFIGS[] lookup table.
// Configuration is millisecond-based, automatically converted to frame cycles via:
//   cycle = animationConfig.cycleMs / deviceRefreshMs
// Call this function every time you update a chain of LEDs.
LED_HSV Lighting::getDynamicColorHSV(uint8_t deviceSlot, ColorID color, uint8_t brightness, uint8_t saturation) {
  // Validate deviceSlot is within range, default to 0 if out of bounds
  if(deviceSlot >= numDevices) {
    return getColorHSV(color, brightness, saturation);
  }

  // Handle custom color (special case - not animated)
  if(color == C_CUSTOM) {
    return getCustomColorHSV(deviceSlot);
  }

  // Handle static colors (anything below dynamic color range)
  if(color < 100) {
    return getColorHSV(color, brightness, saturation);
  }

  // Get animation configuration for this dynamic color
  const AnimationConfig& cfg = getAnimationConfig(color);
  
  // Convert millisecond-based timing to frame cycles based on device refresh rate
  // Minimum 1 frame to ensure animation progresses
  uint8_t cycle = (cfg.cycleMs / deviceRefreshMs);
  if(cycle < 1) cycle = 1;

  // Dispatch to animation helper based on mode
  switch(cfg.mode) {
    case ANIM_ALTERNATE:
      return animateAlternate(deviceSlot, cfg, cycle, brightness);
    
    case ANIM_FADE:
      return animateFade(deviceSlot, cfg, cycle, brightness);
    
    case ANIM_PULSE:
      return animatePulse(deviceSlot, cfg, cycle, brightness);
    
    case ANIM_CYCLE_HUE:
      return animateCycleHue(deviceSlot, cfg, cycle, brightness);
    
    case ANIM_DECAY_HUE:
      return animateDecayHue(deviceSlot, cfg, cycle, brightness);
    
    default:
      // Unknown animation mode, return safe default (white)
      return getColorHSV(C_WHITE, brightness, saturation);
  }
}

// Set a custom static color HSV value for the device (default slot 0 for single-device projects).
void Lighting::setCustomColorHSV(const LED_HSV &hsv, uint8_t deviceSlot) {
  // For single-device instances, always use slot 0
  // Multi-device instances should override this behavior if needed
  if(deviceSlot < numDevices) {
    customColorHSV[deviceSlot] = hsv;
  }
}

// Get the currently stored custom color HSV value for the device (default slot 0 for single-device projects).
LED_HSV Lighting::getCustomColorHSV(uint8_t deviceSlot) const {
  if(deviceSlot < numDevices) {
    return customColorHSV[deviceSlot];
  } else {
    return getColorHSV(C_WHITE, 255, 255); // Return white as a default value.
  }
}

// Set the color channel order for a specific device slot.
void Lighting::setColorOrder(uint8_t deviceSlot, ColorOrder order) {
  if(deviceSlot < numDevices) {
    deviceColorOrder[deviceSlot] = order;
  }
}

// Get the color channel order for a specific device slot.
ColorOrder Lighting::getColorOrder(uint8_t deviceSlot) const {
  if(deviceSlot < numDevices) {
    return deviceColorOrder[deviceSlot];
  } else {
    return ORDER_RGB;
  }
}

// Get an interpolated palette color with smooth animation and speed control.
LED_RGB Lighting::getPaletteColor(uint8_t deviceSlot,
                                  const LED_Palette16& palette, 
                                  float speedMultiplier,
                                  uint8_t phaseOffset, 
                                  uint8_t brightness,
                                  bool reverse) {
  if(deviceSlot >= numDevices) {
    return LED_RGB_BLACK;
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
