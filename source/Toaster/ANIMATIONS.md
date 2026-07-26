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

```cpp
enum AnimationMode : uint8_t {
  ANIM_IDLE = 0,
  ANIM_RECORDING = 1,
  ANIM_PLAYBACK = 2
};

// In-memory runtime state (used during recording/playback)
struct AnimationSession {
  uint8_t buffer[600];      // Current animation being recorded or played
  uint16_t keyFrames;      // How many frames in current animation
  uint32_t startTime;       // When recording/playback started (for timing)
  uint8_t mode;             // Current mode (IDLE, RECORDING, PLAYBACK)
  int8_t sourceSlot;        // Source context: -1=fresh recording, 0-3=loaded from slot, 0xFF=idle
};

AnimationSession currentAnimation;
```

When saved to NVS, only the `keyFrames` and `buffer` are stored (as AnimationData struct). The `startTime`, `mode`, and `sourceSlot` are runtime only.

**sourceSlot Context Tracking:**

The `sourceSlot` field tracks where an animation came from for UI display purposes:

| Value | State                           | UI Display                    |
| ----- | ------------------------------- | ----------------------------- |
| -1    | Fresh recording (not yet saved) | "Recording: Frame X / Y"      |
| 0-3   | Loaded from NVS slot            | "Playing Slot N: Frame X / Y" |
| 0xFF  | Idle (no animation)             | Hidden progress display       |

**State Transitions:**

- `startRecording()` → sourceSlot = -1 (fresh recording)
- `saveRecordingToNVS(2)` → sourceSlot = 2 (now persisted to slot 2)
- `startPlayback(0)` → sourceSlot = 0 (loaded from slot 0)
- `stopPlayback()` → sourceSlot = 0xFF (idle)

---

## State Machine

```
┌──────────────────────────┐
│        IDLE              │
│  Awaiting command        │
└─────┬────────────────────┘
      │
      ├──[Record Start]──→ RECORDING ──[Save]──→ NVS
      │
      └──[Playback Start]──→ PLAYBACK
           (from NVS)

┌──────────────────────────┐
│     RECORDING            │
│  User triggers relays    │
│  via Web UI buttons      │
└──────────────────────────┘

┌──────────────────────────┐
│     PLAYBACK             │
│  Execute frames every    │
│  100ms until complete    │
└──────────────────────────┘
```

---

## Constants (Header.h)

```cpp
// Persistent animation data (stored in NVS)
struct AnimationData {
  uint16_t keyFrames;      // How many frames were recorded
  uint16_t checksum;        // CRC16 of frames[] for optional validation against NVS
  uint8_t frames[600];      // The recorded frame data (0 = no action, 1-4 = relay ID)
};

// Runtime state (NOT persisted)
enum AnimationMode : uint8_t {
  ANIM_IDLE = 0,
  ANIM_RECORDING = 1,
  ANIM_PLAYBACK = 2
};

struct AnimationSession {
  uint8_t buffer[600];      // Current animation being recorded or played
  uint16_t keyFrames;      // How many frames in current animation
  uint32_t startTime;       // When recording/playback started (for timing)
  uint8_t mode;             // Current mode (IDLE, RECORDING, PLAYBACK)
  int8_t sourceSlot;        // Source context: -1=fresh recording, 0-3=loaded from slot, 0xFF=idle
};


const uint16_t ANIM_MAX_FRAMES = 600;        // 1 minute @ 100ms
const uint16_t ANIM_TIME_UNIT_MS = 100;      // Frame duration in milliseconds
const uint8_t ANIM_MAX_STORED = 4;           // One per RF button
const char* ANIMATION_NAMES[4] = {"anim1", "anim2", "anim3", "anim4"};

extern AnimationSession currentAnimation;  // Global animation session state
```

---

---

## NVS Storage

Each animation is stored as an `AnimationData` struct under fixed keys derived from `ANIMATION_NAMES[4]`.

**What's stored:**

- `keyFrames`: Tells playback how many frames to execute
- `checksum`: CRC16 of the `frames[]` array—optional, for verifying data matches what's in NVS if needed
- `frames[]`: The 600-byte array of relay values

**Storage Keys & Layout:**

```
NVS Namespace: Default
NVS Keys:      "anim1", "anim2", "anim3", "anim4"
Type:          Blob (binary)
Size:          AnimationData struct (2 + 2 + 600 = 604 bytes per animation)
Total:         4 animations × 604 bytes = ~2.4KB (16KB NVS available ✅)
```

**NVS Access Pattern (from Animation.cpp):**

```cpp
nvs_handle_t handle;
nvs_open("animations", NVS_READWRITE, &handle);

// Write animation to NVS
AnimationData data = {...};
nvs_set_blob(handle, ANIMATION_NAMES[animIndex], &data, sizeof(data));
nvs_commit(handle);

// Read animation from NVS
AnimationData data;
size_t size = sizeof(data);
nvs_get_blob(handle, ANIMATION_NAMES[animIndex], &data, &size);

nvs_close(handle);
```

**Checksum Validation (optional):**

```cpp
// When loading from NVS, optionally verify data integrity
uint16_t computed = computeChecksum(data.frames);
if (computed != data.checksum) {
  // Checksum mismatch—data corrupted or load failed
  // Skip this animation or alert user
}
```

**Example: Storing a 5-frame animation**

What was recorded:

```
Frame 0 (t=0ms):    Trigger Relay 1
Frame 1 (t=100ms):  (nothing)
Frame 2 (t=200ms):  Trigger Relay 2
Frame 3 (t=300ms):  (nothing)
Frame 4 (t=400ms):  Trigger Relay 3
```

Stored as AnimationData:

```cpp
AnimationData animData = {
  .keyFrames = 5,
  .checksum = computeChecksum(animData.frames),  // Computed when saving
  .frames = {1, 0, 2, 0, 3, 0, 0, 0, ...}  // Rest is zero-filled
};
```

---

## RF Button Behavior

RF buttons are **playback control only**:

| Press                          | Behavior                       |
| ------------------------------ | ------------------------------ |
| Button N (IDLE)                | Load & play animation N        |
| Button N (same, PLAYBACK)      | Stop playback                  |
| Button M (different, PLAYBACK) | Stop current, play animation M |

---

## Core Functions (Animation Class)

**`void startRecording()`**

- Clear buffer, set mode = ANIM_RECORDING, capture startTime
- File: [src/Animation.cpp](src/Animation.cpp)

**`void recordRelayAtCurrentFrame(uint8_t actuatorID)`**

- Calculate frame index: `(millis() - startTime) / ANIM_TIME_UNIT_MS`
- Write actuatorID to `buffer[frame]` (values 1-4 for ACTUATOR_1 through ACTUATOR_4)
- Bound frame to `[0, ANIM_MAX_FRAMES-1]` to prevent buffer overflow
- Update keyFrames if this frame is new

**`uint16_t stopRecording()`**

- Set mode = ANIM_IDLE
- Return keyFrames

**`bool saveRecordingToNVS(uint8_t animIndex)`**

- Validate `animIndex` is in range `[0, ANIM_MAX_STORED-1]`
- Compute checksum of buffer
- Create AnimationData struct with keyFrames, checksum, and frames
- Write to NVS under key `ANIMATION_NAMES[animIndex]`
- Return true if successful

**`bool loadAnimationFromNVS(uint8_t animIndex)`**

- Validate `animIndex` is in range
- Read AnimationData from NVS
- Optionally validate checksum (warn if mismatch)
- Copy frames and keyFrames into buffer
- Clear mode (leave as IDLE)
- Return true if successful

**`bool startPlayback(uint8_t animIndex)`**

- Call loadAnimationFromNVS(animIndex)
- If load fails, return false
- Set mode = ANIM_PLAYBACK, capture startTime
- Return true

**`void stopPlayback()`**

- Set mode = ANIM_IDLE
- Clear buffer and keyFrames

**`void updatePlayback()`** (called each AnimationTask cycle ~10ms)

- If mode != ANIM_PLAYBACK, return immediately
- Calculate current frame: `(millis() - startTime) / ANIM_TIME_UNIT_MS`
- If `frame >= keyFrames`: playback complete → stopPlayback()
- Else if `buffer[frame] != 0`: trigger the relay via `triggerActuator()`
  - Note: Only trigger once per frame (use previous frame tracking to avoid repeat triggers)
- File: [src/Animation.cpp](src/Animation.cpp)

---

## Integration

**Global Animation Instance (main.cpp):**

```cpp
// Near top of main.cpp, after includes
AnimationSession currentAnimation = {};  // Initialize to all zeros (IDLE mode)
```

**AnimationTask (main.cpp, line ~122):**

```cpp
void AnimationTask(void *parameter) {
  while(true) {
    // ... existing relay logic ...

    // Update animation playback each cycle
    currentAnimation.updatePlayback();

    updateAudio();
    checkMusic();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
```

**UserInputTask (main.cpp, RF Button Handler):**

```cpp
// Track which animation is currently playing
static uint8_t currentPlayingAnim = 0xFF;  // 0xFF = no animation playing

// When RF button triggers (after debounce detection):
if (stateChanged && buttons[i]->state.currentState && !buttons[i]->state.previousState) {
  uint8_t buttonIndex = i;  // 0-3 maps to anim 0-3

  // Check current animation state
  if (currentAnimation.mode == ANIM_IDLE) {
    // Not playing - start this animation
    if (currentAnimation.startPlayback(buttonIndex)) {
      currentPlayingAnim = buttonIndex;
    }
  } else if (currentAnimation.mode == ANIM_PLAYBACK) {
    // Currently playing
    if (buttonIndex == currentPlayingAnim) {
      // Same button pressed - stop playback
      currentAnimation.stopPlayback();
      currentPlayingAnim = 0xFF;
    } else {
      // Different button - stop current, play new
      currentAnimation.stopPlayback();
      if (currentAnimation.startPlayback(buttonIndex)) {
        currentPlayingAnim = buttonIndex;
      }
    }
  }

  notifyWSClients();
}
```

**Web Endpoints (Webrouting.h & Webhandler.h):**

```
POST   /api/animations/record/start           → Start recording
POST   /api/animations/record/relay?id=N      → Record relay trigger at current frame
POST   /api/animations/record/stop            → Stop recording, return frame count
POST   /api/animations/record/save?index=N    → Save to NVS under slot N (0-3)
POST   /api/animations/play?index=N           → Load & play animation N
POST   /api/animations/stop                   → Stop playback immediately
GET    /api/animations/status                 → Get current mode & animation info
GET    /api/status                            → Device status (includes animationSlots array)
```

---

## Slot Metadata Cache

**Purpose:** Track which NVS animation slots have valid recordings so the UI can display available animations and disable empty slots in the play dropdown.

**Cache Structure (Webhandler.h):**

```cpp
struct AnimationSlot {
  uint8_t id;           // Slot ID (0-3)
  bool hasAnimation;    // Whether this slot has a valid recording
  uint16_t keyFrames;  // Frame count if hasAnimation=true, 0 otherwise
};

AnimationSlot animationSlots[4] = {};  // Global cache array
```

**Cache Initialization:**

- `refreshAnimationSlotCache()` is called on startup in `startWebServer()`
- Scans NVS for each animation slot (0-3)
- Validates AnimationData structure and keyFrames
- Populates cache with slot availability and frame count

**Cache Updates:**

- `refreshAnimationSlotCache()` is called after every successful save via `saveRecordingToNVS()`
- Client notified via WebSocket that slots have changed
- UI dropdowns update to reflect current slot availability

**API Response (GET /api/status):**

The device status endpoint now includes `animationSlots` array:

```json
{
  "equipmentType": "Toaster",
  "systemUptime": 12345,
  "buttons": [...],
  "relays": [...],
  "animationSlots": [
    {
      "id": 0,
      "hasAnimation": true,
      "keyFrames": 120
    },
    {
      "id": 1,
      "hasAnimation": false,
      "keyFrames": 0
    },
    {
      "id": 2,
      "hasAnimation": true,
      "keyFrames": 300
    },
    {
      "id": 3,
      "hasAnimation": false,
      "keyFrames": 0
    }
  ]
}
```

---

## Client-Side Slot Management

**JavaScript Functions (index.js):**

`updateSaveSlots(slots)` - Updates save dropdown (all slots always enabled for overwriting):

- Takes animationSlots array from status API
- Updates each option label to show frame count if slot has data
- All options remain enabled (user can save to any slot)

`updatePlaySlots(slots)` - Updates play dropdown (only enables slots with animations):

- Takes animationSlots array from status API
- Updates option labels with frame count and "(empty)" indicator
- Disables options where `hasAnimation=false`
- Disables Play button if no slots have animations

**UI Integration:**

- Both functions called automatically when status is received via WebSocket
- Dropdowns refresh whenever `updateEquipment()` processes a status update
- Play button remains disabled until at least one animation is saved

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
  - `computeChecksum()`, `validateChecksum()`, `clearBuffer()`
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
