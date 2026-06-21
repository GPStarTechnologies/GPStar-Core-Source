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
const uint8_t relayPins[4] = {
  RELAY1_PIN,
  RELAY2_PIN,
  RELAY3_PIN,
  RELAY4_PIN
};

/**
 * ActuatorState
 * Holds the current state of each actuator, including whether it is active and the time it should turn off (for timed activations).
 */
struct ActuatorState {
  bool relayActive;
  uint32_t relayOffTime;
};
ActuatorState actuator[4];
const uint16_t ACTUATOR_PULSE_MS = 400;

/**
 * WebSocketData - Holds all relevant fields received from the WebSocket JSON payload.
 */
struct WebSocketData {
  String mode = "";
  String theme = "";
  String switchState = "";
  String pack = "";
  String safety = "";
  uint8_t wandPower = 5; // Default to max power.
  String wandMode = "";
  String firing = "";
  bool ctsActive = false; // Default to not crossing streams.
  String cable = "";
  String cyclotron = "";
  bool cyclotronLid = true; // Default to lid on.
  String temperature = "";
};
WebSocketData wsData; // Instance of WebSocketData struct.
