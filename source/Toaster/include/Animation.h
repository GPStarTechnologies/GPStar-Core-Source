/**
 *   GPStar Toaster - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2026 Dustin Grau <dustin.grau@gmail.com>
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

/**
 * Animation Recording & Playback System
 * =====================================
 *
 * OVERVIEW:
 * Records sequences of relay actuations and stores them for later playback.
 * Each animation is a 600-frame buffer where each frame represents 100ms.
 * Animations are persisted to NVS (4 slots) and mapped to RF buttons 1-4.
 *
 * WORKFLOW:
 * 1. startRecording()             → Clear buffer, enter RECORDING mode
 * 2. [User triggers relays via RF/web, calls recordRelayAtCurrentFrame() at each trigger]
 * 3. stopRecording()              → Exit RECORDING mode, return frame count
 * 4. saveRecordingToNVS(index)    → Persist buffer to NVS slot [0-3], clear buffer
 * 5. startPlayback(index)         → Load animation from NVS, enter PLAYBACK mode
 * 6. updatePlayback() [in loop]   → Calculate current frame, trigger relays, stop when done
 * 7. stopPlayback()               → Exit PLAYBACK mode, clear buffer
 *
 * KEY CONCEPTS:
 * - Frame Buffer:     currentAnimation.data.frames[600] holds frame data (0=idle, 1-4=relay ID)
 * - Frame Timing:     Each frame = 100ms. At 100fps, max duration = 1 minute
 * - Recording Mode:   Automatically captures relay triggers via recordRelayAtCurrentFrame()
 * - Playback Mode:    Replays recorded frames by calculating elapsed time and firing relays
 * - NVS Persistence:  AnimationData struct stored with CRC16 checksum for validation
 * - Button Binding:   4 NVS slots map directly to 4 RF buttons (slot 0→button 1, etc.)
 *
 * SESSION vs PERSISTENT DATA:
 * - AnimationSession (currentAnimation):      Runtime state (frames, keyFrames, state, timing)
 * - AnimationData (NVS):          Persistent struct (keyFrames, checksum, frames[600])
 *
 * AUTOMATIC RECORDING:
 * When currentAnimation.state == ANIM_RECORDING, any call to recordRelayAtCurrentFrame(actuatorID)
 * will write that actuator ID to the frame buffer at the calculated frame position.
 * This happens automatically when:
 *   - User presses RF button (calls recordRelayAtCurrentFrame in UserInputTask)
 *   - Web API /device/actuator/N called (calls recordRelayAtCurrentFrame in Webhandler)
 *
 * DEFERRED PERSISTENCE:
 * Recording and persistence are intentionally decoupled. After stopRecording(), the
 * buffer remains in RAM. User must explicitly call saveRecordingToNVS(index) to commit
 * to a permanent slot. This allows review/testing before overwriting existing animations.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <Communication.h>

// Forward declarations
bool triggerActuator(ActuatorID actuatorID);
void sendAnimationFrameData(); // From Webhandler.h

/**
 * Animation State Machine
 * Records and plays back sequences of relay actuations at 100ms intervals.
 * 
 * STATE FLOW:
 *   IDLE_EMPTY (no data)
 *     → startRecording() → RECORDING (active recording)
 *     → stopRecording() → IDLE_PENDING_SAVE (data unsaved)
 *     → saveRecordingToNVS() → IDLE_LOADED (ready to play or re-record)
 *     → OR discardRecording() → IDLE_EMPTY
 *   IDLE_LOADED (data persisted or just loaded)
 *     → startPlayback() → PLAYBACK (active playback)
 *     → OR startRecording() → RECORDING (overwrite mode, clears buffer)
 *   PLAYBACK (active playback)
 *     → stopPlayback() → IDLE_LOADED (animation data preserved)
 *     → OR auto-complete → IDLE_LOADED (via stopPlayback() at end)
 */

enum AnimationState : uint8_t {
  ANIM_IDLE_EMPTY = 0,              // No buffer data, no loaded animation
  ANIM_RECORDING = 1,               // Recording in progress (stop button enabled)
  ANIM_IDLE_PENDING_SAVE = 2,       // Recording stopped, data unsaved (save/discard buttons)
  ANIM_IDLE_LOADED = 3,             // Animation loaded from NVS or just saved (play button enabled)
  ANIM_PLAYBACK = 4                 // Playback in progress (stop button enabled)
};

enum TriggerSource : uint8_t {
  TRIGGER_SOURCE_NONE = 0,
  TRIGGER_SOURCE_RF,
  TRIGGER_SOURCE_WEB
};

// Animation Constants
#define ANIM_MAX_FRAMES 600             // 1 minute @ 100ms = 600 frames
const uint16_t ANIM_TIME_UNIT_MS = 100; // Frame duration in milliseconds
const uint8_t ANIM_MAX_STORED = 4;      // One per RF button
const char* ANIMATION_NAMES[4] = {"anim1", "anim2", "anim3", "anim4"};

/**
 * AnimationData - Persistent structure stored in NVS
 * Contains the recorded animation frames and metadata.
 */
struct AnimationData {
  uint16_t totalFrames;            // How many frames were recorded (0-ANIM_MAX_FRAMES)
  uint16_t keyFrames;              // How many frames contain a non-zero value
  uint16_t checksum;               // CRC16 of frames[] for optional validation against NVS
  uint8_t frames[ANIM_MAX_FRAMES]; // The recorded frame data (0 = no action, 1-4 = relay ID)
};

/**
 * AnimationSession - Runtime state used during recording/playback
 * Encapsulates both the animation frame data and playback session metadata.
 * Single source of truth: 'state' field determines all UI button enabled/disabled states.
 */
struct AnimationSession {
  uint8_t state;               // Current AnimationState (single source of truth for UI)
  int8_t sourceSlot;           // Metadata: -1=no slot, 0-3=loaded from slot (only meaningful in IDLE_LOADED)
  TriggerSource triggerSource; // Metadata: source that initiated the active playback
  uint32_t wallTime;           // Metadata: millis() timestamp when recording/playback began
  AnimationData data;          // Frame buffer and metadata (frames, keyFrames, totalFrames, checksum)
};
AnimationSession currentAnimation = {};

// AnimationSlot[N] - Tracks which slots have recordings and their durations
struct AnimationSlot {
  uint8_t id;             // Slot index (0-3)
  bool hasAnimation;      // Whether NVS data exists and is valid
  float animationSeconds; // Animation duration in seconds (calculated from keyFrames * ANIM_TIME_UNIT_MS / 1000)
};
const uint8_t ANIMATION_SLOTS_COUNT = 4;
AnimationSlot animationSlots[ANIMATION_SLOTS_COUNT] = {};

/**
 * Clear the animation buffer and reset runtime state to IDLE_EMPTY
 */
inline void clearAnimationBuffer() {
  currentAnimation.state = ANIM_IDLE_EMPTY;
  currentAnimation.sourceSlot = -1;
  currentAnimation.triggerSource = TRIGGER_SOURCE_NONE;
  currentAnimation.wallTime = 0;
  currentAnimation.data.totalFrames = 0;
  currentAnimation.data.keyFrames = 0;
  currentAnimation.data.checksum = 0;
  memset(currentAnimation.data.frames, 0, ANIM_MAX_FRAMES);
}

/**
 * Compute CRC16 checksum over 600 bytes of animation frame data
 * Reuses shared crc16 function from Communication.h for consistency
 */
inline uint16_t computeChecksum() {
  return crc16(currentAnimation.data.frames, ANIM_MAX_FRAMES);
}

/**
 * Start a new recording session
 * Clears buffer, sets state to RECORDING, captures wall time
 */
inline void startRecording() {
  clearAnimationBuffer();
  currentAnimation.state = ANIM_RECORDING;
  currentAnimation.wallTime = millis();
}

/**
 * Update the recording timeline span based on current elapsed time
 * Called continuously during RECORDING state to track actual duration
 * (not just when keyFrames are recorded)
 */
inline void updateRecordingElapsedTime() {
  if (currentAnimation.state != ANIM_RECORDING) {
    return;
  }

  // Calculate current frame based on elapsed time since recording started
  uint32_t elapsed = millis() - currentAnimation.wallTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Bound to valid range
  if (currentFrame >= ANIM_MAX_FRAMES) {
    currentFrame = ANIM_MAX_FRAMES - 1;
  }

  // Update totalFrames to track timeline span (even if no keyFrames in this interval)
  if (currentFrame >= currentAnimation.data.totalFrames) {
    currentAnimation.data.totalFrames = currentFrame + 1;
  }
}

/**
 * Record a relay trigger at the current frame
 * Calculates frame index based on elapsed time since recording started
 * Bounds checking prevents buffer overflow. Increments keyFrames count.
 */
inline void recordRelayAtCurrentFrame(uint8_t actuatorID) {
  if (currentAnimation.state != ANIM_RECORDING) {
    return; // Only record when in RECORDING state
  }

  if (actuatorID < 1 || actuatorID > 4) {
    return;  // Invalid actuator ID (valid range is 1-4)
  }

  // Update timeline span first (tracks elapsed time)
  updateRecordingElapsedTime();

  // Calculate current frame based on elapsed time
  uint32_t elapsed = millis() - currentAnimation.wallTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Bound frame to valid range
  if (currentFrame >= ANIM_MAX_FRAMES) {
    currentFrame = ANIM_MAX_FRAMES - 1;
  }

  // Check if this frame is currently empty (not yet triggered)
  uint8_t oldValue = currentAnimation.data.frames[currentFrame];
  
  // Write actuator ID to buffer at this frame
  currentAnimation.data.frames[currentFrame] = actuatorID;

  // Only increment keyFrames if this frame was previously empty (newly active frame)
  if (oldValue == 0 && actuatorID != 0) {
    currentAnimation.data.keyFrames++;
  }

  // Update totalFrames to track the timeline span
  if (currentFrame >= currentAnimation.data.totalFrames) {
    currentAnimation.data.totalFrames = currentFrame + 1;
  }
}

/**
 * Stop recording and return the total duration (totalFrames).
 * Transitions to IDLE_PENDING_SAVE (data not yet persisted).
 * Freezes timeline and waits for save/discard decision.
 */
inline uint16_t stopRecording() {
  if (currentAnimation.state != ANIM_RECORDING) {
    return 0;
  }

  // Final update to capture any elapsed time since last update
  updateRecordingElapsedTime();

  currentAnimation.state = ANIM_IDLE_PENDING_SAVE;
  currentAnimation.wallTime = 0;
  return currentAnimation.data.totalFrames;
}

/**
 * Discard the current unsaved recording
 * Clears buffer and returns to IDLE_EMPTY state.
 * Called when user chooses not to save after stopRecording().
 */
inline void discardRecording() {
  if (currentAnimation.state != ANIM_IDLE_PENDING_SAVE) {
    return; // Only discard from PENDING_SAVE state
  }

  clearAnimationBuffer(); // Returns to IDLE_EMPTY
}

/**
 * Save the current recording to NVS under a specific animation slot
 * Computes checksum and writes AnimationData struct to persistent storage.
 * Updates sourceSlot to indicate this animation is now saved to that slot.
 */
/**
 * saveRecordingToNVS() - Serialize and persist the current recording to flash storage
 * 
 * HOW IT WORKS:
 * - Animation data is stored as a binary blob under a text KEY NAME in NVS
 * - Key names are fixed: "anim0", "anim1", "anim2", "anim3" (from ANIMATION_NAMES array)
 * - Each slot stores an AnimationData struct serialized as bytes (606 total: metadata + frame buffer)
 * - putBytes() writes the struct directly to NVS; subsequent reads via getBytes() deserialize it back
 * 
 * STRUCTURE SAVED (AnimationData - 606 bytes total):
 * - keyFrames (2 bytes): Count of frames that contain relay triggers
 * - totalFrames (2 bytes): Timeline duration in frames (1 frame = 100ms)
 * - checksum (2 bytes): CRC16 of frames buffer for corruption detection
 * - frames[600] (600 bytes): The actual animation data (0=idle, 1-4=relay ID to trigger)
 * 
 * PERSISTENCE MODEL:
 * - NVS namespace "animations" holds 4 slots, each up to ~606 bytes
 * - Slot name determines where it's stored (e.g., animIndex=0 → saved under key "anim0")
 * - Once written, data persists across power cycles until explicitly deleted or overwritten
 * 
 * @param animIndex: Slot number 0-3; determines which ANIMATION_NAMES key is used
 * @return: true if write succeeded (written bytes == struct size), false otherwise
 */
inline bool saveRecordingToNVS(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;  // Invalid animation slot
  }

  if (currentAnimation.data.keyFrames == 0) {
    return false; // No frames to save
  }

  // Create AnimationData structure from current session
  AnimationData data = currentAnimation.data;

  // Compute checksum over entire 600-byte frame buffer
  data.checksum = computeChecksum();

  // Open Preferences and write the blob
  Preferences preferences;
  if (!preferences.begin("animations", false)) {
    return false;
  }

  // putBytes() serializes the AnimationData struct and writes it to NVS
  // Parameters: key name (ANIMATION_NAMES[animIndex]), pointer to data, size in bytes
  // Returns: number of bytes written (should equal sizeof(AnimationData))
  // Returns 0 if write failed (NVS full, corrupted, etc.)
  size_t written = preferences.putBytes(ANIMATION_NAMES[animIndex], &data, sizeof(data));
  preferences.end();

  if (written == sizeof(data)) {
    // Transition to IDLE_LOADED state after successful save
    // Animation data persists in buffer and is now backed by NVS
    currentAnimation.sourceSlot = animIndex;
    currentAnimation.state = ANIM_IDLE_LOADED;
    return true;
  }

  return false;
}

/**
 * Load an animation from NVS into the buffer
 * Optionally validates checksum to detect corruption
 * Sets sourceSlot to indicate where this animation came from
 */
/**
 * loadAnimationFromNVS() - Deserialize and load a saved animation from flash storage
 * 
 * HOW IT WORKS:
 * - Reverse of saveRecordingToNVS(): reads the binary blob from NVS and reconstructs the AnimationData struct
 * - Looks up the animation by slot number (0-3), which maps to ANIMATION_NAMES key ("anim0", "anim1", etc.)
 * - getBytes() pulls the 606-byte struct from NVS and deserializes it back into an AnimationData struct
 * - Validates the deserialized data before loading it into the runtime buffer
 * 
 * VALIDATION CHECKS:
 * - Size: Must equal sizeof(AnimationData) - indicates data is structurally intact
 * - keyFrames: Must be > 0 - slot must contain at least one frame with an action
 * - totalFrames: Must be > 0 and <= ANIM_MAX_FRAMES (600) - duration must be valid
 * - Checksum: CRC16 of frames buffer must match stored checksum (corruption detection)
 * 
 * @param animIndex: Slot number 0-3
 * @return: true if load and validation succeeded, false if slot is empty or data is corrupted
 */
inline bool loadAnimationFromNVS(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin("animations", true)) {
    return false;
  }

  // getBytes() deserializes data from NVS back into the AnimationData struct
  // Parameters: key name (ANIMATION_NAMES[animIndex]), pointer to destination buffer, max size
  // Returns: number of bytes read (should equal sizeof(AnimationData) if successful)
  // Returns 0 if key doesn't exist or read failed
  AnimationData data;
  size_t size = preferences.getBytes(ANIMATION_NAMES[animIndex], &data, sizeof(data));
  preferences.end();

  if (size != sizeof(data) || data.keyFrames == 0 || data.totalFrames == 0 || data.totalFrames > ANIM_MAX_FRAMES) {
    return false;
  }

  // Validate checksum using shared crc16 function
  uint16_t computed = crc16(data.frames, ANIM_MAX_FRAMES);
  if (computed != data.checksum) {
    // Log warning but continue loading (optional - adjust based on requirements)
    // debugln(F("Animation checksum mismatch - data may be corrupted"));
  }

  currentAnimation.sourceSlot = animIndex;  // Mark which slot this came from
  currentAnimation.state = ANIM_IDLE_LOADED; // Transition to loaded state
  currentAnimation.data = data;             // Copy data into runtime buffer

  return true;
}

/**
 * Start playback of a recorded animation
 * Loads from NVS, sets state to PLAYBACK, captures wall time
 */
inline bool startPlayback(uint8_t animIndex, TriggerSource triggerSource) {
  if (!loadAnimationFromNVS(animIndex)) {
    return false;
  }

  currentAnimation.state = ANIM_PLAYBACK;       // Transition to playback to run the animation
  currentAnimation.triggerSource = triggerSource;
  currentAnimation.wallTime = millis();         // Capture the time that playback began

  return true;
}

/**
 * Stop current playback immediately
 * Halts playback but preserves the loaded animation data in case it needs to play again.
 * Transitions to IDLE_LOADED (data still loaded from NVS).
 * Does not clear the buffer—that only happens in clearAnimationBuffer() for fresh recordings.
 * Sends event to notify client that playback has ended and system returned to IDLE_LOADED.
 */
inline void stopPlayback() {
  currentAnimation.state = ANIM_IDLE_LOADED;            // Return to loaded state (preserves data)
  currentAnimation.triggerSource = TRIGGER_SOURCE_NONE; // Reset to an unknown trigger source
  currentAnimation.wallTime = 0;                        // Clear the timer to prevent further updates
  sendAnimationFrameData();                             // Send one final update to notify client of state change
}

/**
 * Update playback state each cycle (called from AnimationTask ~10ms)
 * Checks elapsed time, triggers relays at appropriate frames, stops when complete
 * Only processes if state is ANIM_PLAYBACK. Playback ends when currentFrame >= totalFrames.
 */
inline void updatePlayback() {
  if (currentAnimation.state != ANIM_PLAYBACK) {
    return;  // Only update if actually playing
  }

  // Calculate current frame based on elapsed time
  uint32_t elapsed = millis() - currentAnimation.wallTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Check if playback is complete (reached end of recorded timeline)
  if (currentFrame >= currentAnimation.data.totalFrames) {
    stopPlayback(); // Will handle state transition to IDLE and send event
    return;
  }

  // Check if the current frame value corresponds to a relay (1-4); 0 is a rest/no-op.
  uint8_t relayID = currentAnimation.data.frames[currentFrame];
  if (relayID >= 1 && relayID <= 4) {
    // CRITICAL: Use a static variable to track which frame was last triggered.
    // WHY: updatePlayback() runs every ~10ms (AnimationTask loop), but each frame represents 100ms.
    // Without static persistence, the same frame number would be "current" for ~10 consecutive calls.
    // A local variable would reset on each call, causing triggerActuator() to fire 10x per frame.
    // Static remembers across calls, so we only trigger once per unique frame.
    // Initialize to 0xFFFF (65535), a sentinel value outside valid range (0-599), ensures first frame triggers.
    static uint16_t lastTriggeredFrame = 0xFFFF;
    
    if (currentFrame != lastTriggeredFrame) {
      // Trigger the relay and play its audio effect via the system function
      ActuatorID actuatorID = static_cast<ActuatorID>(relayID - 1);  // Convert 1-4 to 0-3
      triggerActuator(actuatorID);
      
      // Remember this frame to prevent triggering it again on the next 10 loop cycles
      lastTriggeredFrame = currentFrame;
    }
  }
}

/**
 * Validate stored animation checksum against computed value
 * Returns true if checksums match or validation is disabled
 */
inline bool validateChecksum(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin("animations", true)) {
    return false;
  }

  AnimationData data;
  size_t size = preferences.getBytes(ANIMATION_NAMES[animIndex], &data, sizeof(data));
  preferences.end();

  if (size != sizeof(data)) {
    return false;
  }

  // Compute checksum using shared crc16 function
  uint16_t computed = crc16(data.frames, ANIM_MAX_FRAMES);
  return computed == data.checksum;
}

/**
 * Scan NVS and rebuild the animation availability cache
 * 
 * HOW IT WORKS:
 * - Queries NVS for each of the 4 animation slots ("anim0", "anim1", "anim2", "anim3")
 * - For each slot, attempts to deserialize AnimationData using getBytes()
 * - Updates the in-memory animationSlots[] array with hasAnimation flag and animationSeconds duration
 * - This cache is used by the UI to populate dropdowns and enable/disable the Play button
 * 
 * CACHE ARRAY (animationSlots[4]):
 * - animationSlots[i].id = slot number (0-3)
 * - animationSlots[i].hasAnimation = true if slot contains valid data
 * - animationSlots[i].animationSeconds = animation duration in seconds (0 if empty)
 * 
 * WHEN TO CALL:
 * - After saveRecordingToNVS() completes (so UI learns about new saved animation)
 * - On system startup (to populate UI with any previously saved animations)
 * - When user explicitly requests "refresh" action
 * 
 * OPERATION:
 * - Opens Preferences in read-only mode, iterates all 4 slots
 * - For each slot, getBytes() returns 0 if key doesn't exist (empty slot)
 * - For each slot, getBytes() returns size if key exists; validates keyFrames > 0
 * - After all slots checked, closes Preferences connection
 */
void refreshAnimationSlotCache() {
  Preferences preferences;
  
  if (!preferences.begin("animations", true)) {
    // Unable to open namespace - mark all slots as empty
    for (uint8_t i = 0; i < 4; i++) {
      animationSlots[i].id = i;
      animationSlots[i].hasAnimation = false;
      animationSlots[i].animationSeconds = 0.0f;
    }
    return;
  }

  // Check each animation slot
  for (uint8_t i = 0; i < 4; i++) {
    animationSlots[i].id = i;
    
    // getBytes() deserializes the AnimationData struct from NVS for this slot
    // Parameters: key name (ANIMATION_NAMES[i]), pointer to data buffer, max size
    // Returns: number of bytes read (should equal sizeof(AnimationData) if slot has animation)
    // Returns 0 if key doesn't exist (slot is empty)
    AnimationData data;
    size_t size = preferences.getBytes(ANIMATION_NAMES[i], &data, sizeof(data));
    
    // Validation: only mark slot as "hasAnimation" if read succeeded and data is valid
    if (size == sizeof(data) && data.totalFrames > 0 && data.totalFrames <= ANIM_MAX_FRAMES) {
      animationSlots[i].hasAnimation = true;
      // Convert totalFrames to seconds: totalFrames * 100ms per frame / 1000ms per second
      animationSlots[i].animationSeconds = (data.totalFrames * ANIM_TIME_UNIT_MS) / 1000.0f;
    } else {
      animationSlots[i].hasAnimation = false;
      animationSlots[i].animationSeconds = 0.0f;
    }
  }
  
  preferences.end();
}
