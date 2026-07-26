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
 * - Frame Buffer:     currentAnimation.buffer[600] holds frame data (0=idle, 1-4=relay ID)
 * - Frame Timing:     Each frame = 100ms. At 100fps, max duration = 1 minute
 * - Recording Mode:   Automatically captures relay triggers via recordRelayAtCurrentFrame()
 * - Playback Mode:    Replays recorded frames by calculating elapsed time and firing relays
 * - NVS Persistence:  AnimationData struct stored with CRC16 checksum for validation
 * - Button Binding:   4 NVS slots map directly to 4 RF buttons (slot 0→button 1, etc.)
 *
 * SESSION vs PERSISTENT DATA:
 * - AnimationSession (currentAnimation):      Runtime state (buffer, keyFrames, mode, timing)
 * - AnimationData (NVS):          Persistent struct (keyFrames, checksum, frames[600])
 *
 * AUTOMATIC RECORDING:
 * When currentAnimation.mode == ANIM_RECORDING, any call to recordRelayAtCurrentFrame(actuatorID)
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
#include "Header.h"
#include <Communication.h>

// Forward declarations
bool triggerActuator(ActuatorID actuatorID);

/**
 * Clear the animation buffer and reset runtime state
 */
inline void clearBuffer() {
  memset(currentAnimation.buffer, 0, sizeof(currentAnimation.buffer));
  currentAnimation.keyFrames = 0;
  currentAnimation.startTime = 0;
  currentAnimation.mode = ANIM_IDLE;
  currentAnimation.sourceSlot = -1;
}

/**
 * Compute CRC16 checksum over 600 bytes of animation frame data
 * Reuses shared crc16 function from Communication.h for consistency
 */
inline uint16_t computeChecksum() {
  return crc16(currentAnimation.buffer, ANIM_MAX_FRAMES);
}

/**
 * Start a new recording session
 * Clears buffer, sets mode to RECORDING, captures start time, marks as fresh recording
 */
inline void startRecording() {
  clearBuffer();
  currentAnimation.mode = ANIM_RECORDING;
  currentAnimation.startTime = millis();
  currentAnimation.keyFrames = 0;
  currentAnimation.sourceSlot = -1; // Mark as fresh recording (not yet saved to any slot)
}

/**
 * Record a relay trigger at the current frame
 * Calculates frame index based on elapsed time since recording started
 * Bounds checking prevents buffer overflow
 */
inline void recordRelayAtCurrentFrame(uint8_t actuatorID) {
  if (currentAnimation.mode != ANIM_RECORDING) {
    return;  // Only record when in RECORDING mode
  }

  if (actuatorID < 1 || actuatorID > 4) {
    return;  // Invalid actuator ID (valid range is 1-4)
  }

  // Calculate current frame based on elapsed time
  uint32_t elapsed = millis() - currentAnimation.startTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Bound frame to valid range
  if (currentFrame >= ANIM_MAX_FRAMES) {
    currentFrame = ANIM_MAX_FRAMES - 1;
  }

  // Write actuator ID to buffer at this frame
  currentAnimation.buffer[currentFrame] = actuatorID;

  // Update keyFrames if this is a new frame we haven't recorded
  if (currentFrame >= currentAnimation.keyFrames) {
    currentAnimation.keyFrames = currentFrame + 1;
  }
}

/**
 * Stop recording and return the number of frames recorded
 */
inline uint16_t stopRecording() {
  if (currentAnimation.mode != ANIM_RECORDING) {
    return 0;
  }

  currentAnimation.mode = ANIM_IDLE;
  return currentAnimation.keyFrames;
}

/**
 * Save the current recording to NVS under a specific animation slot
 * Computes checksum and writes AnimationData struct to persistent storage.
 * Updates sourceSlot to indicate this animation is now saved to that slot.
 */
inline bool saveRecordingToNVS(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;  // Invalid animation slot
  }

  if (currentAnimation.keyFrames == 0) {
    return false;  // Nothing to save
  }

  // Create AnimationData structure
  AnimationData data;
  data.keyFrames = currentAnimation.keyFrames;
  memcpy(data.frames, currentAnimation.buffer, sizeof(data.frames));

  // Compute checksum over entire 600-byte frame buffer
  data.checksum = computeChecksum();

  // Open Preferences and write the blob
  Preferences preferences;
  if (!preferences.begin("animations", false)) {
    return false;
  }

  size_t written = preferences.putBytes(ANIMATION_NAMES[animIndex], &data, sizeof(data));
  preferences.end();

  if (written == sizeof(data)) {
    // Update sourceSlot to indicate this recording is now saved to this slot
    currentAnimation.sourceSlot = animIndex;
    return true;
  }

  return false;
}

/**
 * Load an animation from NVS into the buffer
 * Optionally validates checksum to detect corruption
 * Sets sourceSlot to indicate where this animation came from
 */
inline bool loadAnimationFromNVS(uint8_t animIndex) {
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

  if (size != sizeof(data) || data.keyFrames == 0 || data.keyFrames > ANIM_MAX_FRAMES) {
    return false;
  }

  // Validate checksum using shared crc16 function
  uint16_t computed = crc16(data.frames, ANIM_MAX_FRAMES);
  if (computed != data.checksum) {
    // Log warning but continue loading (optional - adjust based on requirements)
    // debugln(F("Animation checksum mismatch - data may be corrupted"));
  }

  // Copy data into runtime buffer
  memcpy(currentAnimation.buffer, data.frames, sizeof(data.frames));
  currentAnimation.keyFrames = data.keyFrames;
  currentAnimation.mode = ANIM_IDLE;  // Leave in IDLE, caller will set to PLAYBACK
  currentAnimation.sourceSlot = animIndex;  // Mark which slot this came from

  return true;
}

/**
 * Start playback of a recorded animation
 * Loads from NVS, sets mode to PLAYBACK, captures start time, records source slot
 */
inline bool startPlayback(uint8_t animIndex) {
  if (!loadAnimationFromNVS(animIndex)) {
    return false;
  }

  currentAnimation.mode = ANIM_PLAYBACK;
  currentAnimation.startTime = millis();
  currentAnimation.sourceSlot = animIndex;  // Explicitly mark which slot we're playing from

  return true;
}

/**
 * Stop current playback immediately
 */
inline void stopPlayback() {
  currentAnimation.mode = ANIM_IDLE;
  currentAnimation.startTime = 0;
  currentAnimation.keyFrames = 0;
  currentAnimation.sourceSlot = -1;
  memset(currentAnimation.buffer, 0, sizeof(currentAnimation.buffer));
}

/**
 * Update playback state each cycle (called from AnimationTask ~10ms)
 * Checks elapsed time, triggers relays at appropriate frames, stops when complete
 * Only processes if mode is ANIM_PLAYBACK
 */
inline void updatePlayback() {
  if (currentAnimation.mode != ANIM_PLAYBACK) {
    return;  // Only update if actually playing
  }

  // Calculate current frame based on elapsed time
  uint32_t elapsed = millis() - currentAnimation.startTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Check if playback is complete
  if (currentFrame >= currentAnimation.keyFrames) {
    stopPlayback();
    return;
  }

  // Check if there's a relay to trigger at this frame
  uint8_t relayID = currentAnimation.buffer[currentFrame];
  if (relayID >= 1 && relayID <= 4) {
    // Only trigger if this frame is different from the previous one
    // (prevents repeated triggers for multi-cycle frames)
    uint16_t prevFrame = (currentFrame > 0) ? currentFrame - 1 : 0xFFFF;
    uint8_t prevRelayID = (currentFrame > 0) ? currentAnimation.buffer[prevFrame] : 0;

    if (relayID != prevRelayID) {
      // Trigger the relay via the system function
      ActuatorID actuatorID = static_cast<ActuatorID>(relayID - 1);  // Convert 1-4 to 0-3
      triggerActuator(actuatorID);

      // NOTE: Recording check happens in calling function (handleActuator) for web API
      // During playback, we're just replaying recorded animation, so no additional recording needed
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
