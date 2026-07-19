/**
 *   GPStar Toaster - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024-2026 Dustin Grau <dustin.grau@gmail.com>
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
 * - Frame Buffer:     anim.buffer[600] holds frame data (0=idle, 1-4=relay ID)
 * - Frame Timing:     Each frame = 100ms. At 100fps, max duration = 1 minute
 * - Recording Mode:   Automatically captures relay triggers via recordRelayAtCurrentFrame()
 * - Playback Mode:    Replays recorded frames by calculating elapsed time and firing relays
 * - NVS Persistence:  AnimationData struct stored with CRC16 checksum for validation
 * - Button Binding:   4 NVS slots map directly to 4 RF buttons (slot 0→button 1, etc.)
 * 
 * SESSION vs PERSISTENT DATA:
 * - AnimationSession (anim):      Runtime state (buffer, frameCount, mode, timing)
 * - AnimationData (NVS):          Persistent struct (frameCount, checksum, frames[600])
 * 
 * AUTOMATIC RECORDING:
 * When anim.mode == ANIM_RECORDING, any call to recordRelayAtCurrentFrame(actuatorID)
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
#include <nvs_flash.h>
#include <nvs.h>

// Forward declarations
bool triggerActuator(ActuatorID actuatorID);

// Global animation session instance
// Shared across main.cpp, AnimationTask, and UserInputTask
AnimationSession anim = {};

/**
 * Clear the animation buffer and reset runtime state
 */
inline void clearBuffer() {
  memset(anim.buffer, 0, sizeof(anim.buffer));
  anim.frameCount = 0;
  anim.startTime = 0;
  anim.mode = ANIM_IDLE;
}

/**
 * Compute CRC16 checksum over 600 bytes of animation frame data
 * Reuses shared crc16 function from Communication.h for consistency
 */
inline uint16_t computeChecksum() {
  return crc16(anim.buffer, ANIM_MAX_FRAMES);
}

/**
 * Start a new recording session
 * Clears buffer, sets mode to RECORDING, captures start time
 */
inline void startRecording() {
  clearBuffer();
  anim.mode = ANIM_RECORDING;
  anim.startTime = millis();
  anim.frameCount = 0;
}

/**
 * Record a relay trigger at the current frame
 * Calculates frame index based on elapsed time since recording started
 * Bounds checking prevents buffer overflow
 */
inline void recordRelayAtCurrentFrame(uint8_t actuatorID) {
  if (anim.mode != ANIM_RECORDING) {
    return;  // Only record when in RECORDING mode
  }

  if (actuatorID < 1 || actuatorID > 4) {
    return;  // Invalid actuator ID (valid range is 1-4)
  }

  // Calculate current frame based on elapsed time
  uint32_t elapsed = millis() - anim.startTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Bound frame to valid range
  if (currentFrame >= ANIM_MAX_FRAMES) {
    currentFrame = ANIM_MAX_FRAMES - 1;
  }

  // Write actuator ID to buffer at this frame
  anim.buffer[currentFrame] = actuatorID;

  // Update frameCount if this is a new frame we haven't recorded
  if (currentFrame >= anim.frameCount) {
    anim.frameCount = currentFrame + 1;
  }
}

/**
 * Stop recording and return the number of frames recorded
 */
inline uint16_t stopRecording() {
  if (anim.mode != ANIM_RECORDING) {
    return 0;
  }

  anim.mode = ANIM_IDLE;
  return anim.frameCount;
}

/**
 * Save the current recording to NVS under a specific animation slot
 * Computes checksum and writes AnimationData struct to persistent storage
 */
inline bool saveRecordingToNVS(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;  // Invalid animation slot
  }

  if (anim.frameCount == 0) {
    return false;  // Nothing to save
  }

  // Create AnimationData structure
  AnimationData data;
  data.frameCount = anim.frameCount;
  memcpy(data.frames, anim.buffer, sizeof(data.frames));

  // Compute checksum over entire 600-byte frame buffer
  data.checksum = computeChecksum();

  // Open NVS and write the blob
  nvs_handle_t handle;
  esp_err_t err = nvs_open("animations", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    return false;
  }

  err = nvs_set_blob(handle, ANIMATION_NAMES[animIndex], &data, sizeof(data));
  if (err != ESP_OK) {
    nvs_close(handle);
    return false;
  }

  err = nvs_commit(handle);
  nvs_close(handle);

  if (err == ESP_OK) {
    // Clear the session after successful save
    clearBuffer();
    return true;
  }

  return false;
}

/**
 * Load an animation from NVS into the buffer
 * Optionally validates checksum to detect corruption
 */
inline bool loadAnimationFromNVS(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;
  }

  nvs_handle_t handle;
  esp_err_t err = nvs_open("animations", NVS_READONLY, &handle);
  if (err != ESP_OK) {
    return false;
  }

  AnimationData data;
  size_t size = sizeof(data);
  err = nvs_get_blob(handle, ANIMATION_NAMES[animIndex], &data, &size);
  nvs_close(handle);

  if (err != ESP_OK) {
    return false;
  }

  if (size != sizeof(data) || data.frameCount == 0 || data.frameCount > ANIM_MAX_FRAMES) {
    return false;
  }

  // Validate checksum using shared crc16 function
  uint16_t computed = crc16(data.frames, ANIM_MAX_FRAMES);
  if (computed != data.checksum) {
    // Log warning but continue loading (optional - adjust based on requirements)
    // debugln(F("Animation checksum mismatch - data may be corrupted"));
  }

  // Copy data into runtime buffer
  memcpy(anim.buffer, data.frames, sizeof(data.frames));
  anim.frameCount = data.frameCount;
  anim.mode = ANIM_IDLE;  // Leave in IDLE, caller will set to PLAYBACK

  return true;
}

/**
 * Start playback of a recorded animation
 * Loads from NVS, sets mode to PLAYBACK, captures start time
 */
inline bool startPlayback(uint8_t animIndex) {
  if (!loadAnimationFromNVS(animIndex)) {
    return false;
  }

  anim.mode = ANIM_PLAYBACK;
  anim.startTime = millis();

  return true;
}

/**
 * Stop current playback immediately
 */
inline void stopPlayback() {
  anim.mode = ANIM_IDLE;
  anim.startTime = 0;
  anim.frameCount = 0;
  memset(anim.buffer, 0, sizeof(anim.buffer));
}

/**
 * Update playback state each cycle (called from AnimationTask ~10ms)
 * Checks elapsed time, triggers relays at appropriate frames, stops when complete
 * Only processes if mode is ANIM_PLAYBACK
 */
inline void updatePlayback() {
  if (anim.mode != ANIM_PLAYBACK) {
    return;  // Only update if actually playing
  }

  // Calculate current frame based on elapsed time
  uint32_t elapsed = millis() - anim.startTime;
  uint16_t currentFrame = elapsed / ANIM_TIME_UNIT_MS;

  // Check if playback is complete
  if (currentFrame >= anim.frameCount) {
    stopPlayback();
    return;
  }

  // Check if there's a relay to trigger at this frame
  uint8_t relayID = anim.buffer[currentFrame];
  if (relayID >= 1 && relayID <= 4) {
    // Only trigger if this frame is different from the previous one
    // (prevents repeated triggers for multi-cycle frames)
    uint16_t prevFrame = (currentFrame > 0) ? currentFrame - 1 : 0xFFFF;
    uint8_t prevRelayID = (currentFrame > 0) ? anim.buffer[prevFrame] : 0;

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
 * Get the current animation mode
 */
inline uint8_t getMode() {
  return anim.mode;
}

/**
 * Get the frame count of the current animation in buffer
 */
inline uint16_t getFrameCount() {
  return anim.frameCount;
}

/**
 * Validate stored animation checksum against computed value
 * Returns true if checksums match or validation is disabled
 */
inline bool validateChecksum(uint8_t animIndex) {
  if (animIndex >= ANIM_MAX_STORED) {
    return false;
  }

  nvs_handle_t handle;
  esp_err_t err = nvs_open("animations", NVS_READONLY, &handle);
  if (err != ESP_OK) {
    return false;
  }

  AnimationData data;
  size_t size = sizeof(data);
  err = nvs_get_blob(handle, ANIMATION_NAMES[animIndex], &data, &size);
  nvs_close(handle);

  if (err != ESP_OK || size != sizeof(data)) {
    return false;
  }

  // Compute checksum using shared crc16 function
  uint16_t computed = crc16(data.frames, ANIM_MAX_FRAMES);
  return computed == data.checksum;
}
