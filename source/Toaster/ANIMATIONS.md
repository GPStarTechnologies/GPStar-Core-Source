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
enum AnimationState : uint8_t {
  ANIM_IDLE_EMPTY = 0,       // No buffer data, no loaded animation
  ANIM_RECORDING = 1,        // Recording in progress
  ANIM_IDLE_PENDING_SAVE = 2,// Recording stopped, data unsaved (user can save/discard)
  ANIM_IDLE_LOADED = 3,      // Animation loaded from NVS, ready to play
  ANIM_PLAYBACK = 4          // Playback in progress
};

// In-memory runtime state (used during recording/playback)
struct AnimationSession {
  uint8_t buffer[600];       // Current animation being recorded or played
  uint16_t keyFrames;        // Count of frames containing non-zero values (trigger events)
  uint16_t totalFrames;      // Timeline span from 0 to last frame (0-ANIM_MAX_FRAMES)
  uint32_t wallTime;         // millis() timestamp when recording/playback began
  uint8_t state;             // Current state (5 explicit states, see AnimationState enum)
  int8_t sourceSlot;         // Source context: -1=fresh recording, 0-3=loaded from slot
};

AnimationSession currentAnimation;
```

When saved to NVS, `keyFrames`, `totalFrames`, and `buffer` are stored (as AnimationData struct). The `wallTime`, `mode`, and `sourceSlot` are runtime-only (recalculated on each session).

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

```cpp
// Persistent animation data (stored in NVS)
struct AnimationData {
  uint16_t keyFrames;       // Count of frames with relay activity (trigger events)
  uint16_t totalFrames;     // Timeline span (0 to last frame when user pressed stop)
  uint16_t checksum;        // CRC16 of frames[] for optional validation against NVS
  uint8_t frames[600];      // The recorded frame data (0 = no action, 1-4 = relay ID)
};

// Runtime state (NOT persisted) - 5 explicit states
enum AnimationState : uint8_t {
  ANIM_IDLE_EMPTY = 0,       // No buffer data, no loaded animation
  ANIM_RECORDING = 1,        // Recording in progress
  ANIM_IDLE_PENDING_SAVE = 2,// Recording stopped, data unsaved
  ANIM_IDLE_LOADED = 3,      // Animation loaded from NVS
  ANIM_PLAYBACK = 4          // Playback in progress
};

struct AnimationSession {
  uint8_t buffer[600];      // Current animation being recorded or played
  uint16_t keyFrames;       // Count of frames containing non-zero values
  uint16_t totalFrames;     // Timeline span from 0 to last frame
  uint32_t wallTime;        // millis() timestamp when recording/playback began
  uint8_t mode;             // Current mode (IDLE, RECORDING, PLAYBACK)
  int8_t sourceSlot;        // Source context: -1=fresh recording, 0-3=loaded from slot
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

**`void startRecording()`**

- Clear buffer, set mode = ANIM_RECORDING, capture wallTime, set keyFrames = 0, totalFrames = 0
- File: [src/Animation.cpp](src/Animation.cpp)

**`void recordRelayAtCurrentFrame(uint8_t actuatorID)`**

- Calculate frame index: `(millis() - wallTime) / ANIM_TIME_UNIT_MS`
- Write actuatorID to `buffer[frame]` (values 1-4 for ACTUATOR_1 through ACTUATOR_4)
- Increment `keyFrames` (count of relay events)
- Update `totalFrames` to track timeline span
- Bound frame to `[0, ANIM_MAX_FRAMES-1]` to prevent buffer overflow

**`void updateRecordingElapsedTime()`** (NEW - called continuously during recording)

- If mode != ANIM_RECORDING, return immediately
- Calculate elapsed time: `(millis() - wallTime) / ANIM_TIME_UNIT_MS`
- Update `totalFrames` to track timeline span continuously (not just when keyFrames recorded)
- Bound to `[0, ANIM_MAX_FRAMES-1]` to prevent overflow
- **Purpose:** Ensure totalFrames always reflects true elapsed time, not locked to last keyFrame position
- **Called from:** AnimationTask every ~10ms during recording, and from `recordRelayAtCurrentFrame()` before each trigger
- File: [include/Animation.h](include/Animation.h)

**`uint16_t stopRecording()`**

- Call `updateRecordingElapsedTime()` for final update
- Set mode = ANIM_IDLE
- Return totalFrames (timeline span frozen at true duration)

**`bool saveRecordingToNVS(uint8_t animIndex)`**

- Validate `keyFrames > 0` (must have at least one relay event to save)
- Validate `animIndex` is in range `[0, ANIM_MAX_STORED-1]`
- Compute checksum of buffer
- Create AnimationData struct with keyFrames, totalFrames, checksum, and frames
- Write to NVS under key `ANIMATION_NAMES[animIndex]`
- Return true if successful

**`bool loadAnimationFromNVS(uint8_t animIndex)`**

- Validate `animIndex` is in range
- Read AnimationData from NVS
- Validate `keyFrames > 0 && totalFrames > 0 && totalFrames <= ANIM_MAX_FRAMES`
- Optionally validate checksum (warn if mismatch)
- Copy frames, keyFrames, and totalFrames into runtime buffer
- Clear mode (leave as IDLE)
- Return true if successful

**`bool startPlayback(uint8_t animIndex)`**

- Call loadAnimationFromNVS(animIndex)
- If load fails, return false
- Set mode = ANIM_PLAYBACK, capture wallTime = millis()
- Return true

**`void stopPlayback()`**

- Set mode = ANIM_IDLE
- Clear buffer, keyFrames, totalFrames, wallTime

**`void updatePlayback()`** (called each AnimationTask cycle ~10ms)

- If mode != ANIM_PLAYBACK, return immediately
- Calculate current frame: `(millis() - wallTime) / ANIM_TIME_UNIT_MS`
- If `currentFrame >= totalFrames`: playback complete → stopPlayback()
- Else if `buffer[currentFrame] != 0`: trigger the relay via `triggerActuator()`
- File: [src/Animation.cpp](src/Animation.cpp)
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

Calls two critical functions:
1. `updatePlayback()` - Advances playback frame counter and triggers relays at proper times
2. `sendAnimationFrameData()` - **Controlling point** for all animation state updates:
   - Calls `updateRecordingElapsedTime()` internally for recording timeline tracking
   - Sends SSE events during RECORDING, PLAYBACK, or IDLE_PENDING_SAVE states
   - Ensures frontend receives consistent snapshots of animation state every ~10ms

```cpp
void AnimationTask(void *parameter) {
  while(true) {
    // ... relay cleanup logic ...

    updatePlayback(); // Update animation playback if currently playing
    // Send animation data during active recording/playback, or on transition to unsaved state
    if(currentAnimation.state == ANIM_RECORDING || currentAnimation.state == ANIM_PLAYBACK || currentAnimation.state == ANIM_IDLE_PENDING_SAVE) {
      sendAnimationFrameData(); // Send real-time frame data to connected clients via SSE
    }

    updateAudio();
    checkMusic();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
```

**Key Points:**
- `sendAnimationFrameData()` is the **controlling point** for all animation state updates
- It calls `updateRecordingElapsedTime()` internally before building frame data
- This ensures timeline tracking happens atomically with data transmission
- Atomic updates prevent frontend from seeing stale or inconsistent state

**UserInputTask (main.cpp, RF Button Handler):**

RF buttons are **playback control only**. The 5-state model simplifies button logic:
- If NOT in PLAYBACK state: start animation
- If IN PLAYBACK state: same button = stop, different button = switch

```cpp
// Track which animation is currently playing
static int8_t currentPlayingAnim = -1;  // -1 = no animation playing

// When RF button triggers (after debounce detection):
if (stateChanged && buttons[i]->state.currentState && !buttons[i]->state.previousState) {
  uint8_t buttonIndex = i;  // 0-3 maps to anim 0-3

  // Handle animation playback control based on current state
  // RF buttons only trigger playback when not in active playback mode
  if (currentAnimation.state != ANIM_PLAYBACK) {
    // Not playing - start this animation
    if (startPlayback(buttonIndex)) {
      currentPlayingAnim = buttonIndex;
      notifyWSClients();
    }
  } else if (currentAnimation.state == ANIM_PLAYBACK) {
    // Currently playing - handle same or different button
    if (buttonIndex == currentPlayingAnim) {
      // Same button pressed - stop playback
      stopPlayback();
      currentPlayingAnim = -1;
      notifyWSClients();
    } else {
      // Different button - stop current, play new
      stopPlayback();
      if (startPlayback(buttonIndex)) {
        currentPlayingAnim = buttonIndex;
        notifyWSClients();
      }
    }
  }
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

## Real-Time Data Stream (SSE Events)

## Server-Sent Events (SSE) Animation Stream

**Purpose:** Send animation frame updates to frontend in real-time during recording and playback. This is the **single source of truth** for frontend state synchronization.

**Function: `String getAnimationFrame()`** (Webhandler.h)

Serializes current animation state as JSON object with these fields:

```json
{
  "mode": "RECORDING|PLAYBACK|IDLE",
  "sourceSlot": -1 to 3,
  "totalFrames": 600,
  "currentFrame": 0-600,
  "keyFrames": 0-600,
  "elapsedSeconds": 0.0-60.0,
  "totalTime": 0.0-60.0,
  "progress": 0.0-100.0,
  "frameValue": 0-4,
  "lastActuator": 0-4
}
```

**Field Descriptions:**

- `mode`: Current state (string for UI display)
- `sourceSlot`: Context indicator (-1=fresh recording, 0-3=loaded slot, -1 again on idle)
- `totalFrames`: Timeline capacity (always 600 for now)
- `currentFrame`: Frame position based on elapsed time since wallTime
- `keyFrames`: Count of non-zero frames (keyFrames) - used to validate save was captured
- `elapsedSeconds`: Human-readable elapsed time (currentFrame × 0.1s), rounded to 2 decimals
- `totalTime`: Total duration of animation (totalFrames × 0.1s), rounded to 2 decimals
- `progress`: Percentage completion (currentFrame / totalFrames × 100), rounded to 2 decimals
- `frameValue`: Actuator ID at current frame (0=idle, 1-4=relay triggering now)
- `lastActuator`: Most recent actuator triggered (backward search from totalFrames-1), always shows recent activity

**Function: `void sendAnimationFrameData()`** (Webhandler.h)

- **Controlling point for all animation state updates**
- Calls `updateRecordingElapsedTime()` first (before building frame data)
- Builds current animation state via `getAnimationFrame()`
- Sends SSE "animation" event if:
  - Mode is RECORDING or PLAYBACK (active modes), OR
  - Mode is IDLE **AND** keyFrames > 0 (transition event with recorded data)
- **Purpose:** Atomically update and transmit animation state every AnimationTask cycle (~10ms)
- Ensures frontend sees consistent snapshot of state (timestamps sync'd to single moment)

**SSE Event Flow:**

1. **During RECORDING:**
   - `sendAnimationFrameData()` called every ~10ms from AnimationTask
   - Sends frame position, elapsed time, lastActuator to frontend
   - Frontend updates progress display in real-time
   - **When user presses Stop:**
     - `stopRecording()` called, mode set to IDLE
     - Next `sendAnimationFrameData()` detects IDLE with keyFrames > 0
     - Sends **final event** with recorded state
     - Frontend receives it, transitions UI to show save selector

2. **During PLAYBACK:**
   - `sendAnimationFrameData()` called every ~10ms from AnimationTask
   - Sends current frame, elapsed time, triggered actuator
   - Frontend updates progress display in real-time
   - **When animation finishes:**
     - `updatePlayback()` detects currentFrame >= totalFrames
     - Calls `stopPlayback()`, mode set to IDLE
     - Next `sendAnimationFrameData()` sees IDLE with keyFrames == 0 (after stopPlayback cleared it)
     - No event sent (IDLE with no keyFrames = not a "recordable" state)

3. **Idle State:**
   - SSE events stop being sent (mode = IDLE with keyFrames = 0)
   - Frontend continues displaying last known state
   - Ready for next recording or playback start

## Animation Slot Cache

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

## Frontend UI (index.html & index.js)

**Simplified Direct Slot Selection:**

Removed redundant trigger buttons (`btnSaveButton`, `btnPlayButton`). Slot selectors now show/hide directly based on animation state:

- **Save Selector:** Hidden by default. Shows directly when recording stops with captured frames (SSE event with `mode=IDLE, keyFrames>0`)
- **Play Selector:** Hidden by default. Shows when user clicks would-be "Play" button (future feature)

**Recording UI Flow:**

1. User clicks "Start" → progress displays show, selectors hide
2. User triggers actuators → progress updates in real-time via SSE
3. User clicks "Stop" → progress hides, **save selector appears directly**
4. User selects slot and clicks "Save" → animation persisted to NVS
5. UI refreshes slot cache, Play dropdown updates to show saved animation

**Frontend Functions (index.js):**

- `recordingStart()` - Hide selectors, show progress during active recording
- `recordingStop()` - Hide progress, wait for SSE final event
- `updateAnimationDisplay(animData)` - Main state handler:
  - If transitioning RECORDING→IDLE with keyFrames>0: `showEl("saveSlotSelector")`
  - If transitioning RECORDING→IDLE with no keyFrames: `hideEl("saveSlotSelector")`
  - During active modes: update progress HTML from animData fields
- `cancelSaveSlot()` - Hide save selector
- `saveToSlot()` - POST to /animations/record/save/{slot}, hide selector on complete

**Key UI Simplification:** No intermediate button clicks needed. Recording→Stop triggers direct save UI appearance via SSE event. One-click path to save.

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
