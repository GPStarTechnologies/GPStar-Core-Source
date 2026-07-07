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

// Helper: Get the relay channel for a given actuator ID
RelayChannel* getRelayForActuator(ActuatorID actuatorID) {
  switch(actuatorID) {
    case ACTUATOR_1: return &devices.relay1;
    case ACTUATOR_2: return &devices.relay2;
    case ACTUATOR_3: return &devices.relay3;
    case ACTUATOR_4: return &devices.relay4;
  }
  return nullptr; // Should never reach here with valid enum
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

// Check if activating the given actuator would violate forbidden pair constraints
bool wouldCreateForbiddenPair(ActuatorID actuatorID) {
  for(uint8_t i = 0; i < FORBIDDEN_PAIRS_COUNT; i++) {
    ActuatorID opposingID = ACTUATOR_1; // Default, will be overwritten
    
    if(actuatorID == forbiddenPairs[i].element1) {
      opposingID = forbiddenPairs[i].element2;
    }
    else if(actuatorID == forbiddenPairs[i].element2) {
      opposingID = forbiddenPairs[i].element1;
    }
    else {
      continue; // Not part of this forbidden pair
    }
    
    // Check if the opposing actuator is already active
    RelayChannel* opposingRelay = getRelayForActuator(opposingID);
    if(opposingRelay && opposingRelay->state.relayActive) {
      return true;
    }
  }
  return false;
}

// Trigger the specified actuator (given as ActuatorID enum 0-3) for a fixed duration (defined by ACTUATOR_PULSE_MS).
bool triggerActuator(ActuatorID actuatorID) {
    // Check if activating this actuator would violate forbidden pair constraints
    if(wouldCreateForbiddenPair(actuatorID)) {
      debugln(F("Cannot activate - forbidden pair constraint violated"));
      return false;
    }

    // Play a random sound effect for this actuator trigger.
    debugln(String(F("Triggering Actuator ")) + String(actuatorID + 1));
    uint8_t soundEffects[] = {S_PING1, S_PING2, S_PING3, S_PING4};
    playEffect(soundEffects[random(4)]);

    // Activate the relay by first looking up the corresponding channel.
    RelayChannel* relay = getRelayForActuator(actuatorID);
    relay->state.relayActive = true;
    relay->state.relayOffTime = millis() + ACTUATOR_PULSE_MS;

    return true;
}
