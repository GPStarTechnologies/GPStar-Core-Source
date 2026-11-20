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
 * Static IR Data Definitions
 */

// Defines an IR command as captured from the PKE device at full power.
// This does not follow a standard protocol, so we use raw timings.
static const uint16_t ir_GhostInTrap[] = {
  1770, 1200, 600, 600, 600, 600, 580, 1200, 600, 600,
  580, 1200, 600, 1200, 580, 600, 580, 1200, 600
};

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
  m_customCommandsAvailable = loadCustomCommands();
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
bool InfraredManager::isValidCustomCommand(const IR_CUSTOM_COMMAND_ENTRY& entry) const {
  uint8_t nameLength = 0;
  while(nameLength <= IR_CUSTOM_COMMAND_NAME_MAX && entry.name[nameLength] != '\0') {
    nameLength++;
  }

  if(nameLength == 0 || nameLength > IR_CUSTOM_COMMAND_NAME_MAX) {
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

bool InfraredManager::loadCustomCommands() {
  IR_CUSTOM_COMMAND_ENTRY loadedCommands[IR_CUSTOM_COMMAND_MAX] = {};
  Preferences preferences;

  auto loadDefaults = [this]() {
    static const char* const defaultNames[] = {
      "Run", "Stop", "Repeat", "Previous", "Next", "Pause", "BrightnessLower", "BrightnessHigher"
    };
    static const uint8_t defaultAddress[] = {0xA6, 0x0A};
    static const uint8_t defaultCommands[][2] = {
      {0x00, 0xFF}, {0x02, 0xFD}, {0x05, 0xFA}, {0x08, 0xF7},
      {0x0A, 0xF5}, {0x09, 0xF6}, {0x10, 0xEF}, {0x12, 0xED}
    };

    memset(m_customCommands, 0, sizeof(m_customCommands));
    for(uint8_t index = 0; index < sizeof(defaultNames) / sizeof(defaultNames[0]); index++) {
      for(size_t characterIndex = 0; defaultNames[index][characterIndex] != '\0'; characterIndex++) {
        m_customCommands[index].name[characterIndex] = defaultNames[index][characterIndex];
      }
      memcpy(m_customCommands[index].address, defaultAddress, sizeof(defaultAddress));
      memcpy(m_customCommands[index].command, defaultCommands[index], sizeof(defaultCommands[index]));
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

  for(uint8_t i = 0; i < IR_CUSTOM_COMMAND_MAX; i++) {
    if(loadedCommands[i].name[0] != '\0' && !isValidCustomCommand(loadedCommands[i])) {
      return false;
    }
  }

  memcpy(m_customCommands, loadedCommands, sizeof(m_customCommands));
  return true;
}

bool InfraredManager::saveCustomCommands(const IR_CUSTOM_COMMAND_ENTRY* entries) {
  if(entries == nullptr) {
    return false;
  }

  Preferences preferences;
  if(!preferences.begin("ir_custom", false)) {
    return false;
  }

  size_t bytesWritten = preferences.putBytes("commands", entries, sizeof(m_customCommands));
  preferences.end();
  return bytesWritten == sizeof(m_customCommands);
}

bool InfraredManager::getCustomCommand(uint8_t index, IR_CUSTOM_COMMAND_ENTRY& entry) const {
  if(!m_customCommandsAvailable || index >= IR_CUSTOM_COMMAND_MAX || m_customCommands[index].name[0] == '\0') {
    return false;
  }

  entry = m_customCommands[index];
  return true;
}

bool InfraredManager::setCustomCommand(uint8_t index, const IR_CUSTOM_COMMAND_ENTRY& entry) {
  if(!m_customCommandsAvailable || index >= IR_CUSTOM_COMMAND_MAX || !isValidCustomCommand(entry)) {
    return false;
  }

  IR_CUSTOM_COMMAND_ENTRY candidate[IR_CUSTOM_COMMAND_MAX];
  memcpy(candidate, m_customCommands, sizeof(candidate));
  candidate[index] = entry;

  if(!saveCustomCommands(candidate)) {
    return false;
  }

  memcpy(m_customCommands, candidate, sizeof(m_customCommands));
  return true;
}

bool InfraredManager::clearCustomCommand(uint8_t index) {
  if(!m_customCommandsAvailable || index >= IR_CUSTOM_COMMAND_MAX) {
    return false;
  }

  IR_CUSTOM_COMMAND_ENTRY candidate[IR_CUSTOM_COMMAND_MAX];
  memcpy(candidate, m_customCommands, sizeof(candidate));
  IR_CUSTOM_COMMAND_ENTRY emptyEntry = {};
  candidate[index] = emptyEntry;

  if(!saveCustomCommands(candidate)) {
    return false;
  }

  memcpy(m_customCommands, candidate, sizeof(m_customCommands));
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
JsonDocument InfraredManager::sendCommand(IR_COMMAND_TYPE commandType, uint8_t streamMode, uint8_t powerLevel, uint8_t streamInfo, uint8_t customCommandIndex) {
  JsonDocument jsonResult;

  if(commandType == IR_CMD_GHOST_TRAP) {
    // Send the standard Ghost Trap (PKE) IR signal.
    jsonResult["command"] = "ghostintrap";
    jsonResult["description"] = "Ghost Trap raw IR signal (PKE format)";
    jsonResult["status"] = "success";
    IrSender.sendRaw(ir_GhostInTrap, sizeof(ir_GhostInTrap) / sizeof(ir_GhostInTrap[0]), CARRIER_KHZ);
  }
  else if(commandType == IR_CMD_CUSTOM) {
    IR_CUSTOM_COMMAND_ENTRY entry = {};

    if(!m_customCommandsAvailable) {
      jsonResult["command"] = "custom";
      jsonResult["status"] = "failure";
      jsonResult["error"] = "Custom IR command storage is unavailable";
    }
    else if(customCommandIndex >= IR_CUSTOM_COMMAND_MAX || !getCustomCommand(customCommandIndex, entry)) {
      jsonResult["command"] = "custom";
      jsonResult["status"] = "failure";
      jsonResult["error"] = "Invalid or unconfigured custom IR command index";
    }
    else {
      // Rebuild the 16-bit NEC address from the UI's low-byte-first storage.
      uint16_t address = static_cast<uint16_t>(entry.address[0]) |
                         (static_cast<uint16_t>(entry.address[1]) << 8);
      // sendNEC generates the inverse byte from the selected command byte.
      uint8_t command = entry.command[0];

      jsonResult["command"] = "custom";
      jsonResult["customCommandIndex"] = customCommandIndex;
      jsonResult["name"] = entry.name;
      jsonResult["address"] = "0x" + String(address, HEX);
      jsonResult["customCommand"] = "0x" + String(command, HEX);
      jsonResult["status"] = "success";
      IrSender.sendNEC(address, command, 0);
    }
  }
  else if(commandType == IR_CMD_TEST) {
    // Send a special command to test the NEC-based IR signal.
    uint16_t myAddress = buildIRAddress(m_deviceType, m_deviceID);
    uint8_t command = IR_COMMAND_TEST; // Command 0xFF (255) = Testing

    jsonResult["command"] = "gpstartest";
    jsonResult["description"] = "GPStar NEC-based IR signal test command";
    jsonResult["preamble"] = "0x" + String(IR_GPSTAR_PREAMBLE, HEX);
    jsonResult["deviceType"] = "0x" + String(m_deviceType, HEX);
    jsonResult["deviceID"] = "0x" + String(m_deviceID, HEX);
    jsonResult["address"] = "0x" + String(myAddress, HEX);
    jsonResult["command"] = "0x" + String(command, HEX);

    // Show what bytes should be transmitted in standard NEC.
    uint8_t addrLSB = myAddress & 0xFF;
    uint8_t addrMSB = (myAddress >> 8) & 0xFF;
    jsonResult["expectedNEC"] = String(addrLSB, HEX) + " " + String(addrMSB, HEX) + " " + String(command, HEX) + " " + String((uint8_t)~command, HEX);

    // Compute what raw data the library will generate.
    uint32_t rawData = IrSender.computeNECRawDataAndChecksum(myAddress, command);
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
  else if(commandType == IR_CMD_FIRING) {
    // Send firing command with both stream type and power level embedded
    uint16_t myAddress = buildIRAddress(m_deviceType, m_deviceID);

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
    uint8_t command = baseCommand + (powerLevel - 1); // Add power level offset

    jsonResult["command"] = "firing";
    jsonResult["description"] = "GPStar NEC-based IR signal firing command with stream type and power level";
    jsonResult["streamMode"] = streamMode;
    jsonResult["powerLevel"] = powerLevel;
    jsonResult["streamInfo"] = streamInfo;
    jsonResult["preamble"] = "0x" + String(IR_GPSTAR_PREAMBLE, HEX);
    jsonResult["deviceType"] = "0x" + String(m_deviceType, HEX);
    jsonResult["deviceID"] = "0x" + String(m_deviceID, HEX);
    jsonResult["address"] = "0x" + String(myAddress, HEX);
    jsonResult["command"] = "0x" + String(command, HEX);

    // Show what bytes should be transmitted in standard NEC.
    uint8_t addrLSB = myAddress & 0xFF;
    uint8_t addrMSB = (myAddress >> 8) & 0xFF;
    jsonResult["expectedNEC"] = String(addrLSB, HEX) + " " + String(addrMSB, HEX) + " " + String(command, HEX) + " " + String((uint8_t)~command, HEX);

    // Compute what raw data the library will generate.
    uint32_t rawData = IrSender.computeNECRawDataAndChecksum(myAddress, command);
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
    jsonResult["command"] = "unknown";
    jsonResult["status"] = "failure";
    jsonResult["error"] = "Unknown IR Command Type";
  }

  return jsonResult;
}

// Convert string command type to IR_COMMAND_TYPE enum
bool InfraredManager::stringToCommandType(const String& commandStr, IR_COMMAND_TYPE& commandType) {
  // Check strings directly using equals() as a simple switch statement can only work on integers.
  if(commandStr.equals("ghostintrap")) {
    commandType = IR_CMD_GHOST_TRAP;
    return true;
  }
  else if(commandStr.equals("firing")) {
    commandType = IR_CMD_FIRING;
    return true;
  }
  else if(commandStr.equals("gpstartest")) {
    commandType = IR_CMD_TEST;
    return true;
  }
  else if(commandStr.equals("custom")) {
    commandType = IR_CMD_CUSTOM;
    return true;
  }

  return false; // Default fall-through.
}
#endif // ESP32
#endif // (defined(ESP32) || defined(__AVR_ATtiny1616__))
