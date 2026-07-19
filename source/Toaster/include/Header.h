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

#pragma once

#define BUILT_IN_LED 2

/**
 * RF Inputs
 * ---------
 * GPIO34 (Input-Only)
 * GPIO33 (Normal In/Out)
 * GPIO35 (Input-Only)
 * GPIO39 (Input-Only, SVN)
 */
#define RF1_PIN 34
#define RF2_PIN 33
#define RF3_PIN 35
#define RF4_PIN 39

/**
 * Relay Outputs
 * -------------
 * GPIO25
 * GPIO26
 * GPIO27
 * GPIO32
 */
#define RELAY1_PIN 25
#define RELAY2_PIN 26
#define RELAY3_PIN 27
#define RELAY4_PIN 32

/**
 * Actuator Locations/IDs
 * Note that the physical layaout means we should prevent diagonal pairs
 * from being active at the same time to avoid destabilizing the device.
 *    _______
 *   | 1   2 |
 *   |       |
 *   |       |
 *   |       |
 * ==| 3   4 |
 *    ‾‾‾‾‾‾‾
 */
enum ActuatorID {
  ACTUATOR_1 = 0,
  ACTUATOR_2 = 1,
  ACTUATOR_3 = 2,
  ACTUATOR_4 = 3
};

/**
 * Forbidden Actuator Pairs
 * Defines which actuators cannot be active simultaneously to prevent device destabilization.
 * Diagonal pairs (1/4 and 2/3) are forbidden as they create opposing forces.
 */
struct ForbiddenActuatorPair {
  ActuatorID element1;  // First element of forbidden pair
  ActuatorID element2;  // Second element of forbidden pair
};
const ForbiddenActuatorPair forbiddenPairs[] = {
  {ACTUATOR_1, ACTUATOR_4},
  {ACTUATOR_2, ACTUATOR_3}
};
const uint8_t FORBIDDEN_PAIRS_COUNT = 2;

/**
 * ActuatorState
 * Holds the current state of each relay actuator, including whether it is active and the time it should turn off (for timed activations).
 */
struct ActuatorState {
  bool relayActive;
  uint32_t relayOffTime;
};

/**
 * RelayChannel
 * Represents a single relay output with its hardware pin and state.
 */
struct RelayChannel {
  uint8_t pin;
  ActuatorState state;
};

const uint16_t ACTUATOR_PULSE_MS = 300;

/**
 * RFInputState
 * Tracks the state of RF inputs for debouncing and edge detection.
 */
struct RFInputState {
  bool currentState;     // Current debounced state (HIGH=true, LOW=false)
  bool previousState;    // Previous debounced state for edge detection
  uint8_t debounceCount; // Counter for debouncing (requires consistent reads)
};

/**
 * RFButtonChannel
 * Represents a single RF input button with its hardware pin and state.
 */
struct RFButtonChannel {
  uint8_t pin;
  RFInputState state;
};

const uint8_t RF_DEBOUNCE_THRESHOLD = 3; // Number of consistent reads required for state change
const uint8_t RF_DEBOUNCE_MAX = 5;       // Maximum debounce count to prevent overflow

/**
 * Devices
 * Contains all relay outputs and RF input buttons as explicit named members instead of arrays.
 * This eliminates array indexing confusion and makes the hardware layout explicit.
 */
struct Devices {
  // Relay outputs (GPIO 25, 26, 27, 32)
  RelayChannel relay1;
  RelayChannel relay2;
  RelayChannel relay3;
  RelayChannel relay4;

  // RF input buttons (GPIO 34, 33, 35, 39)
  RFButtonChannel button1;
  RFButtonChannel button2;
  RFButtonChannel button3;
  RFButtonChannel button4;
};

// Global device instance initialized with pin assignments
Devices devices = {
  // Relays
  {RELAY1_PIN, {false, 0}},  // relay1
  {RELAY2_PIN, {false, 0}},  // relay2
  {RELAY3_PIN, {false, 0}},  // relay3
  {RELAY4_PIN, {false, 0}},  // relay4
  // RF Buttons
  {RF1_PIN, {false, false, 0}},  // button1
  {RF2_PIN, {false, false, 0}},  // button2
  {RF3_PIN, {false, false, 0}},  // button3
  {RF4_PIN, {false, false, 0}}   // button4
};

/**
 * Animation System
 * Records and plays back sequences of relay actuations at 100ms intervals.
 */

enum AnimationMode : uint8_t {
  ANIM_IDLE = 0,
  ANIM_RECORDING = 1,
  ANIM_PLAYBACK = 2
};

/**
 * AnimationData - Persistent structure stored in NVS
 * Contains the recorded animation frames and metadata.
 */
struct AnimationData {
  uint16_t frameCount;      // How many frames were recorded (0-600)
  uint16_t checksum;        // CRC16 of frames[] for optional validation against NVS
  uint8_t frames[600];      // The recorded frame data (0 = no action, 1-4 = relay ID)
};

/**
 * AnimationSession - Runtime state used during recording/playback
 * Not persisted; recreated from NVS data when needed.
 */
struct AnimationSession {
  uint8_t buffer[600];      // Current animation being recorded or played
  uint16_t frameCount;      // How many frames in current animation
  uint32_t startTime;       // When recording/playback started (for timing)
  uint8_t mode;             // Current mode (IDLE, RECORDING, PLAYBACK)
};

// Animation Constants
const uint16_t ANIM_MAX_FRAMES = 600;        // 1 minute @ 100ms
const uint16_t ANIM_TIME_UNIT_MS = 100;      // Frame duration in milliseconds
const uint8_t ANIM_MAX_STORED = 4;           // One per RF button
const char* ANIMATION_NAMES[4] = {"anim1", "anim2", "anim3", "anim4"};
