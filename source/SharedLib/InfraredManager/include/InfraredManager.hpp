/**
 *   Infrared - IR communication methods for GPStar devices.
 *   Handles all sending/receiving logic for Infrared devices.
 *   Copyright (C) 2023-2026 Michael Rajotte, Dustin Grau, Nomake Wan
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

// Only compile this code for ESP32 or ATTINY1616 platforms
#if (defined(ESP32) || defined(__AVR_ATtiny1616__))

#include "InfraredManager.h"

/**
 * Infrared Manager Class Interface
 *
 * NOTE: This class requires WirelessManager to be initialized first to provide the deviceID,
 * which is derived from the WiFi adapter's MAC address for unique device identification.
 */

InfraredManager::InfraredManager(uint8_t deviceType, uint16_t deviceID)
  : m_deviceType(deviceType), m_deviceID(deviceID) {
  // Initialize IR LED pin
  pinMode(IR_LED_PIN, OUTPUT);
  digitalWrite(IR_LED_PIN, LOW);
  IrSender.begin(IR_LED_PIN);

  #ifndef GPSTAR_IR_TX_ONLY
  // If not in TX-only mode, also set up the receiver pin and timer.
  IrReceiver.begin(IR_RECEIVER_PIN);
  rx_timer.start(0);
  #endif

  // Initialize and start the timer with 0 delay
  tx_timer.start(0);

#ifdef ESP32
  m_EntriesAvailable = loadFromPreferences();
#endif
}

void InfraredManager::setDeviceID(uint16_t deviceID) {
  m_deviceID = deviceID;
}

uint16_t InfraredManager::getDeviceID() const {
  return m_deviceID;
}

uint8_t InfraredManager::getDeviceType() const {
  return m_deviceType;
}

void InfraredManager::startTXTimer(uint16_t delay) {
  tx_timer.start(delay);
}

void InfraredManager::restartTXTimer() {
  tx_timer.repeat();
}

void InfraredManager::stopTXTimer() {
  tx_timer.stop();
}

bool InfraredManager::isTXTimerExpired() {
  return tx_timer.justFinished();
}

#ifdef ESP32
bool InfraredManager::isValidCommand(const IR_COMMAND_ENTRY& entry) const {
  uint8_t nameLength = 0;
  while(nameLength <= IR_COMMAND_NAME_MAX && entry.name[nameLength] != '\0') {
    nameLength++;
  }

  if(nameLength == 0 || nameLength > IR_COMMAND_NAME_MAX) {
    return false;
  }

  for(uint8_t i = 0; i < nameLength; i++) {
    uint8_t character = static_cast<uint8_t>(entry.name[i]);
    if(character < 0x20 || character > 0x7E) {
      return false;
    }
  }

  return true;
}

bool InfraredManager::loadFromPreferences() {
  IR_COMMAND_ENTRY loadedCommands[IR_COMMAND_MAX] = {};
  Preferences preferences;

  auto loadDefaults = [this]() {
    // ir_GhostInTrap:  Defines raw timings as captured from the PKE device at full power.
    static const uint16_t ir_GhostInTrap[] = {
      1770, 1200, 600, 600, 600, 600, 580, 1200, 600, 600,
      580, 1200, 600, 1200, 580, 600, 580, 1200, 600
    };

    static const char* const defaultNames[] = {
      "GPStarTest", "Firing", "GhostInTrap",
      "HoloRun", "HoloStop", "HoloRepeat", "HoloPrevious",
      "HoloNext", "HoloPause", "HoloDim", "HoloBright"
    };

    // Defaults for the Holo* commands using NEC protocol.
    static const uint8_t defaultAddress[] = {0xA6, 0x0A};
    static const uint8_t defaultCommands[][2] = {
      {0x00, 0xFF}, {0x02, 0xFD}, {0x05, 0xFA}, {0x08, 0xF7},
      {0x0A, 0xF5}, {0x09, 0xF6}, {0x10, 0xEF}, {0x12, 0xED}
    };

    // Clear every command slot for the in-memory array.
    memset(m_Entries, 0, sizeof(m_Entries));

    // Create new command entries by filling each named entry in slot order.
    for(uint8_t index = 0; index < sizeof(defaultNames) / sizeof(defaultNames[0]); index++) {
      // Copy the name as the cleared buffer already has a null terminator after it.
      for(size_t characterIndex = 0; defaultNames[index][characterIndex] != '\0'; characterIndex++) {
        m_Entries[index].name[characterIndex] = defaultNames[index][characterIndex];
      }

      switch(index) {
        case IR_GPSTAR_TEST:
          // GPStarTest
          m_Entries[index].protocol = IR_NEC_RAW;
          m_Entries[index].protection = IR_READ_ONLY;
        break;

        case IR_DEVICE_FIRING:
          // Firing
          m_Entries[index].protocol = IR_NEC_RAW;
          m_Entries[index].protection = IR_DYNAMIC;
        break;

        case IR_GHOST_IN_TRAP:
          // GhostInTrap
          m_Entries[index].protocol = IR_RAW_TIMINGS;
          m_Entries[index].protection = IR_READ_ONLY;
          memcpy(m_Entries[index].rawData, ir_GhostInTrap, sizeof(ir_GhostInTrap));
          m_Entries[index].rawDataLength = sizeof(ir_GhostInTrap) / sizeof(ir_GhostInTrap[0]);
        break;

        default:
          // Treat all other entries as user-defined commands which can be changed/removed.
          m_Entries[index].protocol = IR_NEC;
          m_Entries[index].protection = IR_READ_WRITE;

          // Store the two address bytes and two command bytes for this slot.
          memcpy(m_Entries[index].address, defaultAddress, sizeof(defaultAddress));
          uint8_t commandIndex = index - IR_GHOST_IN_TRAP - 1; // Adjust index for the custom command array.
          memcpy(m_Entries[index].command, defaultCommands[commandIndex], sizeof(defaultCommands[commandIndex]));
        break;
      }
    }

    return true;
  };

  // Open read-write so first use can create the namespace.
  if(!preferences.begin("ir_custom", false)) {
    return loadDefaults();
  }

  size_t storedSize = preferences.getBytesLength("commands");
  if(storedSize == 0) {
    preferences.end();
    return loadDefaults();
  }
  if(storedSize != sizeof(loadedCommands)) {
    preferences.end();
    return false;
  }

  size_t bytesRead = preferences.getBytes("commands", loadedCommands, sizeof(loadedCommands));
  preferences.end();
  if(bytesRead != sizeof(loadedCommands)) {
    return false;
  }

  for(uint8_t i = 0; i < IR_COMMAND_MAX; i++) {
    if(loadedCommands[i].name[0] != '\0' && !isValidCommand(loadedCommands[i])) {
      return false;
    }
  }

  memcpy(m_Entries, loadedCommands, sizeof(m_Entries));
  return true;
}

bool InfraredManager::saveAllCommands(const IR_COMMAND_ENTRY* entries) {
  if(entries == nullptr) {
    return false;
  }

  Preferences preferences;
  if(!preferences.begin("ir_custom", false)) {
    return false;
  }

  size_t bytesWritten = preferences.putBytes("commands", entries, sizeof(m_Entries));
  preferences.end();
  return bytesWritten == sizeof(m_Entries);
}

bool InfraredManager::getCommandEntry(uint8_t index, IR_COMMAND_ENTRY& entry) const {
  if(!m_EntriesAvailable || index >= IR_COMMAND_MAX || m_Entries[index].name[0] == '\0') {
    return false;
  }

  entry = m_Entries[index];
  return true;
}

bool InfraredManager::setCommandEntry(uint8_t index, const IR_COMMAND_ENTRY& entry) {
  if(!m_EntriesAvailable ||
     index >= IR_COMMAND_MAX ||
     m_Entries[index].protection != IR_READ_WRITE ||
     !isValidCommand(entry)) {
    return false;
  }

  IR_COMMAND_ENTRY candidate[IR_COMMAND_MAX];
  memcpy(candidate, m_Entries, sizeof(candidate));
  candidate[index] = entry;
  candidate[index].protection = m_Entries[index].protection;

  if(!saveAllCommands(candidate)) {
    return false;
  }

  memcpy(m_Entries, candidate, sizeof(m_Entries));
  return true;
}

bool InfraredManager::clearCommandEntry(uint8_t index) {
  if(!m_EntriesAvailable ||
     index >= IR_COMMAND_MAX ||
     m_Entries[index].protection != IR_READ_WRITE) {
    return false;
  }

  IR_COMMAND_ENTRY candidate[IR_COMMAND_MAX];
  memcpy(candidate, m_Entries, sizeof(candidate));
  IR_COMMAND_ENTRY emptyEntry = {};
  candidate[index] = emptyEntry;

  if(!saveAllCommands(candidate)) {
    return false;
  }

  memcpy(m_Entries, candidate, sizeof(m_Entries));
  return true;
}
#endif // ESP32

#ifndef GPSTAR_IR_TX_ONLY
void InfraredManager::startRXTimer(uint16_t delay) {
  rx_timer.start(delay);
}

void InfraredManager::restartRXTimer() {
  rx_timer.repeat();
}

void InfraredManager::stopRXTimer() {
  rx_timer.stop();
}

bool InfraredManager::isRXTimerExpired() {
  return rx_timer.justFinished();
}

bool InfraredManager::decodeData() {
  return IrReceiver.decode();
}

void InfraredManager::resumeData() {
  IrReceiver.resume();
}

bool InfraredManager::dataIsNEC() {
  return IrReceiver.decodedIRData.protocol == NEC;
}

uint8_t InfraredManager::dataPreamble() {
  uint16_t address = IrReceiver.decodedIRData.address;
  return ((address >> 14) & 0x03);
}

uint8_t InfraredManager::dataDeviceType() {
  uint16_t address = IrReceiver.decodedIRData.address;
  return ((address >> 12) & 0x03);
}

uint16_t InfraredManager::dataDeviceID() {
  uint16_t address = IrReceiver.decodedIRData.address;
  return (address & 0x0FFF);
}

uint8_t InfraredManager::dataCommand() {
  return IrReceiver.decodedIRData.command;
}
#endif

// Construct 16-bit NEC address from device type and device ID
// Format: [2-bit preamble][2-bit device type][12-bit device ID]
uint16_t InfraredManager::buildIRAddress(uint8_t deviceType, uint16_t deviceId) {
  // Ensure deviceType is only 2 bits (0-3)
  deviceType &= 0x03;

  // Ensure deviceId is only 12 bits (0-4095)
  deviceId &= 0x0FFF;

  // Construct address: preamble(2) + deviceType(2) + deviceId(12)
  return (IR_GPSTAR_PREAMBLE << 14) | (deviceType << 12) | deviceId;
}

#ifdef ESP32
// Send an infrared command using the device's configured type and ID.
// Returns a JSON document with details about the command sent and its status.
JsonDocument InfraredManager::sendCommand(uint8_t commandIndex, uint8_t streamMode, uint8_t powerLevel, uint8_t streamInfo) {
  IR_COMMAND_ENTRY commandEntry = {};
  uint16_t necAddress;
  uint8_t necCommand;
  JsonDocument jsonResult;

  if(!m_EntriesAvailable) {
    // No commands available, fail immediately with a proper response.
    jsonResult["index"] = commandIndex;
    jsonResult["name"] = "Unknown";
    jsonResult["status"] = "failure";
    jsonResult["error"] = "Custom IR command storage is unavailable";
    return jsonResult;
  }
  else if(commandIndex >= IR_COMMAND_MAX || !getCommandEntry(commandIndex, commandEntry)) {
    jsonResult["index"] = commandIndex;
    jsonResult["name"] = "Unknown";
    jsonResult["status"] = "failure";
    jsonResult["error"] = "Invalid or unconfigured IR command index";
    return jsonResult;
  }

  if(isValidCommand(commandEntry)) {
    if(strcmp(commandEntry.name, "GhostInTrap") == 0) {
      // Send the standard Ghost Trap (PKE) IR signal.
      jsonResult["index"] = commandIndex;
      jsonResult["name"] = commandEntry.name;
      jsonResult["description"] = "Ghost Trap raw IR signal (PKE format)";

      if(commandEntry.rawDataLength == 0 ||
         commandEntry.rawDataLength > IR_COMMAND_RAW_MAX) {
        jsonResult["status"] = "failure";
        jsonResult["error"] = "Invalid raw timing length";
        return jsonResult;
      }

      jsonResult["status"] = "success";
      IrSender.sendRaw(commandEntry.rawData, commandEntry.rawDataLength, CARRIER_KHZ);
    }
    else if(strcmp(commandEntry.name, "GPStarTest") == 0) {
      // Send a special command to test the NEC-based IR signal.
      necAddress = buildIRAddress(m_deviceType, m_deviceID);
      necCommand = IR_COMMAND_TEST; // Command 0xFF (255) = Testing

      jsonResult["index"] = commandIndex;
      jsonResult["name"] = commandEntry.name;
      jsonResult["description"] = "GPStar NEC-based IR signal test command";
      jsonResult["preamble"] = "0x" + String(IR_GPSTAR_PREAMBLE, HEX);
      jsonResult["deviceType"] = "0x" + String(m_deviceType, HEX);
      jsonResult["deviceID"] = "0x" + String(m_deviceID, HEX);
      jsonResult["address"] = "0x" + String(necAddress, HEX);
      jsonResult["command"] = "0x" + String(necCommand, HEX);

      // Show what bytes should be transmitted in standard NEC.
      uint8_t addrLSB = necAddress & 0xFF;
      uint8_t addrMSB = (necAddress >> 8) & 0xFF;
      jsonResult["expectedNEC"] = String(addrLSB, HEX) + " " + String(addrMSB, HEX) + " "
                                + String(necCommand, HEX) + " " + String((uint8_t)~necCommand, HEX);

      // Compute what raw data the library will generate.
      uint32_t rawData = IrSender.computeNECRawDataAndChecksum(necAddress, necCommand);
      jsonResult["rawData"] = "0x" + String(rawData, HEX);

      // Break down the raw data into bytes.
      uint8_t byte0 = rawData & 0xFF;
      uint8_t byte1 = (rawData >> 8) & 0xFF;
      uint8_t byte2 = (rawData >> 16) & 0xFF;
      uint8_t byte3 = (rawData >> 24) & 0xFF;
      jsonResult["rawBytes"] = String(byte0, HEX) + " "
                              + String(byte1, HEX) + " "
                              + String(byte2, HEX) + " "
                              + String(byte3, HEX);
      jsonResult["status"] = "success";

      // Send as raw NEC data (without repeats) as we constructed the data exactly as we want it.
      IrSender.sendNECRaw(rawData, 0);
    }
    else if(strcmp(commandEntry.name, "Firing") == 0) {
      // Send firing command with both stream type and power level embedded
      necAddress = buildIRAddress(m_deviceType, m_deviceID);

      // Calculate command based on stream type and power level
      uint8_t baseCommand = IR_COMMAND_PROTON; // Initialize to Proton stream.
      switch(streamMode) {
        case IR_STREAM_PROTON:
        default:
          if(streamInfo == IR_CMD_STREAM) {
            baseCommand = IR_COMMAND_PROTON; // Proton: 0x20-0x24 (power levels 1-5)
          }
          else if(streamInfo == IR_CMD_BLAST) {
            baseCommand = IR_COMMAND_BOSON_DART; // Boson Dart: 0x25-0x29 (power levels 1-5)
          }
          else if(streamInfo == IR_CMD_CTS) {
            baseCommand = IR_COMMAND_CTS; // Crossing the Streams: 0x60-0x64 (power levels 1-5)
          }
        break;
        case IR_STREAM_SLIME:
          if(streamInfo == IR_CMD_STREAM) {
            baseCommand = IR_COMMAND_SLIME; // Slime: 0x30-0x34 (power levels 1-5)
          }
          else if(streamInfo == IR_CMD_BLAST) {
            baseCommand = IR_COMMAND_SLIME_BLAST; // Slime Blast: 0x35-0x39 (power levels 1-5)
          }
        break;
        case IR_STREAM_STASIS:
          if(streamInfo == IR_CMD_STREAM) {
            baseCommand = IR_COMMAND_STASIS; // Stasis: 0x40-0x44 (power levels 1-5)
          }
          else if(streamInfo == IR_CMD_BLAST) {
            baseCommand = IR_COMMAND_SHOCK_BLAST; // Shock Blast: 0x45-0x49 (power levels 1-5)
          }
        break;
        case IR_STREAM_MESON:
          if(streamInfo == IR_CMD_STREAM) {
            baseCommand = IR_COMMAND_MESON; // Meson: 0x50-0x54 (power levels 1-5)
          }
          else if(streamInfo == IR_CMD_BLAST) {
            baseCommand = IR_COMMAND_OVERLOAD_PULSE; // Overload Pulse: 0x55-0x59 (power levels 1-5)
          }
        break;
      }
      necCommand = baseCommand + (powerLevel - 1); // Add power level offset

      jsonResult["index"] = commandIndex;
      jsonResult["name"] = commandEntry.name;
      jsonResult["description"] = "GPStar NEC-based IR signal firing command with stream type and power level";
      jsonResult["streamMode"] = streamMode;
      jsonResult["powerLevel"] = powerLevel;
      jsonResult["streamInfo"] = streamInfo;
      jsonResult["preamble"] = "0x" + String(IR_GPSTAR_PREAMBLE, HEX);
      jsonResult["deviceType"] = "0x" + String(m_deviceType, HEX);
      jsonResult["deviceID"] = "0x" + String(m_deviceID, HEX);
      jsonResult["address"] = "0x" + String(necAddress, HEX);
      jsonResult["command"] = "0x" + String(necCommand, HEX);

      // Show what bytes should be transmitted in standard NEC.
      uint8_t addrLSB = necAddress & 0xFF;
      uint8_t addrMSB = (necAddress >> 8) & 0xFF;
      jsonResult["expectedNEC"] = String(addrLSB, HEX) + " " + String(addrMSB, HEX) + " "
                                + String(necCommand, HEX) + " " + String((uint8_t)~necCommand, HEX);

      // Compute what raw data the library will generate.
      uint32_t rawData = IrSender.computeNECRawDataAndChecksum(necAddress, necCommand);
      jsonResult["rawData"] = "0x" + String(rawData, HEX);

      // Break down the raw data into bytes.
      uint8_t byte0 = rawData & 0xFF;
      uint8_t byte1 = (rawData >> 8) & 0xFF;
      uint8_t byte2 = (rawData >> 16) & 0xFF;
      uint8_t byte3 = (rawData >> 24) & 0xFF;
      jsonResult["rawBytes"] = String(byte0, HEX) + " "
                             + String(byte1, HEX) + " "
                             + String(byte2, HEX) + " "
                             + String(byte3, HEX);
      jsonResult["status"] = "success";

      // Send as raw NEC data (without repeats) as we constructed the data exactly as we want it.
      IrSender.sendNECRaw(rawData, 0);
    }
    else {
      // Fall-through for all other signals which do not match a specific name.
      jsonResult["index"] = commandIndex;
      jsonResult["name"] = commandEntry.name;

      switch(commandEntry.protocol) {
        case IR_NEC:
          // Rebuild the 16-bit NEC address from the UI's low-byte-first storage.
          necAddress = static_cast<uint16_t>(commandEntry.address[0]) |
                       (static_cast<uint16_t>(commandEntry.address[1]) << 8);
          // sendNEC generates the inverse byte from the selected command byte.
          necCommand = commandEntry.command[0];

          jsonResult["address"] = "0x" + String(necAddress, HEX);
          jsonResult["command"] = "0x" + String(necCommand, HEX);
          jsonResult["status"] = "success";
          IrSender.sendNEC(necAddress, necCommand, 0);
        break;

        default:
          // Unsupported protocol encountered.
          jsonResult["status"] = "failure";
          jsonResult["error"] = "Command protocol is not supported by this handler";
        break;
      }
    }
  }
  else {
    jsonResult["index"] = commandIndex;
    jsonResult["name"] = "Unknown";
    jsonResult["status"] = "failure";
    jsonResult["error"] = "Invalid IR command entry requested";
  }

  return jsonResult;
}
#endif // ESP32
#endif // (defined(ESP32) || defined(__AVR_ATtiny1616__))
