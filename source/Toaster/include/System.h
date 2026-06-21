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

void sendDebug(const String& message);

// Clear any prior information from the WebSocket client.
void resetWebSocketData() {
  wsData.mode = "";
  wsData.theme = "";
  wsData.switchState = "";
  wsData.pack = "";
  wsData.safety = "";
  wsData.wandPower = 5; // Default to max power.
  wsData.wandMode = "";
  wsData.firing = "";
  wsData.cable = "";
  wsData.cyclotron = "";
  wsData.temperature = "";
}

// Obtain a list of partitions for this device.
void printPartitions() {
  const esp_partition_t *partition;
  esp_partition_iterator_t iterator = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if(iterator == nullptr) {
    debugln(F("No partitions found."));
    return;
  }

  debugln(F("Partitions:"));
  while(iterator != nullptr) {
    partition = esp_partition_get(iterator);
    debugf("Label: %s, Size: %lu bytes, Address: 0x%08lx\n",
                  partition->label,
                  partition->size,
                  partition->address);
    iterator = esp_partition_next(iterator);
  }

  esp_partition_iterator_release(iterator);  // Release the iterator once done
}

// Trigger the specified actuator (given 1-4) for a fixed duration (defined by ACTUATOR_PULSE_MS).
bool triggerActuator(uint8_t actuatorID) {
    if ((actuatorID < 1) || (actuatorID > 4)) {
      return false;
    }  

    // Obtain the correct index (0-3) and output a signal to the relay.
    uint8_t index = actuatorID - 1; // Convert to 0-based index.

    // Play a sound effect corresponding to the triggered actuator.
    switch(index) {
      case 0:
        debugln(F("Triggering Actuator 1"));
        playEffect(S_PING1);
        break;
      case 1:
        debugln(F("Triggering Actuator 2"));
        playEffect(S_PING2);
        break;
      case 2:
        debugln(F("Triggering Actuator 3"));
        playEffect(S_PING3);
        break;
      case 3:
        debugln(F("Triggering Actuator 4"));
        playEffect(S_PING4);
        break;
    }

    // Mark for activation and the AnimationTask will handle turning the relay on or off.
    actuator[index].relayActive = true;

    // If the actuator is already active, this simply extends the time until powered off.
    actuator[index].relayOffTime = millis() + ACTUATOR_PULSE_MS;

    return true;
}
