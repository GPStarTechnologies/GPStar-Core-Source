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
  uint16_t frameCount;      // How many frames in current animation
  uint32_t startTime;       // When recording/playback started (for timing)
  uint8_t mode;             // Current mode (IDLE, RECORDING, PLAYBACK)
};

AnimationSession g_anim;
```

When saved to NVS, only the `frameCount` and `buffer` are stored (as AnimationData struct). The `startTime` and `mode` are runtime only.

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
  uint16_t frameCount;      // How many frames were recorded
  uint16_t checksum;        // CRC16 of frames[] for optional validation against NVS
  uint8_t frames[600];      // The recorded frame data (0 = no action, 1-4 = relay ID)
};

const uint16_t ANIM_MAX_FRAMES = 600;        // 1 minute @ 100ms
const uint16_t ANIM_TIME_UNIT_MS = 100;      // Frame duration in milliseconds
const uint8_t ANIM_MAX_STORED = 4;           // One per RF button
const char* ANIMATION_NAMES[4] = {"anim1", "anim2", "anim3", "anim4"};
```

---

---

## NVS Storage

Each animation is stored as an `AnimationData` struct under fixed keys (one per RF button).

**What's stored:**
- `frameCount`: Tells playback how many frames to execute
- `checksum`: CRC16 of the `frames[]` array—optional, for verifying data matches what's in NVS if needed
- `frames[]`: The 600-byte array of relay values

**Storage:**
```
NVS Key: "anim1"  →  AnimationData struct (2 + 2 + 600 = 604 bytes)
NVS Key: "anim2"  →  AnimationData struct (2 + 2 + 600 = 604 bytes)
NVS Key: "anim3"  →  AnimationData struct (2 + 2 + 600 = 604 bytes)
NVS Key: "anim4"  →  AnimationData struct (2 + 2 + 600 = 604 bytes)
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
AnimationData anim = {
  .frameCount = 5,
  .checksum = crc16_compute(anim.frames, 600),  // Computed when saving
  .frames = {1, 0, 2, 0, 3, 0, 0, 0, ...}  // Rest is zero-filled
};

// Write to NVS
nvs_set_blob(handle, "anim1", &anim, sizeof(anim));

// Read from NVS (optional validation)
AnimationData anim;
nvs_get_blob(handle, "anim1", &anim, &size);

// Verify against NVS checksum if desired
uint16_t computed = crc16_compute(anim.frames, 600);
if (computed != anim.checksum) {
  // Checksum mismatch—could skip this animation
}
```

**Memory usage:** 4 animations × 604 bytes = ~2.4KB NVS (16KB available ✅)

---

## RF Button Behavior

RF buttons are **playback control only**:

| Press | Behavior |
|-------|----------|
| Button N (IDLE) | Load & play animation N |
| Button N (same, PLAYBACK) | Stop playback |
| Button M (different, PLAYBACK) | Stop current, play animation M |

---

## Core Functions

**`startRecording()`**
- Clear buffer, set mode = ANIM_RECORDING, capture startTime

**`recordRelayAtCurrentFrame(uint8_t actuatorID)`**
- frame = (millis() - startTime) / 100
- Write actuatorID to buffer[frame]
- Update frameCount

**`stopRecording()`**
- Set mode = ANIM_IDLE, return frameCount

**`saveRecordingToNVS(uint8_t animIndex)`**
- Write to NVS: frameCount + buffer data
- Clear, mode = ANIM_IDLE

**`loadAnimationFromNVS(uint8_t animIndex)`**
- Read from NVS into buffer, parse frameCount

**`startPlayback(uint8_t animIndex)`**
- Load animation, set mode = ANIM_PLAYBACK, capture startTime

**`stopPlayback()`**
- Set mode = ANIM_IDLE

**`updatePlayback()`** (called each AnimationTask cycle)
- If mode != ANIM_PLAYBACK, return
- frame = (millis() - startTime) / 100
- If frame < frameCount: trigger relay if buffer[frame] != 0
- If frame >= frameCount: stop playback

---

## Integration

**UserInputTask (RF Buttons):**
```cpp
static uint8_t currentPlayingAnim = 0xFF;

if (g_anim.mode == ANIM_IDLE || g_anim.mode == ANIM_PLAYBACK) {
  if (buttonIndex == currentPlayingAnim && g_anim.mode == ANIM_PLAYBACK) {
    g_anim.stopPlayback();
    currentPlayingAnim = 0xFF;
  } else {
    if (g_anim.mode == ANIM_PLAYBACK) {
      g_anim.stopPlayback();
    }
    g_anim.startPlayback(buttonIndex);
    currentPlayingAnim = buttonIndex;
  }
}
```

**AnimationTask:**
```cpp
if (g_anim.mode == ANIM_PLAYBACK) {
  g_anim.updatePlayback();
}
```

**Web Endpoints:**
```
POST /api/animations/record/start
POST /api/animations/record/relay?actuatorID=N
POST /api/animations/record/stop
POST /api/animations/record/save?animIndex=0
POST /api/animations/play?animIndex=0
POST /api/animations/stop
```

---

## Implementation

**Phase 1: Structure & Constants**
- Add AnimationMode enum and AnimationSession struct to Header.h
- Add constants: ANIM_MAX_FRAMES=480, ANIM_TIME_UNIT_MS=250, ANIM_MAX_STORED=4
- Create Animation.h, Animation.cpp
- Add global g_anim to main.cpp

**Phase 2: Recording**
- Implement startRecording, recordRelayAtCurrentFrame, stopRecording, saveRecordingToNVS
- Add web endpoints: /record/start, /record/relay, /record/stop, /record/save

**Phase 3: Playback**
- Implement loadAnimationFromNVS, startPlayback, stopPlayback, updatePlayback
- Integrate RF button control in UserInputTask
- Add web endpoints: /play, /stop

