# Lighting Animation System

## Overview

The Lighting library provides dynamic color animations through the `getDynamicColorHSV()` function. Animations are frame-based, driven by how frequently the LED driver's `show()` method is called. This document tracks the update method and frequency for each GPStar device.

## Device Update Methods

| Device | Method | Interval | Frame Rate | Location | Notes |
|--------|--------|----------|-----------|----------|-------|
| **Attenuator** | FreeRTOS Task (AnimationTask) | vTaskDelay(8ms) | ~125 Hz | `src/main.cpp:169` | Direct LightingManager.show() in animation loop |
| **BeltGizmo** | FreeRTOS Task (AnimationTask) | vTaskDelay(16ms) | ~62.5 Hz | `src/main.cpp:128` | Direct LightingManager.show() in animation loop |
| **NeutronaWand** | millisDelay Timer (updateLEDs) | LED_DRIVER_UPDATE_MS (5ms) | ~200 Hz | `src/main.cpp:379` | Uses LightingManager.show() in millisDelay timer block |
| **ProtonPack** | millisDelay Timer (updateLEDs) | LED_DRIVER_UPDATE_MS (5ms) | ~200 Hz | `src/main.cpp:365` | Uses LightingManager.show() in millisDelay timer block |
| **PSTT** | FreeRTOS Task (AnimationTask) | vTaskDelay(16ms) | ~62.5 Hz | `src/main.cpp:113` | Direct LightingManager.show() in animation loop |
| **SingleShot** | TaskScheduler (animateTask) | 16ms | ~62.5 Hz | `src/main.cpp:152` | Uses direct LightingManager.show() in animation callback |
| **StreamEffects** | FreeRTOS Task (AnimationTask) | vTaskDelay(8ms) | ~125 Hz | `src/main.cpp:154` | Direct LightingManager.show() in animation loop |

## Animation Timing & Cycle Values

The `getDynamicColorHSV()` function uses **frame-based timing** with configurable cycle values:

```cpp
actual_time_between_changes_ms = cycle_value × LED_update_interval_ms
```

### Standardized Update Intervals (August 2026)

- **5ms group** (millisDelay): NeutronaWand, ProtonPack → 200 Hz
- **8ms group** (FreeRTOS): Attenuator, StreamEffects → 125 Hz
- **16ms group** (FreeRTOS/TaskScheduler): BeltGizmo, PSTT, SingleShot → 62.5 Hz

### Current Cycle Values (from Lighting.cpp)

| Animation | Cycle | Speed | Time @ 5ms | Time @ 8ms | Time @ 16ms |
|-----------|-------|-------|-----------|-----------|------------|
| **C_BLUE_FADE** | 1 | Fastest (every frame) | 5ms | 8ms | 16ms |
| **C_AMBER_PULSE** | 6 | Fast | 30ms | 48ms | 96ms |
| **C_RAINBOW** | 6 | Fast | 30ms | 48ms | 96ms |
| **C_PASTEL** | 6 | Fast | 30ms | 48ms | 96ms |
| **C_ORANGEPURPLE** | 7 | Medium | 35ms | 56ms | 112ms |
| **C_REDPURPLE** | 7 | Medium | 35ms | 56ms | 112ms |
| **C_RED_FADE** | 8 | Medium | 40ms | 64ms | 128ms |
| **C_ORANGE_FADE** | 10 | Slower | 50ms | 80ms | 160ms |
| **C_REDGREEN** | 50 | Slow | 250ms | 400ms | 800ms |
| **C_BLUEGREEN** | 50 | Slow | 250ms | 400ms | 800ms |

**DEFAULT (cycle=6):** Fast transitions
- At 5ms:  30ms | At 8ms:  48ms | At 16ms: 96ms

### Cycle Selection Guide

| Cycle Value | Speed | Best For | Examples |
|-------------|-------|----------|----------|
| 1 | Every frame (fastest) | Smooth continuous fades | C_BLUE_FADE |
| 6 | Fast (30-96ms) | Streaming effects, rainbow cycles, pulsing | C_RAINBOW, C_PASTEL, C_AMBER_PULSE |
| 7-8 | Medium (35-128ms) | Color alternations, moderate fades | C_ORANGEPURPLE, C_RED_FADE |
| 10-20 | Slower (50-320ms) | Gentle breathing, status transitions | C_ORANGE_FADE |
| 50+ | Very slow (250ms+) | Calm alternations, low-frequency effects | C_REDGREEN, C_BLUEGREEN |

## Timing Issues & Inconsistencies

### Problem Statement (PARTIALLY RESOLVED)

The `getDynamicColorHSV()` function was designed assuming a **consistent frame rate** across all devices. After standardizing intervals, we now have:

**Standardized Intervals (August 2026):**
- **5ms group** (millisDelay-based): NeutronaWand, PSTT, ProtonPack → 200 Hz
- **8ms group** (FreeRTOS-based): Attenuator, StreamEffects → 125 Hz  
- **16ms group** (slower): BeltGizmo, SingleShot → 62.5 Hz

This is a significant improvement from the previous 3-16ms spread. Remaining variance is by design:
- **5ms devices** have more complex LED setups or need higher update rates
- **8ms devices** use FreeRTOS tasks for better concurrency
- **16ms devices** prioritize power efficiency (less frequent interrupt disruption)

### Example: C_REDGREEN Animation

| Device | Interval | Cycle 50 | Result | Change from Previous |
|--------|----------|----------|--------|---------------------|
| NeutronaWand | 5ms | 250ms | Flashes every 250ms | ✅ 150ms → 250ms (standardized) |
| PSTT | 5ms | 250ms | Flashes every 250ms | ✅ 150ms → 250ms (standardized) |
| ProtonPack | 5ms | 250ms | Flashes every 250ms | No change |
| Attenuator | 8ms | 400ms | Flashes every 400ms | No change |
| StreamEffects | 8ms | 400ms | Flashes every 400ms | No change |
| BeltGizmo | 16ms | 800ms | Flashes every 800ms | No change |
| SingleShot | 16ms | 800ms | Flashes every 800ms | ✅ 1000ms → 800ms (optimized) |

**Animation consistency improved:** All animations now follow predictable timing across three intervals (5ms, 8ms, 16ms) instead of the previous chaotic 3-20ms spread.

## Solution Approaches

### Option 1: Normalize Cycle Values Per Device

Adjust cycle values based on device update interval to maintain consistent animation speeds:

```cpp
// In each device's LightConfig.h
#define ANIMATION_CYCLE_SCALE (5.0 / LED_DRIVER_UPDATE_MS)  // Normalize to 5ms reference
```

### Option 2: Runtime Interval Detection

Modify `getDynamicColorHSV()` to accept the actual update interval as a parameter and auto-adjust cycle counting.

### Option 3: Standardize on Single Update Interval

Convert all devices to use the same method (either all millisDelay or all FreeRTOS tasks) with a common interval (e.g., 10ms).

### Option 4: Make Cycles Millisecond-Based

Replace frame-based cycles with actual time values (milliseconds), independent of update interval.

## Migration Status

- ✅ **isColorDynamic check** applied to all LightConfig files
- ✅ **Animation timing standardization** - Complete (August 2026)
  - NeutronaWand: 3ms → 5ms
  - PSTT: 3ms → 5ms
  - SingleShot: 20ms → 16ms
  - ProtonPack, Attenuator, BeltGizmo, StreamEffects: No change needed
- ⏳ **Update method unification** - Planned (consider converting all to FreeRTOS tasks or millisDelay)
