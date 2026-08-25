# Lighting Library

The Lighting library is a **platform-independent, LED-driver-agnostic color management system** for GPStar controllers. It provides unified color definitions, HSV-to-RGB conversion, animation state management, and palette-based effects that work across all LED chains regardless of their underlying LED driver implementation.

## Terminology Glossary

Understanding the hierarchy of GPStar hardware is essential for using this library:

| Term | Definition | Example |
|------|------------|---------|
| **Project** | The overall application/prop being created | Proton Pack, Neutrona Wand, Ghost Trap |
| **Controller** | The PCB with embedded software (ATMega, ESP32) that runs the project | Proton Pack controller PCB running firmware |
| **Device** | A specific LED-equipped attachment connected to the controller | Barrel LED assembly, Inner Cyclotron Panel, "Cake" lights |
| **Chain** | A series of addressable LEDs controlled by a single hardware GPIO pin | 3 LEDs on a GPIO pin, 16 LEDs on another pin |
| **Segment** | (Pack-specific) When a single LED chain spans multiple physical devices for visual continuity | Cyclotron inner panel LEDs flowing into cyclotron cake lights |

**Relationship**: Controller → Devices (aka. Chains) → Individual LEDs (or Segments)

Each controller can manage multiple chains (one per GPIO pin), and multiple LEDs can be part of the same device. In the Proton Pack specifically, segments allow one chain to visually span multiple dedicated uses.

## Why the Lighting Library Exists

### The Challenge

GPStar controllers require LEDs with sophisticated color and animation capabilities:
- Multiple devices (Barrel LED assembly, Cyclotron panel, etc.) attached to each controller with different LED counts
- Multiple LED chains per controller, each on its own GPIO pin
- Need for consistent colors and animations across all chains
- Support for dynamic color effects with per-chain animation state
- Custom user-configurable colors stored in persistent storage (NVS/Preferences)
- Different LED drivers: Adafruit NeoPixel, FastLED, custom implementations
- Chains with varying update intervals (6ms, 8ms, 16ms) requiring timing normalization

### The Solution

The Lighting library **abstracts away hardware details** by:
1. Defining all colors in a controller-independent HSV format
2. Managing animation state separately for each LED chain
3. Providing color conversion (HSV→RGB) independent of the LED driver
4. Supporting dynamic/stateful animations with frame-based timing
5. Handling palette animations with fractional precision normalization
6. Encapsulating common LED operations in reusable functions
7. Provide ease of switching color ordering (eg. RGB vs. GRB vs. GBR)

This allows each controller/chain to swap its LED driver implementation without changing application code.

## Architecture: Three-Layer Design

### Layer 1: Lighting Library (SharedLib)

**File**: `source/SharedLib/Lighting/`

The core color and animation management library:
- **Color Definitions**: 24 static colors (C_RED, C_BLUE, etc.) + 10 dynamic/animated colors (C_RAINBOW, C_REDGREEN, etc.)
- **State Management**: Per-chain animation counters, hue/brightness values, palette positions
- **Color Conversion**: HSV→RGB conversion, color order (RGB/GRB/GBR/etc.) reordering, brightness scaling
- **Animation System**: Five animation modes (ALTERNATE, FADE, PULSE, CYCLE_HUE, DECAY_HUE) with configurable timing
- **Palette Support**: 16-color palettes with smooth interpolation and refresh-rate normalization

**Key Classes**:
- `Lighting`: Core library instance (one per controller, manages multiple LED chains)
- `LED_HSV`, `LED_RGB`: Platform-independent color types
- `AnimationConfig`, `AnimationMode`: Animation configuration and mode enumeration
- `LED_Palette16`: 16-color palette structure

**No Hardware Dependencies**: This library has zero knowledge of:
- Which LED driver is being used
- How LEDs are physically connected
- What the update interval is (handled by LightingManager)

The result is a library written purely in C code which can be unit-tested to ensure basic functionality always works as intended.

### Layer 2: LightingManager (Project Specific)

**File**: `<project>/include/LightConfig.h`

Each **project** implements a `LightingManager` class that serves as the **driver-agnostic interface** between the application and the LED driver. The manager is responsible for handling chains of addressable LEDs on a project's GPIO pins. Since each project has different hardware configurations (different LED counts, different chains, different drivers), each LightingManager implementation is unique to that project.

**Multi-Segment Architecture**: A controller has multiple **segments** (logical device groupings), each with its own LightingManager instance. However, multiple segments can share the same **chain** (GPIO pin). For example, ProtonPack has 6 segments (Powercell, CyclotronLid, NFilter, InnerPanel, InnerCake, InnerCavity) but only 2 GPIO chains—three segments share PACK_LED_PIN and three share CYCLOTRON_LED_PIN.

Each LightingManager instance:

1. **Manages a logical segment** (e.g., Powercell, Cyclotron Inner Panel):
   - Each segment has a unique LED_SEGMENT enum value
   - Segments map to chains via a registry (`lightingDevices` array)
   - Multiple segments can share the same physical chain (GPIO pin)

2. **Shares the Lighting library**:
   - Static `Lighting lightingLib` instance (shared across all segments on the controller)
   - Initialized with segment count and refresh rate
   - Maintains per-segment animation state for all dynamic colors

3. **Abstracts the LED driver**:
   - **ESP32**: Uses single `Adafruit_NeoPXL8` driver managing all GPIO pins with a unified buffer and per-chain offsets
   - **ATMega**: Uses separate `Adafruit_NeoPixel` instances per chain
   - Handles driver-specific details; application code never directly touches the driver

4. **Provides segment-specific interface with input guarding**:
   - `setPixelColor(index, colorId, brightness)`: Set individual LED color by segment-relative index, with bounds checking (index < segmentMaxLEDs)
   - `fillPalette(palette, speedMultiplier)`: Fill entire segment with palette animation
   - `show()`: Update physical LEDs for this segment's chain
   - `lightsOff()`: Blank all LEDs for this segment
   - `setCustomColorHSV()`: Store user color in Lighting library per segment
   - `setColorOrder()`: Set color channel order for this segment

5. **Manages LED mapping and validation**:
   - Maps segment IDs to chain IDs and buffer offsets via registry
   - Guards against out-of-bounds LED indices before passing to driver
   - Validates color IDs and brightness values
   - Supports LED inversion based on user configuration

**Example** (ProtonPack structure):

See the actual project implementation in `<project>/include/LightConfig.h` for:
- `LED_SEGMENT` enum: Logical device groupings (DEVICE_POWERCELL, DEVICE_INNER_PANEL, etc.)
- `LED_CHAIN` enum: GPIO pin definitions (CHAIN_PACK, CHAIN_CYCLOTRON, etc.)
- `LightingDevice` registry: Maps each segment to its chain, buffer offset, and max LED count
- `LightingManager` class: Multi-instance pattern with `getInstance(segmentID)` factory method

### Layer 3: Controller Application Code

**Files**: `<device>/src/main.cpp`

The controller firmware:

1. **Initialization**:
   - Defines `DEVICE_REFRESH_MS` and other hardware constants in `LightConfig.h`
   - Creates driver-specific object instances for each chain as necessary
   - Initializes the shared Lighting library with overall refresh rate

2. **Animation Loop** (repeating every DEVICE_REFRESH_MS):
   - Gets colors for all segments via `getDynamicColorHSV()` or `getPaletteColor()`
   - Updates LED pixels via `setPixelColor()` for each segment
   - Calls `show()` on each segment to physically update LEDs

3. **User Interaction**:
   - Sets custom colors via `setCustomColorHSV()`
   - Changes color order based on user preferences
   - No direct LED driver interaction—all communication flows through LightingManager

## How Device Timing Works

### Refresh Rate Definition

The **entire project** specifies a single device refresh rate in `include/LightConfig.h`:

```cpp
#define DEVICE_REFRESH_MS 6  // or 8, or 16
```

This constant controls:
- How often the device/animation loop runs (6ms, 8ms, or 16ms intervals)
- When `show()` is called to update physical LEDs on all segments
- When animation frame counters advance in the Lighting library
- The timing reference for all dynamic color animations across all segments

This value is passed to the Lighting library constructor so it can properly manage animation state advancement.

### Three Standardized Device Refresh Rates

| Rate | Projects | Reasoning |
|------|---------|----------|
| **6ms** | NeutronaWand, ProtonPack | High-frequency effects, complex animations |
| **8ms** | Attenuator, StreamEffects | FreeRTOS-based tasks, good balance |
| **16ms** | BeltGizmo, PSTT, SingleShot | Status LEDs, power efficiency |

Each project has **one DEVICE_REFRESH_MS value** that applies to all LED segments. The animation loop runs at this interval, calling `show()` on all LightingManager instances and advancing animation frame counters in the Lighting library.

### Frame-Based Animation Timing

All animations use frame-based cycles, not real-time delays. Every `DEVICE_REFRESH_MS`:

1. **Gets colors** for all segments via `getDynamicColorHSV()` (for dynamic colors) or `getPaletteColor()` (for palette animations)
2. **Updates pixels** via `setPixelColor()` for each segment
3. **Calls show()** to physically update LEDs on each segment's GPIO pin

**Dynamic colors** advance their state based on their `cycleMs` configuration (ANIM_ALTERNATE flips every N frames, ANIM_FADE ramps every N frames, etc.).

**Palette colors** interpolate smoothly at the project refresh rate, adjusted by `speedMultiplier`.

This means:
- Animations are predictable and frame-based—no absolute timers needed
- Animation timing depends only on frame count and `DEVICE_REFRESH_MS`
- Consistent behavior across all segments on the same project

## Color Management

### Color Hierarchy

| Type | Range | Characteristics | Use Case |
|------|-------|-----------------|----------|
| **Static Colors** | 0-99 | Fixed HSV, no animation state | Status indicators, fixed colors |
| **Dynamic Colors** | >= 100 | Animated patterns, per-chain state | Streaming effects, visual feedback |
| **Custom Color** | 254 | User-configured HSV per chain | User-defined preferred color |

### Static Colors

Fixed HSV values retrieved via `getColorHSV(colorId, brightness, saturation)`. All 24 static colors are defined once in `Lighting.cpp` with their base hue, saturation, and brightness values.

Brightness and saturation can be overridden per call for dynamic control.

### Dynamic/Animated Colors

Stateful animations managed by the Lighting library via `getDynamicColorHSV(segmentID, colorId, brightness)`:

- Each call advances the animation state for that segment's frame counter
- State is preserved between calls, enabling smooth animation
- 10 animation presets available, each with 5 animation modes
- See [ANIMATIONS.md](ANIMATIONS.md) for detailed animation mode descriptions and configurations

### Custom Colors

User-configured colors stored per segment via `setCustomColorHSV(hsv, segmentID)` and retrieved via `getCustomColorHSV(segmentID)`.

Custom colors are stored in persistent storage (NVS/Preferences) and persist across reboots.

## Color Order Management

Different LED drivers expect different channel orders (RGB vs GRB vs GBR, etc.). The Lighting library handles this transparently via:

- `setColorOrder(segmentID, order)`: Configure channel order for a segment
- `applyColorOrder(rgb, order)`: Convert RGB to the driver's expected channel order

**Supported Orders**: RGB, GRB, GBR, RBG, BRG, BGR

## Integration Pattern

### For Adding a New Feature to Lighting

1. **Add static color** → Update `getStaticColorDefinition()` switch statement
2. **Add dynamic color** → Create `AnimationConfig` entry in `ANIMATION_CONFIGS` array
3. **Add animation mode** → Implement `animate<ModeName>()` method
4. **Add palette** → Create function in `LightingPalettes.h` returning `LED_Palette16`

## Files

- **[LightingBasics.h](include/LightingBasics.h)**: Core type definitions (LED_RGB, LED_HSV, ColorID, AnimationMode, etc.)
- **[Lighting.h](include/Lighting.h)**: Main Lighting class interface
- **[Lighting.cpp](src/Lighting.cpp)**: Lighting implementation and animation logic
- **[LightingPalettes.h](include/LightingPalettes.h)**: Palette definitions for all stream types
- **[ANIMATIONS.md](ANIMATIONS.md)**: Detailed documentation of animation types, modes, and timing
- **[README.md](README.md)**: This file
