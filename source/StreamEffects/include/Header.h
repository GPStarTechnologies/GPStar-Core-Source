/**
 *   GPStar Stream Effects - Ghostbusters Props, Mods, and Kits.
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
 * Special Values/States
 */
uint8_t i_default_wand_power = 1; // Default wandPower level (1-5), default 1 (for testing)
bool b_firing = false;
