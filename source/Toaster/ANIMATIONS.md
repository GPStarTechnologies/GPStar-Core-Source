# Toaster Animation System

## Overview

**Purpose:** Record and playback sequences of relay actuations. A user records a series of relay triggers over time, and the system plays them back on demand via RF button press.

**Core Concept:** An array where each element represents a fixed time slice and holds a relay ID (1-4) or no data (0) indicating no action. During recording, as time advances, each frame captures which relay (if any) should trigger at that moment. During playback, the system iterates through the array, triggering relays at the corresponding frame times.

**Time Granularity:** 100ms per frame (0.1 second). This provides 10 frames per second—sufficient for relay sequencing while allowing 1-minute recordings (600 frames) without excessive memory usage.

**4 animation slots** (one per RF button). Each animation can be up to 600 frames (1 minute @ 100ms).

---

## Animation Frame Structure

### Frame Format (1 byte per frame)

```
Value | Meaning
  0   | No action
  1   | Trigger ACTUATOR_1 (relay GPIO25)
  2   | Trigger ACTUATOR_2 (relay GPIO26)
  3   | Trigger ACTUATOR_3 (relay GPIO27)
  4   | Trigger ACTUATOR_4 (relay GPIO32)
```

### Example (10 frames @ 100ms = 1 second)

```
Frame: [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]
Value: [1]   [0]   [2]   [0]   [3]   [0]   [4]   [0]   [1]   [0]
Time:  0ms   100ms 200ms 300ms 400ms 500ms 600ms 700ms 800ms 900ms
```

---

## Runtime State

**AnimationState Enum (5 Explicit States):**

| Value | Name                  | Meaning                                  |
| ----- | --------------------- | ---------------------------------------- |
| 0     | ANIM_IDLE_EMPTY       | No buffer data, no loaded animation      |
| 1     | ANIM_RECORDING        | Recording in progress                    |
| 2     | ANIM_IDLE_PENDING_SAVE| Recording stopped, data unsaved          |
| 3     | ANIM_IDLE_LOADED      | Animation loaded from NVS, ready to play |
| 4     | ANIM_PLAYBACK         | Playback in progress                     |

**AnimationSession Struct (Runtime State):**

| Field        | Type     | Purpose                                          |
| ------------ | -------- | ------------------------------------------------ |
| buffer[600]  | uint8_t  | Current animation being recorded or played       |
| keyFrames    | uint16_t | Count of frames containing non-zero values       |
| totalFrames  | uint16_t | Timeline span from 0 to last frame               |
| wallTime     | uint32_t | millis() timestamp when recording/playback began |
| state        | uint8_t  | Current state (one of 5 AnimationState values)   |
| sourceSlot   | int8_t   | Context: -1=fresh recording, 0-3=loaded from NVS |
| data         | AnimationData | Persistent data (keyFrames, totalFrames, frames) |
| triggerSource| uint8_t  | Playback trigger: NONE, RF, or WEB              |

**Global Instance:**

```cpp
AnimationSession currentAnimation;  // Declared in main.cpp, extern in Animation.h
```

When saved to NVS, `keyFrames`, `totalFrames`, and `buffer` are stored (as AnimationData struct). The `wallTime`, `state`, `sourceSlot`, and `triggerSource` are runtime-only (recalculated on each session).

**keyFrames vs totalFrames:**

- **keyFrames**: Count of relay trigger events recorded (non-zero frame count). Used to validate save (must be > 0).
- **totalFrames**: Timeline span from frame 0 to the last frame when recording stopped. Used for progress bar and playback duration.

Example: If user records for 5 seconds (50 frames) and triggers relays 3 times → `keyFrames=3, totalFrames=50`

**sourceSlot Context Tracking:**

The `sourceSlot` field tracks where an animation came from for UI display purposes:

| Value | State                           | UI Display                    |
| ----- | ------------------------------- | ----------------------------- |
| -1    | Fresh recording (not yet saved) | "Recording: Frame X / totalFrames"  |
| 0-3   | Loaded from NVS slot            | "Playing Slot N: Frame X / totalFrames" |

**State Transitions:**

```
IDLE_EMPTY ──startRecording()──→ RECORDING
RECORDING ──stopRecording()──→ IDLE_PENDING_SAVE
IDLE_PENDING_SAVE ──saveRecordingToNVS()──→ IDLE_LOADED
IDLE_PENDING_SAVE ──discardRecording()──→ IDLE_EMPTY
IDLE_LOADED ──startPlayback()──→ PLAYBACK
IDLE_LOADED ──startRecording()──→ RECORDING (overwrites buffer)
PLAYBACK ──stopPlayback()/auto-complete──→ IDLE_LOADED (preserves data)
```

- `startRecording()` → state = ANIM_RECORDING, sourceSlot = -1, keyFrames = 0, totalFrames = 0
- `recordRelayAtCurrentFrame()` → keyFrames++, totalFrames updated as timeline extends
- `stopRecording()` → state = ANIM_IDLE_PENDING_SAVE, totalFrames frozen at current span
- `saveRecordingToNVS(slot)` → if keyFrames > 0 → state = ANIM_IDLE_LOADED, sourceSlot = slot
- `discardRecording()` → state = ANIM_IDLE_EMPTY, buffer cleared
- `startPlayback(slot)` → load from NVS, state = ANIM_PLAYBACK, sourceSlot = slot, wallTime = millis()
- `stopPlayback()` → state = ANIM_IDLE_LOADED (preserves loaded animation data)

---

## State Machine

```
                    ┌────────────────────────┐
                    │   IDLE_EMPTY           │
                    │ (No data anywhere)     │
                    └───────┬────────────────┘
                            │ startRecording()
                            ↓
                    ┌────────────────────────┐
                    │    RECORDING           │
                    │(User triggers relays)  │
                    └───────┬────────────────┘
                            │ stopRecording()
                            ↓
                    ┌────────────────────────┐
                    │ IDLE_PENDING_SAVE      │
                    │(Unsaved data in buffer)│
                    └──────┬──────────────────┘
                    ┌──────┴──────────────┐
    saveRecording() │                     │ discardRecording()
                    ↓                     ↓
        ┌────────────────────────┐  IDLE_EMPTY
        │   IDLE_LOADED          │
        │(Animation ready/saved) │
        └────────┬───────────────┘
                 │ startPlayback()
                 ↓
        ┌────────────────────────┐
        │     PLAYBACK           │
        │(Execute frames @100ms) │
        └────────┬───────────────┘
                 │ stopPlayback() OR auto-complete
                 ↓
        IDLE_LOADED (data preserved)
```

---

## Constants (Header.h)

**AnimationData Struct (Persistent in NVS):**

| Field     | Type     | Purpose                                    |
| --------- | -------- | ------------------------------------------ |
| keyFrames | uint16_t | Count of frames with relay activity        |
| totalFrames | uint16_t | Timeline span (0 to last frame when stopped) |
| checksum  | uint16_t | CRC16 of frames[] for validation           |
| frames[600] | uint8_t | The recorded frame data (0=none, 1-4=relay ID) |

**Configuration Constants:**

| Constant            | Value | Purpose                              |
| ------------------- | ----- | ------------------------------------ |
| ANIM_MAX_FRAMES     | 600   | Maximum frames per animation (1 min @ 100ms) |
| ANIM_TIME_UNIT_MS   | 100   | Frame duration in milliseconds       |
| ANIM_MAX_STORED     | 4     | Number of animation slots (one per RF button) |
| ANIMATION_NAMES[4]  | "anim1"-"anim4" | NVS storage keys for each slot |

**External Global:**

```cpp
extern AnimationSession currentAnimation;  // Runtime state, defined in main.cpp
```

---

## NVS Storage

Each animation is stored as an `AnimationData` struct under fixed keys derived from `ANIMATION_NAMES[4]`.

**What's stored:**

- `keyFrames`: Count of relay trigger events (non-zero frame count)
- `totalFrames`: Timeline span from recording start to stop
- `checksum`: CRC16 of the `frames[]` array—optional, for verifying data matches what's in NVS if needed
- `frames[]`: The 600-byte array of relay values

**Storage Keys & Layout:**

```
NVS Namespace: animations
NVS Keys:      "anim1", "anim2", "anim3", "anim4"
Type:          Blob (binary)
Size:          AnimationData struct (2 + 2 + 2 + 600 = 606 bytes per animation)
Total:         4 animations × 606 bytes = ~2.4KB (16KB NVS available ✅)
```

**Access Patterns:**

- **Write:** Use NVS C API `nvs_set_blob()` with namespace "animations", key `ANIMATION_NAMES[slot]`, and AnimationData struct
- **Read:** Use NVS C API `nvs_get_blob()` with same namespace and key
- **Validation:** Compute checksum of `frames[]` and compare to stored value to detect NVS corruption

---

## RF Button Behavior

RF buttons are **playback control only**. With 5-state model:

| Current State                   | Button Press           | Behavior                           |
| ------------------------------- | ---------------------- | ---------------------------------- |
| IDLE_EMPTY, IDLE_PENDING_SAVE   | Any button N           | Load animation N from NVS, play    |
| IDLE_LOADED                     | Any button N           | Load animation N from NVS, play    |
| PLAYBACK (animation N)          | Button N (same)        | Stop playback (stay IDLE_LOADED)   |
| PLAYBACK (animation N)          | Button M (different)   | Stop N, start M                    |
| RECORDING                       | Any button             | Ignored (can't playback during recording) |

---

## Core Functions (Animation Class)

**Recording Functions:**

| Function | Purpose |
| -------- | ------- |
| `void startRecording()` | Initialize buffer and state for new recording session |
| `void recordRelayAtCurrentFrame(uint8_t actuatorID)` | Record relay trigger at current frame position |
| `void updateRecordingElapsedTime()` | Update totalFrames to reflect elapsed time (called continuously during recording) |
| `uint16_t stopRecording()` | Freeze timeline and return final totalFrames |

**Save/Load Functions:**

| Function | Purpose |
| -------- | ------- |
| `bool saveRecordingToNVS(uint8_t slot)` | Persist current animation to NVS (validates keyFrames > 0) |
| `bool loadAnimationFromNVS(uint8_t slot)` | Load animation from NVS into buffer (validates keyFrames and totalFrames) |

**Playback Functions:**

| Function | Purpose |
| -------- | ------- |
| `bool startPlayback(uint8_t slot)` | Load animation from NVS and begin playback |
| `void updatePlayback()` | Advance playback frame counter and trigger relays at proper times (called every ~10ms) |
| `void stopPlayback()` | Stop playback and clear state |

**Helper Functions:**

| Function | Purpose |
| -------- | ------- |
| `uint16_t computeChecksum()` | Calculate CRC16 of frames[] array |
| `void clearAnimationBuffer()` | Zero-fill buffer and reset counters |

---

## Integration

**Global Animation Instance:**

Declared in main.cpp, extern in Animation.h:
```cpp
AnimationSession currentAnimation = {};  // Initialized to all zeros (IDLE_EMPTY state)
```

**AnimationTask (Core Loop):**

Runs every ~10ms on Core 1. Calls:
1. `updatePlayback()` - Advances playback frame and triggers relays at proper times
2. `sendAnimationFrameData()` - **Controlling point** for state updates
   - Calls `updateRecordingElapsedTime()` internally before building frame data
   - Ensures timeline tracking is atomic with data transmission

**UserInputTask (RF Button Handler):**

RF buttons are playback control only. State-driven logic:
- State != PLAYBACK: Button press starts animation
- State == PLAYBACK (same button): Stop playback
- State == PLAYBACK (different button): Stop current, start new

Track `currentPlayingAnim` (0-3) to differentiate button behavior.

**Web API Endpoints:**

All animation operations exposed via REST:

| Method | Endpoint | Purpose |
| ------ | -------- | ------- |
| POST | `/animations/record/start` | Begin recording |
| POST | `/animations/record/relay?id=N` | Record relay trigger at current frame |
| POST | `/animations/record/stop` | Freeze timeline, return frame count |
| POST | `/animations/record/save/{slot}` | Persist to NVS (validate keyFrames > 0) |
| POST | `/animations/play/{slot}` | Load and start playback |
| POST | `/animations/stop` | Stop playback immediately |
| GET | `/status` | Device status (includes animationSlots array) |

---

## Real-Time Data Stream (SSE Events)

## Server-Sent Events (SSE) Animation Stream

**Purpose:** Send animation frame updates to frontend in real-time during recording and playback via SSE "animation" event.

**JSON Fields (sent every ~10ms during RECORDING, PLAYBACK, or IDLE_PENDING_SAVE):**

| Field | Type | Purpose |
| ----- | ---- | ------- |
| `state` | string | Animation state name (IDLE_EMPTY, RECORDING, IDLE_PENDING_SAVE, IDLE_LOADED, PLAYBACK) |
| `sourceSlot` | int | Animation origin (-1=fresh recording, 0-3=loaded from NVS) |
| `totalFrames` | uint16 | Timeline span (0-600) |
| `currentFrame` | uint16 | Current frame position |
| `keyFrames` | uint16 | Count of relay trigger events |
| `elapsedSeconds` | float | Human-readable elapsed time (seconds, 2 decimals) |
| `totalTime` | float | Total animation duration (seconds, 2 decimals) |
| `progress` | float | Percentage completion (0-100, 2 decimals) |
| `frameValue` | uint8 | Relay ID at current frame (0=none, 1-4=relay triggering now) |
| `lastActuator` | uint8 | Most recent relay triggered (0=none, 1-4=relay ID) |
| `triggerSource` | string | Playback source (NONE, RF, WEB) |

**Transmission Rules:**

- Send SSE "animation" event every ~10ms when state is RECORDING or PLAYBACK
- Send final SSE "animation" event on transition to IDLE_PENDING_SAVE (recording stopped with captured frames)
- Stop sending when state transitions to IDLE_EMPTY or IDLE_LOADED (no active operation)

**Device State SSE Event:**

Additionally, a "device" SSE event sends RF button and relay states on every state change, independent of animation state. This provides real-time hardware monitoring without animation-specific context.

## Animation Slot Cache

**Purpose:** Track which NVS animation slots have valid recordings so the UI can display available animations and disable empty slots in the play dropdown.

**Cache Structure:**

| Field | Type | Purpose |
| ----- | ---- | ------- |
| id | uint8 | Slot ID (0-3) |
| hasAnimation | bool | Whether this slot has a valid recording |
| keyFrames | uint16 | Frame count if hasAnimation=true, 0 otherwise |

Global cache array: `animationSlot animationSlots[4]`

**Lifecycle:**

1. **Initialization:** `refreshAnimationSlotCache()` called on startup in `startWebServer()`, scans all 4 NVS slots
2. **On Save:** Cache refreshed after successful `saveRecordingToNVS()`, then WebSocket notifies clients
3. **UI Updates:** Dropdowns automatically refresh when status endpoint returns animationSlots array

**Status API Response:**

GET `/status` includes `animationSlots` array with id, hasAnimation, and keyFrames for each slot.

---

## Frontend UI (index.html & index.js)

**Animation State Machine (5 States):**

The frontend maintains state parity with backend via:
1. WebSocket bulk status (includes animation state)
2. SSE "animation" event (real-time frame updates during RECORDING/PLAYBACK)
3. SSE "device" event (RF button and relay state changes)

**UI State Mapping:**

Each animation state (IDLE_EMPTY, RECORDING, IDLE_PENDING_SAVE, IDLE_LOADED, PLAYBACK) controls which buttons/selectors are enabled:

| State | Start/Stop Visible | Save Selector | Play Selector | Progress Display |
| ----- | ------------------ | ------------- | ------------- | ---------------- |
| IDLE_EMPTY | Start only | Hidden | Show (if slots viable) | Hidden |
| RECORDING | Stop only | Hidden | Hidden | Show |
| IDLE_PENDING_SAVE | Hidden | Show | Hidden | Hidden |
| IDLE_LOADED | Start only | Hidden | Show (if slots viable) | Hidden |
| PLAYBACK | Stop only | Hidden | Hidden | Show |

**Frontend Functions (index.js):**

| Function | Purpose |
| -------- | ------- |
| `updateAnimationDisplay(animData)` | Main state handler - applies button states and updates progress from SSE data |
| `applyButtonStates(state)` | Atomically enable/disable all animation UI controls for given state |
| `buildAnimationProgressHTML(animData)` | Build progress display with slot, percentage, elapsed time, last actuator |
| `updateSaveSlots()` | Refresh save dropdown labels to show animation duration |
| `updatePlaySlots()` | Refresh play dropdown, disable empty slots, set hasAnyViableSlots flag |

**Integration with Backend:**

- SSE "animation" event handler passes data to `updateAnimationDisplay()`
- SSE "device" event handler logs RF button/relay state changes to console
- WebSocket status includes animation state on initial page load

---

## Implementation Phases

**Phase 1: Structure & Constants** ✅ DONE

- ✅ Add AnimationMode enum, AnimationData & AnimationSession structs to [include/Header.h](include/Header.h)
- ✅ Add animation constants to [include/Header.h](include/Header.h)
- ✅ Declare global `currentAnimation` in [include/Header.h](include/Header.h)
- ✅ Create Animation class interface in [include/Animation.h](include/Animation.h)
- Define all public methods with documentation

**Phase 2: Core Implementation (Recording & Playback)**

- Implement [src/Animation.cpp](src/Animation.cpp):
  - Constructor: Initialize buffer to zeros, mode = IDLE
  - `startRecording()`, `recordRelayAtCurrentFrame()`, `stopRecording()`
  - `saveRecordingToNVS()`, `loadAnimationFromNVS()`
  - `startPlayback()`, `stopPlayback()`, `updatePlayback()`
  - `computeChecksum()`, `validateChecksum()`, `clearAnimationBuffer()`
  - Helper functions for CRC16 computation and NVS access

**Phase 3: Task Integration**

- Update [src/main.cpp](src/main.cpp):
  - Instantiate global `currentAnimation` object
  - Add `currentAnimation.updatePlayback()` call to AnimationTask loop
  - Add RF button playback logic to UserInputTask

**Phase 4: Web API**

- Add handlers to [include/Webhandler.h](include/Webhandler.h):
  - `handleRecordStart()`, `handleRecordRelay()`, `handleRecordStop()`, `handleRecordSave()`
  - `handlePlayAnimation()`, `handleStopAnimation()`, `handleAnimationStatus()`
- Register routes in [include/Webrouting.h](include/Webrouting.h):
  - Map endpoints to handlers with documentation tags
  - Use `addSimpleRoute()` for PUT/POST endpoints

**Phase 5: Testing & Validation**

- Unit tests for recording/playback timing
- Integration tests with NVS persistence
- Manual RF button trigger verification
- Web API endpoint testing
