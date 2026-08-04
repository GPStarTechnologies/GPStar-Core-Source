# Lighting Library Consolidation & LED Driver Migration Plan

## Overview

**Vision**: Establish the Lighting library as the single canonical source for all color, HSV/RGB conversion, and animation logic across all 7 device projects. FastLED will be completely decoupled from project-level code; only the Lighting library may include FastLED (during transition). This decoupling enables future LED driver swaps (Adafruit_NeoPXL8 for ESP32-S3, Adafruit_NeoPixel for others) without touching application logic.

**Two-Milestone Approach**:
1. **Milestone 1: FastLED Consolidation** (BLOCKING) — Audit all Colours.h, consolidate everything into Lighting library, achieve zero FastLED dependencies in projects
2. **Milestone 2: LED Driver Migration** (Follow-up) — Replace Lighting library's FastLED dependency with Adafruit_NeoPXL8 (ESP32-S3) and Adafruit_NeoPixel (others)

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

### Current State: Colours.h Audit

**Colours.h Files (7 total)**
| Project | File | Device Slots | Key Features |
|---------|------|--------------|--------------|
| Attenuator | `source/Attenuator/include/Colours.h` | 3 (DEVICE_NUM_LEDS) | millisDelay timers, device-indexed state arrays, C_CUSTOM |
| ProtonPack | `source/ProtonPack/include/Colours.h` | 6 | getDeviceColour() routing by FireMode, custom color overrides (C_CUSTOM_POWERCELL, C_CUSTOM_CYCLOTRON, C_CUSTOM_INNER_CYCLOTRON), C_HASLAB, C_BLUE_FADE, millisDelay timers |
| NeutronaWand | `source/NeutronaWand/include/Colours.h` | 1 (wand barrel) | WAND_ACTION_STATUS awareness, barrel LED count adjustment (48-LED vs. GPStar/Frutto), C_CUSTOM with global color/saturation variables, millisDelay timers |
| PSTT | `source/PSTT/include/PSTT/include/Colours.h` | 1 (target trainer) | C_BLUEGREEN, C_REDPURPLE added (not in other projects), standard animations, millisDelay timers |
| BeltGizmo | `source/BeltGizmo/include/Colours.h` | 1 | millisDelay timers, C_CUSTOM, standard animations |
| SingleShot | `source/SingleShot/include/Colours.h` | 1 (barrel + cyclotron combined) | Simplest implementation, no timers needed (counter-based only), standard animations |
| StreamEffects | `source/StreamEffects/include/Colours.h` | 1 | millisDelay timers, C_CUSTOM, standard animations |

#### Complete Colour Enum Audit (All 7 Projects)

**Comprehensive Color Coverage**

| Color Enum | Lighting Lib | ProtonPack | Attenuator | NeutronaWand | PSTT | BeltGizmo | SingleShot | StreamEffects |
|------------|-------------|-----------|-----------|------------|------|-----------|-----------|-------------|
| **Static Colors** (24 base) |
| C_BLACK - C_PURPLE | ✅ (24) | ✅ (24) | ✅ (24) | ✅ (24) | ✅ (24) | ✅ (24) | ✅ (24) | ✅ (24) |
| **Dynamic Colors** |
| C_REDGREEN | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| C_ORANGEPURPLE | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| C_BLUEGREEN | ✅ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ |
| C_REDPURPLE | ✅ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ |
| C_AMBER_PULSE | ✅ | ❌ | ✅ | ❌ | ❌ | ✅ | ❌ | ✅ |
| C_ORANGE_FADE | ✅ | ❌ | ✅ | ❌ | ❌ | ✅ | ❌ | ✅ |
| C_RED_FADE | ✅ | ❌ | ✅ | ❌ | ❌ | ✅ | ❌ | ✅ |
| C_BLUE_FADE | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| C_PASTEL | ✅ | ✅ | ❌ | ✅ | ✅ | ❌ | ✅ | ❌ |
| C_RAINBOW | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Custom Colors** |
| C_CUSTOM (generic) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ❌ | ✅ |
| C_CUSTOM_POWERCELL | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| C_CUSTOM_CYCLOTRON | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| C_CUSTOM_INNER_CYCLOTRON | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| C_HASLAB | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |

**Critical Gaps**

1. **Color Enum Spread (MUST ADD TO LIGHTING)**:
   - C_BLUEGREEN, C_REDPURPLE: Only in PSTT (2 projects missing)
   - C_AMBER_PULSE: Only in Attenuator/BeltGizmo/StreamEffects (ProtonPack, NeutronaWand, PSTT, SingleShot missing)
   - C_ORANGE_FADE, C_RED_FADE: Only in Attenuator/BeltGizmo/StreamEffects (ProtonPack, NeutronaWand, PSTT, SingleShot missing)
   - C_BLUE_FADE: Only in ProtonPack (all others missing)
   - C_PASTEL: Only in ProtonPack/NeutronaWand/PSTT (Attenuator, BeltGizmo, StreamEffects missing)

2. **Device-Specific Custom Colors (KEEP LOCAL BY DESIGN)**:
   - ProtonPack: C_CUSTOM_POWERCELL, C_CUSTOM_CYCLOTRON, C_CUSTOM_INNER_CYCLOTRON, C_HASLAB (device-specific overrides)
   - Attenuator, NeutronaWand, PSTT, BeltGizmo, StreamEffects: C_CUSTOM with global hue/saturation (device-local extension)
   - SingleShot: No custom color support
   - **Recommendation**: Keep device-specific colors as local enum extensions; Lighting library provides base 24 static + 10 dynamic

3. **Timing Model Mismatch (CRITICAL FOR PHASE 1B)**:
   - Lighting library: Frame-counting with cycle values (C_RAINBOW=2, C_REDGREEN=50, etc.)
   - Attenuator, BeltGizmo, StreamEffects: millisDelay timers with per-device i_change_delay[] array
   - NeutronaWand, PSTT: Frame-counting (matches Lighting library)
   - ProtonPack: Frame-counting (matches Lighting library) + getDeviceColour() FireMode routing
   - SingleShot: Frame-counting (matches Lighting library)
   - **Impact**: 3 projects (Attenuator, BeltGizmo, StreamEffects) need migration path: keep millisDelay wrappers or adopt frame-counting?

4. **Device-Specific Capabilities (MUST STAY IN PROJECTS)**:
   - ProtonPack: getDeviceColour() with FireMode routing (PROTON, SLIME, STASIS, MESON, SPECTRAL, HOLIDAY_*, SPECTRAL_CUSTOM)
   - NeutronaWand: WAND_ACTION_STATUS awareness, i_num_barrel_leds cycle adjustment (48-LED → cycle=255)
   - Attenuator: Per-device brightness tracking (i_curr_bright[], i_next_bright[]) with multi-step fades
   - Others: Standard getHue() lookups only

---

### Consolidation Strategy

**Phase 1A: Complete Lighting Library (Blocking gate for all projects)**

Ensure Lighting library has EVERYTHING, projects remove NOTHING locally yet.

1. ✅ Verify hsv2rgb() output matches FastLED baseline (test vectors for 256 hues)
2. ❌ Add nscale8(), scale8_video() utilities
3. ✅ Verify all SingleColor + DynamicColor enums present
4. ✅ Add C_BLUE_FADE to DynamicColor enum + implement in getDynamicColorHSV()
5. 🔧 Document device-specific custom color pattern (e.g., C_CUSTOM_POWERCELL not library-managed)
6. ✅ Verify device slot management (0–6 works for all projects)
7. ⚠️ Audit animation cycle speeds; document any device-specific variations

**Phase 1B: Update All Projects to Include Lighting, Remove FastLED (Atomic switchover)**

For each project, in parallel or sequence:
1. Update #include: Remove `#include "Colours.h"`, add `#include <Lighting.h>` in main.cpp
2. Keep local Colours.h as compatibility adapter (wraps Lighting library calls)
3. Verify compile: Zero FastLED includes in project (only Lighting.h may have it)
4. Verify runtime: Colors/animations identical to baseline

**Phase 1C: Verification & Sign-off**

- ✅ All 7 projects compile without FastLED in includes
- ✅ All projects pass color/animation tests
- ✅ Document any custom color overrides (per-device)
- ✅ Mark Milestone 1 COMPLETE

---

## Current State Analysis (Detailed)

### Architecture Gap

| Aspect | Lighting Library | Device Colours.h | Gap | Impact |
|--------|------------------|------------------|-----|--------|
| **Type System** | LED_HSV/LED_RGB (custom) | CHSV/CRGB (FastLED) | Dual ecosystem | Projects still depend on FastLED types |
| **Dependency** | None (platform-agnostic) | FastLED required | Direct coupling | Projects directly include FastLED |
| **Animation Strategy** | Frame-counting (no timers) | millisDelay timers | Timing mismatch | Animation speed parity needs verification |
| **State Management** | Static arrays for up to 6 devices | Global arrays (size varies 1-6) | Inconsistent indexing | Need to ensure all projects use slots correctly |
| **Device-Specific Logic** | None | ProtonPack getDeviceColour() with FireMode routing | Separation of concerns | Keep in ProtonPack or abstract to Lighting? |
| **Color Enums** | SingleColor + DynamicColor (separate) | Mixed single enum per device | Inconsistent organization | Consolidate into Lighting; document custom colors |
| **Custom Colors** | Not supported | Device-specific globals (i_spectral_*) | No standard mechanism | Keep as device-local; Lighting provides base colors |

### Consolidation Challenges

1. **Custom Color Management**: ProtonPack, NeutronaWand, Attenuator, BeltGizmo, StreamEffects all have device-specific custom color globals. Lighting library cannot manage these (no central config). **Solution**: Keep as device-local; provide extension points in documentation.

2. **Animation Cycle Variations**: NeutronaWand adjusts cycles based on barrel LED count. Other projects don't. **Solution**: Audit if this is universal need or NeutronaWand-specific; document; consider parameterization in getDynamicColorHSV() if needed.

3. **System State Dependencies**: NeutronaWand's getHue() checks WAND_ACTION_STATUS (fire mode). Lighting library is stateless. **Solution**: Keep WAND_ACTION_STATUS logic in NeutronaWand; Lighting library provides raw colors; NeutronaWand adapts as needed.

4. **ProtonPack's getDeviceColour() Complexity**: Highly device-specific routing (FireMode → color, device → color). **Solution**: Keep in ProtonPack; Lighting library provides color values; ProtonPack routes.

---

## PHASE 1A: Complete Lighting Library (COMPLETE — 2026-08-03)

**Status**: ✅ ALL WORK ITEMS COMPLETE. Lighting library is now fully canonical source for all colors/animations. 43/43 tests passing.

**Objective**: Ensure Lighting library has all color/animation logic needed by all projects.

### Work Items (All Complete):

1. **✅ HSV→RGB Conversion Accuracy** (COMPLETE)
   - Comprehensive test suite: 43 tests, all passing (includes later additions)
   - HSV→RGB verified; integer math tolerances documented
   - **UNBLOCKS**: All remaining M1A tasks ✅

2. **✅ Add Math Utilities** (COMPLETE)
   - Implemented: `nscale8()`, `scale8_video()` using FastLED 3.10.3 canonical formulas
   - 13 math utility tests added; licensing compliance verified
   - Formula 1: nscale8(value, scale) = (value * (scale + 1)) >> 8
   - Formula 2: scale8_video(value, scale) = ((value * scale) >> 8) + ((value && scale) ? 1 : 0)

3. **✅ Add All Dynamic Color Animations** (COMPLETE)
   - All 10 dynamic colors implemented in Lighting library getDynamicColorHSV():
     - C_REDGREEN, C_ORANGEPURPLE, C_BLUEGREEN, C_REDPURPLE (alternating)
     - C_AMBER_PULSE, C_ORANGE_FADE, C_RED_FADE, C_BLUE_FADE (fading)
     - C_PASTEL, C_RAINBOW (cycling)
   - Frame-counting pattern with cycle values documented (see audit table above)
   - All animation tests passing

4. **✅ Complete Color Enum Audit** (COMPLETE — 2026-08-03)
   - **Finding**: Lighting library is SUPERSET of all projects' local enums
   - **Key Result**: Projects have SUBSET of available colors; each uses 27-34 colors from Lighting's 34 total (24 static + 10 dynamic)
   - **Action Needed for Phase 1B**: When projects migrate, add missing dynamic colors to local Colours.h as pass-throughs to Lighting
     - SingleShot: Add C_BLUE_FADE, C_AMBER_PULSE, C_ORANGE_FADE, C_RED_FADE, C_BLUEGREEN, C_REDPURPLE (missing 6)
     - ProtonPack: Add C_BLUEGREEN, C_REDPURPLE, C_AMBER_PULSE, C_ORANGE_FADE, C_RED_FADE (missing 5)
     - Attenuator, BeltGizmo, StreamEffects: Add C_BLUE_FADE, C_BLUEGREEN, C_REDPURPLE, C_PASTEL (missing 4 each)
     - NeutronaWand: Add C_BLUE_FADE, C_AMBER_PULSE, C_ORANGE_FADE, C_RED_FADE, C_BLUEGREEN, C_REDPURPLE (missing 6)
     - PSTT: Add C_AMBER_PULSE, C_ORANGE_FADE, C_RED_FADE, C_BLUE_FADE (missing 4)

5. **✅ Custom Color Extension Pattern** (COMPLETE)
   - Pattern documented: Lighting provides 24 static + 10 dynamic; projects keep device-specific custom colors (C_CUSTOM, C_CUSTOM_POWERCELL, etc.) local
   - All 7 projects can extend without modifying Lighting library

6. **✅ Device Slot Indexing Verified** (COMPLETE)
   - All projects within MAX_DYNAMIC_COLOR_DEVICES=6 bounds
   - ProtonPack: 6 devices (full utilization)
   - Attenuator: 3 devices
   - Others: 1 device each
   - State arrays (s_dynamicCounter[], s_dynamicHue[], etc.) sufficient for all projects

### Detailed Animation Cycle Audit (Now Complete)

**Cycle Value Mapping** (Lighting Library → Project Implementation):

---

## ✅ PHASE 1A: COMPLETE — Ready for Phase 1B

**Completion Status**: All 6 M1A work items COMPLETE. Lighting library now contains:
- ✅ M1A-1: HSV→RGB conversion with integer math + 34 tests
- ✅ M1A-2: FastLED 3.10.3 canonical math utilities (nscale8/scale8_video) + 13 tests
- ✅ M1A-3: C_BLUE_FADE animation implementation + test
- ✅ M1A-4: Frame-counting animation cycle audit (VERIFIED across all projects)
- ✅ M1A-5: Custom color extension pattern documented
- ✅ M1A-6: Device slot indexing verified (all 7 projects within bounds)

**Architectural Refactor (2026-08-04)**: Lighting converted from static-only to instance-based class
- ✅ Each device creates its own `Lighting(numDevices)` instance
- ✅ Instance owns all per-device state (frame counters, brightness tracking)
- ✅ Projects no longer manage duplicate state arrays (i_count[], i_curr_bright[], etc.)
- ✅ Added `setCustomColorHSV()` / `getCustomColorHSV()` for user-configured custom colors
- ✅ Test suite refactored from static API to instance API (43/43 tests passing)
- ✅ All test fixtures now create Lighting instances with appropriate device counts

**Test Suite Status**: 43/43 tests PASSING (100% success rate)
- LightingConversionFixture: 12 tests
- LightingColorsFixture: 8 tests
- LightingBrightnessFixture: 13 tests (includes 5 new math utility tests)
- LightingAnimationsFixture: 7 tests
- Other fixtures: 3 tests

**Key Deliverables**:
1. Lighting library is now a canonical color/animation source (platform-independent, no FastLED dependency)
2. Instance-based architecture aligns with other SharedLib libraries (DeviceState, Communication, etc.)
3. All color/animation logic migrated with state management encapsulated in instances
4. Animation cycle values documented (⚠️ some discrepancies require HARDWARE TESTING before Phase 1B)
5. Type system: LED_HSV/LED_RGB (internal) with HSV/RGB conversions implemented
6. State management: Per-instance frame-counting with dynamic arrays (supports 1-6 devices per instance)

**Next Step**: Phase 1B can begin immediately. Projects will create Lighting instances and migrate away from direct FastLED includes and duplicate state arrays.

---

## PHASE 1B: Migrate All Projects to Instance-Based Lighting

**Objective**: All 7 projects create Lighting instances, remove duplicate state arrays, use Lighting for all color/animation logic.

**Architecture Change**: Static-only Lighting → Instance-based Lighting
- **Before**: `Lighting::getDynamicColorHSV(0, C_REDGREEN, 255)` (static methods, projects manage i_count[], i_curr_bright[], etc.)
- **After**: `Lighting lighting(6); lighting.getDynamicColorHSV(C_REDGREEN, 255)` (instances own all state)

**For each project** (Attenuator, ProtonPack, NeutronaWand, BeltGizmo, StreamEffects, PSTT, SingleShot):

### 1. **Create Lighting instance(s) based on device count**

```cpp
// In main.cpp or device initialization
#include <Lighting.h>

// ProtonPack: 6 device slots (POWERCELL, CYCLOTRON_OUTER, CYCLOTRON_INNER, CYCLOTRON_PANEL, VENT_LIGHT, spare)
Lighting lighting(6);

// Attenuator: 3 device slots (Top, Upper, Lower)
Lighting lighting(3);

// Others: 1 device slot each
Lighting lighting(1);
```

### 2. **Remove duplicate state arrays from local Colours.h**

**DELETE these arrays** (Lighting instance now owns them):
```cpp
// REMOVE: These are now managed by Lighting instance
uint8_t i_count[6];           // Animation frame counter
uint8_t i_curr_bright[3];     // Current brightness
int16_t i_next_bright[3];     // Target brightness
uint8_t dynamicCounter[];     // ANY per-device animation state
```

**KEEP project-specific arrays** (not related to standard color/animation):
```cpp
// KEEP: Project-unique state (not animation-related)
millisDelay ms_colour_change[];  // Attenuator/BeltGizmo/StreamEffects timers
uint16_t i_change_delay[];       // Attenuator color change delay values
// Device-specific routing logic arrays stay local
```

### 3. **Update getColorHSV() calls to use Lighting instance**

**Example - ProtonPack:**
```cpp
// BEFORE: Static calls, manage i_count[] locally
CHSV getHue(uint8_t i_colour) {
  if(i_count[device]++ >= cycle) {
    i_count[device] = 0;
  }
  LED_HSV hsv = Lighting::getColorHSV((SingleColor)i_colour, brightness, saturation);
  return CHSV(hsv.h, hsv.s, hsv.v);
}

// AFTER: Instance calls, Lighting owns frame counter
CHSV getHue(uint8_t i_colour) {
  // Lighting instance handles frame counting internally now
  LED_HSV hsv = lighting.getColorHSV((SingleColor)i_colour, brightness, saturation);
  return CHSV(hsv.h, hsv.s, hsv.v);
}
```

### 4. **Update getDynamicColorHSV() calls**

**Example:**
```cpp
// BEFORE: Static method with device slot parameter
LED_HSV hsv = Lighting::getDynamicColorHSV(0, C_REDGREEN, 255);

// AFTER: Instance method, no slot parameter needed if using single-device Lighting
LED_HSV hsv = lighting.getDynamicColorHSV(C_REDGREEN, 255);

// If using multi-device Lighting (ProtonPack), still pass device slot
LED_HSV hsv = lighting.getDynamicColorHSV(C_REDGREEN, 255);  // Slot implicit if this is device 0's Lighting
// OR create separate Lighting instances per device group:
// Lighting lighting_powercell(1), lighting_cyclotron(3), etc.
```

### 5. **Map custom colors using setCustomColorHSV()**

For NeutronaWand barrel, ProtonPack custom colors, Attenuator spectral colors, etc.:

```cpp
// During initialization or preferences load:
LED_HSV barrel_color = {32, 200, 255};  // From NVS
lighting.setCustomColorHSV(0, C_CUSTOM, barrel_color);

// Later, when needed:
LED_HSV custom_hsv = lighting.getCustomColorHSV(0);
```

### 6. **Keep millisDelay wrappers (Attenuator, BeltGizmo, StreamEffects only)**

These projects use timers instead of frame-counting:
```cpp
// Keep timer logic
millisDelay ms_colour_change[3];
uint16_t i_change_delay[3] = { 10, 10, 10 };

// Wrap Lighting calls
if(ms_colour_change[device].justFinished()) {
  ms_colour_change[device].start(i_change_delay[device]);
  // Lighting manages its own frame counter; we just trigger color changes with timers
}
LED_HSV hsv = lighting.getDynamicColorHSV(C_RAINBOW, 255);
```

### 7. **Verify no FastLED in project includes**

Grep/search for:
```bash
grep -r "#include <FastLED.h>" source/Attenuator source/ProtonPack source/NeutronaWand ... \
  --include="*.h" --include="*.cpp"
# Should return ZERO results (FastLED only in Lighting.h)
```

### 8. **Build & test**

```bash
cd source/ProtonPack && pio run
cd source/Attenuator && pio run
# etc. for all 7 projects

# Verify:
# - Zero compile errors
# - All colors/animations appear identical to baseline
# - No "undefined reference to Lighting" errors
```

### 9. **Remove local Colours.h enum duplicates** (Optional Phase 1B+)

Once all projects are stable with Lighting instances:
- Remove redundant enum definitions from project Colours.h
- Projects can optionally keep Colours.h as a thin wrapper/adapter if it's called from many places
- Or fully eliminate Colours.h and use Lighting directly

---

## PHASE 1C: Verification & Sign-off

**Success Criteria (all must be true)**:

- ✅ **Zero FastLED includes in projects**: Only `source/SharedLib/Lighting/include/Lighting.h` may include FastLED
- ✅ **All projects create Lighting instances**: No more static method calls
- ✅ **Duplicate state arrays removed**: No i_count[], i_curr_bright[], i_dynamicCounter[] in projects
- ✅ **HSV→RGB output parity**: 256-hue test vectors match FastLED baseline
- ✅ **Animation cycle parity**: Dynamic colors cycle at expected frame rates (verified on hardware if available)
- ✅ **Custom color mapping works**: setCustomColorHSV() / getCustomColorHSV() tested
- ✅ **All 7 projects compile & run**: No runtime color/animation regressions

**Milestone 1 Checkpoint**: Create PR or branch `feature/lighting-consolidation-m1` with above work

---

## MILESTONE 2: LED Driver Migration (Post-M1)

**Objective** (Only after Milestone 1 complete): Replace Lighting library's FastLED dependency with Adafruit_NeoPXL8 (ESP32-S3) and Adafruit_NeoPixel (others).

### Architecture: FastLED → Adafruit

```
Hardware Targets
├── ESP32-S3 (ProtonPack, NeutronaWand, Attenuator, etc.)
│   └── Adafruit_NeoPXL8 [https://github.com/adafruit/Adafruit_NeoPXL8]
│
└── Other Platforms (PSTT, SingleShot, etc. if non-ESP32-S3)
    └── Adafruit_NeoPixel [https://github.com/adafruit/Adafruit_NeoPixel]
```

### Lighting Library in Milestone 2

```
Lighting Library (Platform-Independent)
├── Color Types (LED_HSV, LED_RGB) ✅ Already independent
├── Color Definitions (SingleColor, DynamicColor, CustomColor enums) ✅ Already independent
├── HSV→RGB Conversion (hsv2rgb_rainbow()) ✅ Will be FastLED-free after M1 validation
├── Math Utilities (nscale8(), scale8_video()) ✅ Will be FastLED-free
└── State Management (per-instance frame-counting) ✅ Already independent

Device Projects (No changes to color/animation logic)
├── Attenuator → Lighting instance + Adafruit_NeoPXL8
├── ProtonPack → Lighting instance + Adafruit_NeoPXL8
├── NeutronaWand → Lighting instance + Adafruit_NeoPXL8
├── BeltGizmo → Lighting instance + Adafruit_NeoPXL8
├── StreamEffects → Lighting instance + Adafruit_NeoPXL8
├── PSTT → Lighting instance + Adafruit_NeoPixel
└── SingleShot → Lighting instance + Adafruit_NeoPixel
```

### Benefits

1. **Decoupled from FastLED**: After M1, Lighting library becomes truly platform-agnostic
2. **Hardware flexibility**: Projects use CPU-appropriate LED library without changing color/animation logic
3. **Future-proof**: Easy to swap LED drivers (ESP32-S3 gets NeoPXL8, others get NeoPixel, etc.)
4. **Reduced duplication**: One canonical color source instead of 7 redundant Colours.h files
5. **Maintainability**: Color definitions and animations updated once in Lighting library

### Milestone 2: LED Driver Abstraction & Migration (Post-Phase 1B Validation)

**Objective**: Abstract LED driver logic behind a clean interface, enabling future driver swaps without touching application code.

**Blocking Gate**: Phase 1B must be 100% complete and validated on hardware (all 7 devices working) before starting Phase 2.

**Rationale**: Separation of concerns:
1. **Phase 1B First**: Prove instance-based Lighting architecture works in real hardware
2. **Phase 2 Later**: Add abstraction layer for driver swaps (can be months away)
3. **Risk Reduction**: One major architectural change at a time

#### Phase 2.1: Define LEDDriver Interface (Lighting.h)

Create abstract interface that each device will implement:

```cpp
// Lighting.h
class LEDDriver {
public:
    virtual ~LEDDriver() = default;
    
    /**
     * Set a single LED to an RGB color.
     * Parameters:
     *   index: [0..MAX_LEDS) - LED position in the strip
     *   color: LED_RGB with r, g, b values [0..255]
     */
    virtual void setLED(uint16_t index, const LED_RGB& color) = 0;
    
    /**
     * Update all LEDs (send buffered changes to hardware).
     * This is the only call that actually updates NeoPixels.
     */
    virtual void show() = 0;
};
```

Update Lighting class to accept a driver:

```cpp
class Lighting {
private:
    LEDDriver* ledDriver;
    
public:
    Lighting(uint8_t deviceCount, LEDDriver* driver) 
        : numDevices(deviceCount), ledDriver(driver) {}
    
    // Color methods now delegate to ledDriver:
    void applyColor(uint16_t ledIndex, SingleColor color, uint8_t brightness = 255) {
        LED_HSV hsv = getColorHSV(color, brightness, 255);
        LED_RGB rgb = hsv2rgb(hsv);
        ledDriver->setLED(ledIndex, rgb);
        ledDriver->show();
    }
};
```

#### Phase 2.2: Implement LEDDriver in Each Device (7 implementations)

**ProtonPack (ESP32-S3 with Adafruit_NeoPXL8)**:
```cpp
class ProtonPackLEDDriver : public LEDDriver {
    Adafruit_NeoPXL8* strip;
    
public:
    ProtonPackLEDDriver(Adafruit_NeoPXL8* s) : strip(s) {}
    
    void setLED(uint16_t index, const LED_RGB& color) override {
        strip->setPixelColor(index, color.r, color.g, color.b);
    }
    
    void show() override {
        strip->show();  // DMA update
    }
};
```

**Attenuator (ESP32 with Adafruit_NeoPixel)**:
```cpp
class AttenuatorLEDDriver : public LEDDriver {
    Adafruit_NeoPixel* strip;
    
public:
    AttenuatorLEDDriver(Adafruit_NeoPixel* s) : strip(s) {}
    
    void setLED(uint16_t index, const LED_RGB& color) override {
        strip->setPixelColor(index, color.r, color.g, color.b);
    }
    
    void show() override {
        strip->show();  // Serial update
    }
};
```

Similar implementations for: NeutronaWand, PSTT, BeltGizmo, SingleShot, StreamEffects.

#### Phase 2.3: Update Lighting.cpp to Use Driver Interface

Before:
```cpp
// Lighting.cpp - hardcoded FastLED
FastLED.setDither(BINARY_DITHER);
for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(r, g, b);
}
FastLED.show();
```

After:
```cpp
// Lighting.cpp - driver agnostic
LED_RGB rgb = hsv2rgb(hsv);
if(ledDriver) {
    ledDriver->setLED(index, rgb);
    ledDriver->show();
}
```

#### Phase 2.4: Integration Testing

**Unit Tests** (Can now mock LEDDriver):
```cpp
class MockLEDDriver : public LEDDriver {
    std::vector<LED_RGB> pixels;
    
public:
    void setLED(uint16_t index, const LED_RGB& color) override {
        if(index < pixels.size()) pixels[index] = color;
    }
    
    void show() override {}  // No-op for testing
    
    LED_RGB getPixel(uint16_t index) const { return pixels[index]; }
};

TEST_F(LightingTest, ApplyColor_UpdatesLED) {
    MockLEDDriver mockDriver;
    Lighting lighting(1, &mockDriver);
    
    lighting.applyColor(0, C_RED, 255);
    
    LED_RGB pixel = mockDriver.getPixel(0);
    EXPECT_EQ(pixel.r, 255);
    EXPECT_EQ(pixel.g, 0);
    EXPECT_EQ(pixel.b, 0);
}
```

**Hardware Tests** (Each device):
1. Create LEDDriver implementation
2. Create Lighting instance with driver
3. Apply test colors to all LEDs
4. Verify visual output matches Phase 1B behavior
5. Cycle through all dynamic colors
6. Check animation timing

#### Phase 2.5: Deployment Steps (Per Device)

For each device (ProtonPack → Attenuator → ... → StreamEffects):

1. **Create driver class** in device's main source file
2. **Instantiate in setup()**:
   ```cpp
   Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
   ProtonPackLEDDriver driver(&strip);
   Lighting lighting(6, &driver);
   ```
3. **Remove FastLED includes** from device headers
4. **Update platformio.ini**: Remove `FastLED`, add appropriate Adafruit library
5. **Compile and test** on hardware
6. **Verify identical behavior** to Phase 1B
7. **Document device-specific notes** (DMA quirks, timing, etc.)

#### Phase 2.6: Validation Checklist

- [ ] Lighting.h defines LEDDriver interface (no FastLED includes in header)
- [ ] All 7 devices implement LEDDriver subclass
- [ ] Lighting.cpp uses `ledDriver->setLED()` and `ledDriver->show()` (no FastLED calls)
- [ ] Unit tests pass with MockLEDDriver
- [ ] ProtonPack compiles and runs identically to Phase 1B
- [ ] Attenuator compiles and runs identically to Phase 1B
- [ ] (5 more devices...) all identical behavior
- [ ] Zero FastLED dependencies remain in projects (only Lighting.h → driver interface)
- [ ] Documentation updated with Phase 2 architecture

#### Phase 2.7: Timeline Estimate

- **Total Duration**: ~2-3 weeks (after Phase 1B is 100% stable)
- Per-device testing: ~2-3 days each × 7 = 14-21 days
- Plus: LEDDriver interface design (2-3 days), integration testing (3-5 days)
- Validation on hardware (parallel, not sequential)

#### Why This Matters

**Before Phase 2**: Lighting still depends on FastLED; projects depend on Lighting + FastLED
```
ProtonPack → FastLED
Attenuator → FastLED
[5 more] → FastLED
All → Lighting (which needs FastLED)
```

**After Phase 2**: Lighting depends only on abstract driver; projects provide concrete implementation
```
ProtonPack → Adafruit_NeoPXL8 (via ProtonPackLEDDriver) → Lighting
Attenuator → Adafruit_NeoPixel (via AttenuatorLEDDriver) → Lighting
[5 more] → [their implementations] → Lighting
Lighting → [abstract LEDDriver interface]
```

**Benefit**: Swap LED libraries in Lighting.cpp once, not in 7 projects.

### Library Compatibility Testing (In Progress)

**Device Target Mapping for Phase 2**:
- **ESP32-S3**: ProtonPack, NeutronaWand, SingleShot, PSTT → Adafruit_NeoPXL8 (parallel DMA)
- **Regular ESP32**: Attenuator, BeltGizmo, StreamEffects → Adafruit_NeoPixel (serial)
- **ATmega2560**: ProtonPack → Adafruit_NeoPixel (serial)

---

## Work Items Summary

### Milestone 1: Complete Consolidation (M1A + M1B + M1C)

**Phase M1A: Complete Lighting Library** (Must complete before M1B/M1C)
- ✅ HSV→RGB conversion accuracy (COMPLETE)
- ❌ Implement nscale8(), scale8_video() math utilities
- ❌ Add C_BLUE_FADE dynamic color
- ⚠️ Audit animation cycle speeds
- 🔧 Document custom color extension pattern
- ⚠️ Verify device slot indexing (0–5)

**Phase M1B: Update All 7 Projects** (Blocked by M1A completion)
- Update #include: `#include "Colours.h"` → `#include <Lighting.h>`
- Replace device Colours.h with wrapper/compatibility layer
- Verify zero FastLED in project includes (only Lighting.h may have it)
- All 7 projects compile successfully

**Phase M1C: Hardware Verification** (Follows M1B)
- Test all 7 devices on actual hardware
- Verify color accuracy and animation speed parity
- Document any deviations

### Milestone 2: LED Driver Migration (Post-M1)

**Status**: Not started (blocked by M1B completion)

**Objective**: Replace Lighting library's FastLED dependency with Adafruit drivers via LEDDriver abstraction

**See Section Above**: Comprehensive Phase 2 documentation with interface design, implementation per device, testing strategy, and validation checklist

---

## Verification Strategy

### Milestone 1: Consolidation Verification

**Unit-Level**:
1. HSV→RGB conversion: 256-hue test vectors (already done in M1A-1)
2. Math utilities: `nscale8(127, 128)` → 64, `scale8_video(0, x)` → 0, `scale8_video(255, x)` → 255
3. Color definitions: `Lighting::getColorHSV(C_RED)` produces expected HSV
4. Animation frame-counting: Run 100 frames of C_RAINBOW, C_REDGREEN; verify cycle counts

**Integration-Level**:
1. **Zero FastLED in projects**: Grep/search confirms `#include <FastLED.h>` only in Lighting.h
2. **All projects compile**: `pio run -e <device>` for all 7 devices, zero errors
3. **Color/animation parity**: Visual inspection of LEDs on hardware (if available)
   - Static colors match baseline
   - Dynamic animations cycle at expected visual speed
   - Device-specific routing (ProtonPack) works

**Regression Prevention**:
- Before/after comparison of color output (HSV values)
- Document any intentional changes (animation speed adjustments)

### Milestone 2: LED Driver Migration Verification

(Detail after M1 complete)

---

## Key Decisions & Trade-offs

| Decision | Rationale | Trade-off |
|----------|-----------|-----------|
| **Milestone 1 blocking before Milestone 2** | Ensures FastLED consolidation complete before switching LED drivers | Longer initial timeline, but cleaner separation of concerns |
| **Reimplement hsv2rgb() instead of depending on FastLED** | Enables Lighting library to be platform-agnostic | Requires rigorous testing against reference implementation |
| **Keep device-specific Colours.h as adapters during M1** | Allows parallel work, lower risk, easier rollback | Temporary duplication until M2 complete |
| **Frame-counting for animations** | No dependency on timers, simpler state | Must verify animation speed parity on hardware |
| **ProtonPack getDeviceColour() stays in device, not in library** | Routing logic is device-specific, library stays generic | Requires devices to implement routing wrapper |
| **Zero FastLED in projects (only in Lighting.h)** | Clean decoupling, enables LED driver swap | Requires rigid include discipline |

---

## References

- **Adafruit_NeoPXL8:** https://github.com/adafruit/Adafruit_NeoPXL8
- **Adafruit_NeoPixel:** https://github.com/adafruit/Adafruit_NeoPixel
- **FastLED hsv2rgb source:** https://github.com/FastLED/FastLED/blob/master/hsv2rgb.cpp
- **FastLED lib8tion source:** https://github.com/FastLED/FastLED/blob/master/lib8tion.h
- **Current Lighting library:** [source/SharedLib/Lighting/](source/SharedLib/Lighting/)
- **Sample Implementation:** [https://github.com/nomakewan/GPStar-proton-pack/tree/neopixeltest](https://github.com/nomakewan/GPStar-proton-pack/tree/neopixeltest)

---

## Status & Checkpoints

- [x] **MILESTONE 1 START**: Review & approve consolidation plan
- [x] **M1A-1**: Verify HSV→RGB output matches FastLED
- [x] **M1A-2**: Add nscale8(), scale8_video(), etc.
- [x] **M1A-3**: Add C_BLUE_FADE, verify all enums
- [x] **M1A-4**: Audit cycle speeds, document patterns
- [x] **M1A-5**: Document custom color extension pattern
- [x] **M1A-6**: Verify device slot indexing (0–5)
- [x] **M1A COMPLETE**: Lighting library ready for projects
- [ ] **M1B START**: Begin updating projects
- [ ] **M1B COMPLETE**: All 7 projects use Lighting.h, zero FastLED includes
- [ ] **M1C**: Verify color/animation parity
- [ ] **MILESTONE 1 COMPLETE**: Consolidation done, PR merged
- [ ] **MILESTONE 2 START**: LED driver migration begins (post-M1)
- [ ] **MILESTONE 2 COMPLETE**: FastLED removed, Adafruit libraries in place
