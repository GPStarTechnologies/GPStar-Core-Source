/**
 *   GPStar BeltGizmo - Ghostbusters Props, Mods, and Kits.
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

/*
 * As an alternative to the standard ESP32 dev board is the Waveshare ESP32-S3 Mini:
 * https://www.waveshare.com/wiki/ESP32-S3-Zero
 */

/*
 * Built-in LED (non-addressable)
 */
#define BUILT_IN_LED 21 // GPIO21 for Waveshare ESP32-S3 Mini (RGB LED)

/*
 * Define Color Options & Timers
 */
#define ANIMATION_DURATION_MS 800  // Time for a full end-to-end animation
millisDelay ms_anim_change;
const uint16_t i_animation_time = 400;
const uint8_t i_animation_step = 4;
bool b_invert_animation = true; // false = Right to Left, true = Left to Right
static const uint8_t i_colour_count = 4; // Total number of colour available.
static const uint16_t i_selftest_interval = 2000; // 2 seconds between colour changes.
millisDelay ms_selftest_cycle; // Timer for self-test cycling using an interval.
uint8_t i_selftest_colour = 0; // Current colour index for cycling in self-test.
uint8_t i_stream_colour; // Current colour index for the stream type.

// Animation duration calculated based on i_num_leds from LightConfig.h
// Will be initialized in setup() after LightConfig.h variables are available
extern uint8_t i_num_leds;
uint16_t i_animation_duration; // Initialized in setup()

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

/*
 * Special States
 */
bool b_firing = false;
