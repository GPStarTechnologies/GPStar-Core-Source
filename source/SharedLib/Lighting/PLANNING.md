# Lighting Library Consolidation & LED Driver Migration Plan

## Overview

**Vision**: Establish the Lighting library as the single canonical source for all color, HSV/RGB conversion, and animation logic across all 7 device projects. FastLED will be completely decoupled from project-level code; only the Lighting library may include FastLED (during transition). This decoupling enables future LED driver swaps (Adafruit_NeoPXL8 for ESP32-S3, Adafruit_NeoPixel for others) without touching application logic.

**Two-Milestone Approach**:
1. **Milestone 1: FastLED Consolidation** (BLOCKING) — Audit all Colours.h, consolidate everything into Lighting library, achieve zero FastLED dependencies in projects
2. **Milestone 2: LED Driver Migration** (Follow-up) — Replace Lighting library's FastLED dependency with Adafruit_NeoPXL8 (ESP32-S3) and Adafruit_NeoPixel (others)

---

## STATUS

**Milestone 1A: Lighting Library** ✅ COMPLETE

**Milestone 1B: Project Migrations** ✅ COMPLETE — 7 of 7 complete
- ✅ StreamEffects (template reference)
- ✅ Attenuator
- ✅ BeltGizmo
- ✅ PSTT
- ✅ SingleShot
- ✅ NeutronaWand
- ✅ ProtonPack (Phase 5, dual-chain + expansion support)

---

## MILESTONE 1: FastLED Consolidation

**Objective**: Audit all Colours.h implementations, identify gaps, consolidate into Lighting library so that **zero projects directly include FastLED** (only Lighting library may).

**Success Criteria**:
- ✅ All 7 projects compile without FastLED in their includes
- ✅ All color definitions from all Colours.h are in Lighting library (or device-specific overrides are documented)
- ✅ All animations (static + dynamic) work identically to baseline
- ✅ All device slot counts (1–6) supported by Lighting library state management
- ✅ Device-specific routing logic (e.g., ProtonPack's getDeviceColour) works with Lighting library
- ✅ HSV→RGB conversion produces identical output to FastLED baseline

---

## ✅ StreamEffects (COMPLETE)

**Template Reference for Remaining Projects**

**Files Changed**:
- Created: `include/LightConfig.h`
- Modified: `src/main.cpp`, `include/System.h`, `include/Header.h`
- Deleted: `include/Colours.h`

**LightConfig.h Pattern**:
```cpp
#pragma once
#define FASTLED_INTERNAL
#include <FastLED.h>        // Driver library only included here
#include <Lighting.h>       // Color abstraction layer

// Hardware config
#define DEVICE_LED_PIN 4
#define DEVICE_MAX_LEDS 500
uint16_t i_num_leds = 250;

// Palette variables (project-specific)
CRGBPalette16 paletteProton, paletteSlime, ...;

// Device enum (project-specific counts: 1-6)
enum device : uint8_t { PRIMARY_LED = 0 };

// Color order enum (same for all projects)
enum LED_COLOR_TYPES : uint8_t { LED_RGB = 1, LED_GRB = 2, LED_GBR = 3 };
LED_COLOR_TYPES LED_COLOR_TYPE = LED_RGB;

// LocalLightingManager singleton (copy template from StreamEffects)
class LocalLightingManager { ... };
LocalLightingManager* LocalLightingManager::instance = nullptr;

// Custom colors (if needed: extern declarations)
extern uint8_t i_spectral_custom_colour;
```

**Files Modified**:
- ✅ Created: `/source/StreamEffects/include/LightConfig.h` (NEW)
- ✅ Modified: `/source/StreamEffects/src/main.cpp` (added includes, updated LED calls)
- ✅ Modified: `/source/StreamEffects/include/System.h` (palette, animation routines)
- ✅ Deleted: `/source/StreamEffects/include/Colours.h` (no longer needed)
- ✅ Modified: `/source/StreamEffects/include/Header.h` (removed LED variables)

---

## Work Specification: Remaining 6 Projects

**Apply to Each Project** (ProtonPack, NeutronaWand, PSTT, BeltGizmo, SingleShot, Attenuator):

### 1. Create `include/LightConfig.h`
Copy StreamEffects template and adjust per project:
- `DEVICE_LED_PIN`: GPIO pin (from Header.h)
- `DEVICE_MAX_LEDS`: Max LED count
- Palette variables: Move from System.h
- `device enum`: Update device count (1-6)
- Constructor: `Lighting(N)` where N = device count

### 2. Update `src/main.cpp`
- Add `#include "LightConfig.h"` before other local includes
- In `setup()`: Call `LocalLightingManager::getInstance().initializeDriver();`
- Replace `FastLED.addLeds()` with manager call
- Replace all `FastLED.show()` calls with `LocalLightingManager::getInstance().show()`
- Replace LED color assignments with manager methods

### 3. Update animation file (System.h or equivalent)
- Move palette definitions to LightConfig.h
- Replace `device_leds` array references with `LocalLightingManager::getInstance().getLEDs()`
- Replace `fill_solid(device_leds, ...)` with `LocalLightingManager::getInstance().lightsOff()`
- Remove the `ledsOff()` function and instead call manager directly where needed

### 4. Update `include/Header.h`
Remove LED variables (all moved to LightConfig.h):
- `DEVICE_LED_PIN`, `DEVICE_MAX_LEDS`, `i_num_leds`
- All palette definitions
- `device enum` (if only LED-related)
- `LED_COLOR_TYPES enum`
- Keep: device state, configuration, non-LED variables

### 5. Delete `include/Colours.h`
Remove file entirely. Verify no includes remain.

### 6. Compile and verify
- `pio run` should compile
- Zero FastLED includes in project code (only in LightConfig.h)

---

## Notes

**Type Deduction**: Use `auto` keyword for Lighting library types (LED_HSV, LED_RGB)

**Device Count per Project**:
- StreamEffects: 1 device
- ProtonPack: 6 devices
- NeutronaWand: 1 device
- PSTT: 1 device
- BeltGizmo: 1 device
- SingleShot: 1 device
- Attenuator: 3 devices

### LocalLightingManager Consistency Pattern

**KEY PRINCIPLE**: Keep the LocalLightingManager class **as identical as possible across all projects**.

- **Same method names**: `getInstance()`, `initializeDriver()`, `show()`, `lightsOff()`, `getLEDs()`, `getColorRGB()`, `getColorGRB()`, `getColorGBR()`, `setBrightness()`
- **Same variable names**: `deviceLEDs`, `instance`, `lightingLib`
- **Same class structure**: Private constructor with Lighting library initialization, singleton pattern

**Values that vary by project**:
- `DEVICE_LED_PIN`: Hardware pin (project-specific)
- `DEVICE_MAX_LEDS`: LED count (project-specific)
- `DEVICE_MAX_BRIGHTNESS`: Brightness level (128 for power conservation, 255 for full brightness)
- Lighting library constructor argument: Device count (1-6 depending on project)

**Benefit**: This standardization ensures all projects follow identical LED abstraction patterns, making refactoring, maintenance, and future driver swaps trivial. If LED driver needs to change, modify only the manager class methods—all projects automatically inherit the update.
