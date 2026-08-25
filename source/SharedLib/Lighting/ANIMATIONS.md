# Lighting Animation System

## Overview

The Lighting library provides both static (fixed) and dynamic (animated) color management for GPStar controllers. This document describes the differences between color types, explains animation modes and their configurations, and details how animations are timed at the device level.

**Key Concepts**: 
- **Dynamic Colors**: Frame-based animations with their own timing configuration (`cycleMs`). The Lighting library maintains separate animation state for each segment, allowing independent animation of different LED segments.
- **Palette Colors**: Interpolate smoothly between colors using the project's `DEVICE_REFRESH_MS` refresh rate plus a `speedMultiplier` for speed control. No separate timing configuration—all palette animations use the same device refresh rate.

## Color Types: Static vs Dynamic

Colors are identified using an integer-backed ENUM named `ColorID` with 2 ranges of values: 0-99 for static colors and >= 100 for dynamic colors.

### Static Colors (ColorID 0-99)

Static colors are fixed HSV values with **no animation state**. They represent:
- Fundamental colors (red, blue, green, white, etc.)
- Fully opaque derived colors (light red, navy blue, dark green, etc.)
- Desaturated colors (pastels, beige, mint)

**Characteristics**:
- Defined once in `getStaticColorDefinition()` lookup table
- No per-device state tracking required
- Brightness and saturation can be overridden per call via `getColorHSV()`
- Example: `Lighting::getColorHSV(C_RED, 200, 255)` returns a red at 200 saturation and full brightness

**Current Static Colors**:
- Reds: C_RED, C_LIGHT_RED, C_RED2, C_RED3, C_RED4, C_RED5
- Oranges/Yellows: C_ORANGE, C_BEIGE, C_YELLOW, C_CHARTREUSE
- Greens: C_GREEN, C_DARK_GREEN, C_MINT
- Blues: C_AQUA, C_LIGHT_BLUE, C_MID_BLUE, C_NAVY_BLUE, C_BLUE
- Purples: C_PURPLE, C_PINK, C_PASTEL_PINK
- Neutrals: C_WHITE, C_WARM_WHITE, C_BLACK
- Custom: C_CUSTOM (user-configured HSV value, set per device slot)

### Dynamic Colors (ColorID >= 100)

Dynamic colors are **animated patterns** with per-chain state tracking. Each chain maintains independent animation counters, hue values, brightness values, and palette positions.

**Characteristics**:
- Animated via `getDynamicColorHSV(chainIndex, colorId, brightness, saturation)`
- Require per-chain state initialized in Lighting constructor
- Animation advance controlled by frame counter and animation configuration
- All dynamic colors use frame-based timing: `(frameCounter % cycle) == 0` triggers state change
- State is preserved across calls to `getDynamicColorHSV()` for smooth animation

**Current Dynamic Colors**:

| ColorID | Name | Type | Effect |
|---------:|------|------|--------|
| C_REDGREEN (100) | Red ↔ Green | ALTERNATE | Flashes red and green (green darkened by 50%) |
| C_ORANGEPURPLE (101) | Orange ↔ Purple | ALTERNATE | Flashes between orange and purple |
| C_BLUEGREEN (102) | Blue ↔ Green | ALTERNATE | Flashes between blue and green |
| C_REDPURPLE (103) | Red ↔ Purple | ALTERNATE | Flashes between red and purple |
| C_AMBER_PULSE (104) | Amber Pulse | PULSE | Hue oscillation (pulsing effect) |
| C_BLUE_FADE (105) | Blue Fade | DECAY_HUE | Smooth blue decay fade |
| C_ORANGE_FADE (106) | Orange Fade | FADE | Smooth orange brightness fade |
| C_RED_FADE (107) | Red Fade | FADE | Smooth red brightness fade |
| C_PASTEL (108) | Pastel Rainbow | CYCLE_HUE | Desaturated rainbow cycle |
| C_RAINBOW (109) | Rainbow | CYCLE_HUE | Full saturation rainbow cycle |

## Animation Modes

All dynamic colors are configured with one of five animation modes. Each mode defines how the animation state advances.

### ANIM_ALTERNATE: Discrete Flip Between Two Values

Flips between two hue values at discrete intervals.

**Configuration Fields**:
- `value1`: First hue (0-255)
- `value2`: Second hue (0-255)
- `adjustBrightness`: If true, darken `value2` by 50% (for visual balance)
- `cycleMs`: Milliseconds between flips

**Examples**:
- **C_REDGREEN**: Alternates red (hue 0) and green (hue 96) every 240ms, darkening green
- **C_ORANGEPURPLE**: Alternates orange (hue 15) and purple (hue 210) every 240ms

### ANIM_FADE: Continuous Brightness Ramp

Fades brightness up and down on a fixed hue, reversing at min/max values.

**Configuration Fields**:
- `fixedHue`: The hue to fade on (0-255)
- `value1`: Minimum brightness (0-255)
- `value2`: Maximum brightness (0-255)
- `cycleMs`: Milliseconds per brightness step

**Examples**:
- **C_RED_FADE**: Fades red (hue 0) brightness 50-255 every 48ms
- **C_ORANGE_FADE**: Fades orange (hue 28) brightness 50-255 every 48ms

### ANIM_PULSE: Hue Oscillation (Breathing)

Oscillates hue between two values, reversing at boundaries.

**Configuration Fields**:
- `value1`: Minimum hue (0-255)
- `value2`: Maximum hue (0-255)
- `cycleMs`: Milliseconds per hue step

**Examples**:
- **C_AMBER_PULSE**: Oscillates hue 24-32 (amber range) every 48ms

### ANIM_CYCLE_HUE: Smooth Rainbow Progression

Continuously cycles through the full hue spectrum with wrapping.

**Configuration Fields**:
- `value2`: Hue increment per cycle frame (typically 5)
- `saturation`: Fixed saturation level (128 for pastel, 255 for vivid)
- `cycleMs`: Milliseconds per increment

**Examples**:
- **C_RAINBOW**: Increments hue +5 every 48ms with full saturation (255)
- **C_PASTEL**: Increments hue +5 every 48ms with pastel saturation (128)

### ANIM_DECAY_HUE: Smooth Fade Between Two Hues

Decays hue from one value to another in one direction with wrapping.

**Configuration Fields**:
- `value1`: Start hue (0-255)
- `value2`: End hue (wraps to this at min boundary)
- `cycleMs`: Milliseconds per hue step

**Examples**:
- **C_BLUE_FADE**: Decays from blue (hue 160) through cyan to blue (hue 146) creating smooth fade

## Animation Configuration Structure

Each dynamic color is defined with an `AnimationConfig` struct that controls all aspects of the animation:

```cpp
struct AnimationConfig {
  uint16_t cycleMs;              // Milliseconds between animation state changes (explicit)
  AnimationMode mode;            // Animation type (ALTERNATE, FADE, PULSE, CYCLE_HUE, DECAY_HUE)
  uint8_t value1;                // Mode-specific value 1 (hue, brightness, or increment)
  uint8_t value2;                // Mode-specific value 2 (hue, brightness, or increment)
  uint8_t fixedHue;              // Fixed hue (only used by ANIM_FADE)
  uint8_t saturation;            // Fixed saturation (only used by ANIM_CYCLE_HUE)
  bool adjustBrightness;         // Darken value2 by 50% (only used by ANIM_ALTERNATE)
};
```

**Current Animation Configurations** (from `Lighting.cpp`):

```cpp
{C_REDGREEN,     {250,  ANIM_ALTERNATE,        0,     96,       0,       255,        true}},
{C_ORANGEPURPLE, {250,  ANIM_ALTERNATE,       15,    210,       0,       255,       false}},
{C_BLUEGREEN,    {250,  ANIM_ALTERNATE,      145,     96,       0,       255,       false}},
{C_REDPURPLE,    {250,  ANIM_ALTERNATE,        0,    210,       0,       255,       false}},
{C_AMBER_PULSE,  { 30,  ANIM_PULSE,           24,     32,       0,       255,       false}},
{C_BLUE_FADE,    {  5,  ANIM_DECAY_HUE,      160,    146,       0,       255,       false}},
{C_ORANGE_FADE,  { 50,  ANIM_FADE,            50,    255,      28,       255,       false}},
{C_RED_FADE,     { 40,  ANIM_FADE,            50,    255,       0,       255,       false}},
{C_PASTEL,       { 30,  ANIM_CYCLE_HUE,        0,      5,       0,       128,       false}},
{C_RAINBOW,      { 30,  ANIM_CYCLE_HUE,        0,      5,       0,       255,       false}},
```

## Device Timing: Refresh Rates and Normalization

### What is Device Refresh Rate?

Each **project** (device/controller) defines a single `DEVICE_REFRESH_MS` constant that controls the animation loop timing for all LED segments. Each segment has:
- Its own LightingManager instance managing a chain of LEDs
- Its own animation state in the shared Lighting library
- Access to the project's single DEVICE_REFRESH_MS value
- Potentially multiple physical LED devices connected in sequence (e.g., Cyclotron panel + Trap in segments on the same GPIO pin)

### Device Refresh Rate Definition

Each **project** defines a single animation loop refresh rate in `include/LightConfig.h`:

```cpp
#define DEVICE_REFRESH_MS 6  // or 8, or 16
```

This value is passed to the Lighting library constructor:

```cpp
static Lighting lightingLib(DEVICE_COUNT, DEVICE_REFRESH_MS);
```

**Standardized Intervals (August 2026)**:

| Refresh Rate | Projects | Frame Rate | Notes |
|--------------|----------|-----------:|-------|
| **6ms** | NeutronaWand, ProtonPack | ~167 Hz | millisDelay-based task scheduling |
| **8ms** | Attenuator, StreamEffects | ~125 Hz | FreeRTOS-based task scheduling |
| **16ms** | BeltGizmo, PSTT, SingleShot | ~62.5 Hz | Lower power consumption, acceptable for status LEDs |

This single DEVICE_REFRESH_MS value applies to the entire project. The animation loop calls `LightingManager::show()` on all device segments at this interval and advances animation frame counters in the shared Lighting library for all segments.

### Frame-Based Animation Timing

Animations use **frame-based cycle counting**, not absolute time. The behavior differs between dynamic colors and palette animations:

#### Dynamic Colors: Own Timing via cycleMs

Each dynamic color's animation state is controlled by its `cycleMs` configuration value. The animation state advances at fixed frame intervals based on the cycle value calculated from `cycleMs`.

**Timing Normalization Concept**:

All `cycleMs` values are **specified as multiples of 48ms**, which is the least common multiple (LCM) of the three standardized refresh rates (6, 8, 16). This ensures clean divisibility across all projects:

```
cycle = cycleMs / DEVICE_REFRESH_MS
```

- **6ms project**: cycle = 240 / 6 = **40 frames** = 240ms ✓
- **8ms project**: cycle = 240 / 8 = **30 frames** = 240ms ✓
- **16ms project**: cycle = 240 / 16 = **15 frames** = 240ms ✓

This normalization ensures animations maintain consistent timing across all device types. The `cycleMs` field documents the intended millisecond timing; the library automatically converts it to frame cycles based on the project's `DEVICE_REFRESH_MS`.

#### Palette Animations: Refresh Rate + Speed Multiplier

Palette animations do NOT have independent timing. Instead, they use:
- **Base timing**: The project's `DEVICE_REFRESH_MS` (6ms, 8ms, or 16ms)
- **Speed adjustment**: A `speedMultiplier` parameter (0.1-10.0x) that controls interpolation speed between palette colors

```cpp
LED_RGB color = lighting.getPaletteColor(
  segmentID,
  palette,
  speedMultiplier = 1.5,  // 1.5x faster through palette
  phaseOffset = 0,
  brightness = 255,
  reverse = false
);
// Palette advances at DEVICE_REFRESH_MS rate, but traverses faster/slower based on speedMultiplier
```

**Current Cycle Values for Dynamic Colors** (from Animation Configs):

All animations maintain consistent timing across refresh rates. Frame count scales inversely with refresh rate to hit the target `cycleMs` timing:

| Animation | cycleMs | Frames @ 6ms | Frames @ 8ms | Frames @ 16ms |
|-----------|--------:|-------------:|-------------:|--------------:|
| C_BLUE_FADE | 6 | 1 | 1 | 1 |
| C_AMBER_PULSE | 48 | 8 | 6 | 3 |
| C_RAINBOW | 48 | 8 | 6 | 3 |
| C_PASTEL | 48 | 8 | 6 | 3 |
| C_RED_FADE | 48 | 8 | 6 | 3 |
| C_ORANGE_FADE | 48 | 8 | 6 | 3 |
| C_REDGREEN | 240 | 40 | 30 | 15 |
| C_ORANGEPURPLE | 240 | 40 | 30 | 15 |
| C_BLUEGREEN | 240 | 40 | 30 | 15 |
| C_REDPURPLE | 240 | 40 | 30 | 15 |

Note: The frame count varies by refresh rate, but animation timing stays synchronized. For example, C_REDGREEN always takes 240ms per state change (40 frames × 6ms = 30 frames × 8ms = 15 frames × 16ms).

### Adding Custom Dynamic Color Animations

When adding new dynamic color animations, choose a `cycleMs` value that is a multiple of 48 to ensure clean frame divisibility across all refresh rates:

**Recommended cycleMs values**: 6, 12, 24, 48, 96, 144, 240, 288, 336, 480...

**Example**: To create a custom 300ms animation effect:
- Use `cycleMs = 288` (closest multiple of 48)
- **6ms projects**: cycle = 288 / 6 = 48 frames = 288ms
- **8ms projects**: cycle = 288 / 8 = 36 frames = 288ms
- **16ms projects**: cycle = 288 / 16 = 18 frames = 288ms

Avoid values that don't divide evenly (e.g., cycleMs=30 or cycleMs=50) as they cause integer truncation and inconsistent timing across refresh rates.

Note: Palette animations don't require this normalization—they automatically adapt to the project's refresh rate using the speed multiplier.

## Palette Animation System

The `getPaletteColor()` function provides **refresh-rate-normalized palette animation** with fractional precision across all chains.

### Palette Position Tracking

Palettes use a 16-bit fractional accumulator allowing smooth interpolation between palette colors. Critically, palette animations **do not have independent timing**—they advance based on the project's `DEVICE_REFRESH_MS` with a `speedMultiplier` that adjusts interpolation speed.

**How It Works**:
- Every `DEVICE_REFRESH_MS`, the palette index advances by `increment`
- The `speedMultiplier` controls how much to advance (0.1 = 10% speed, 1.0 = normal, 2.0 = double speed)
- The project's `DEVICE_REFRESH_MS` naturally normalizes palette speed across different devices
- No separate cycle configuration needed—speed is purely controlled by the multiplier

**How Fractional Precision Works**:
- **Lower 8 bits** (0-255): Sub-pixel fractional precision for smooth interpolation between palette colors
- **Upper 8 bits** (0-255): Actual palette position (0-255)
- **Wrapping**: When position exceeds 255, fractional bits wrap and interpolation continues smoothly
- **No stuttering**: Fractional accumulation ensures smooth color transitions even at 16ms refresh rates

**Example Palette Speed Across Projects**:

With `speedMultiplier = 1.0` and 16-color palette:

| DEVICE_REFRESH_MS | Increment | Frames per Position | Actual Time per Position |
|---:|---:|---:|---|
| 6ms | 256 | 1.0 frame | ~6ms (256/256 × 6ms) |
| 8ms | 192 | 1.33 frames | ~10.67ms (256/192 × 8ms) |
| 16ms | 96 | 2.67 frames | ~42.67ms (256/96 × 16ms) |

Note: Each project naturally takes proportionally longer to cycle the palette based on its device refresh rate (multiply by 16 for full palette cycle time). Use `speedMultiplier` to adjust how fast colors interpolate *within* that refresh rate.

### Palette Features

The `getPaletteColor()` function supports:
- **Speed Multiplier** (0.1-10.0x): Controls how fast to interpolate between palette colors. 1.0 = normal speed, 0.5 = half speed, 2.0 = double speed. The actual animation advances based on `DEVICE_REFRESH_MS`—the multiplier just adjusts interpolation velocity.
- **Phase Offset**: Distributes palette position across multiple LEDs (flowing animations from different starting points)
- **Reverse Flag**: Plays palette animation backward
- **Brightness**: Applied to final RGB output
- **HSV Interpolation**: Smoothly transitions between adjacent palette colors
- **Color Order**: Automatically applied based on `setColorOrder()`
