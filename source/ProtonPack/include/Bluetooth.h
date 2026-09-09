/**
 *   GPStar Proton Pack - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2026 Michael Rajotte <contact@gpstartechnologies.com>
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

// For BluetoothLE (BLE)
#include <NimBLEDevice.h>

// Declare external reference to WirelessManager pointer (allocated in main.cpp after NVS init)
extern WirelessManager* wirelessMgr;

/*
 * BLE Security Configuration
 */
#define BLE_PAIRING_PASSKEY 777888  // Passkey for Wand-Pack pairing (must match on both devices)

/*
 * Bluetooth LE Server State Variables (Pack as GATT Server/Peripheral)
 */
NimBLEServer *g_pBLEServer = nullptr;
NimBLEService *g_pGPStarService = nullptr;
NimBLECharacteristic *g_pCommandCharacteristic = nullptr;
NimBLECharacteristic *g_pStatusCharacteristic = nullptr;
bool b_ble_enabled = true; // Master enable/disable flag; set to false to turn off all BLE scanning and connection
bool b_ble_initialized = false; // NimBLE device initialized, service/characteristics created, advertising started
bool b_ble_connected = false; // Indicates when the Wand has successfully connected and bonded with this Pack

// BLE command queue (store incoming BLE commands for processing)
uint8_t g_ble_rx_buffer[32] = {0};  // Buffer for received BLE bytes
size_t g_ble_rx_length = 0;         // Number of bytes in buffer
bool b_ble_rx_ready = false;        // Flag: data ready to process

// Global callback objects (must persist for lifetime of BLE server)
NimBLEServerCallbacks *g_pPackServerCallbacks = nullptr;
NimBLECharacteristicCallbacks *g_pPackCharacteristicCallbacks = nullptr;

/*
 * BLE UUIDs (custom 128-bit UUIDs for GPStar peer-to-peer protocol)
 *
 * UUID Stability and Hardcoding Strategy:
 * ========================================
 * These UUIDs are intentionally HARDCODED for Phase 1 because:
 * 1. STABILITY: Allows Wand to reliably discover any Pack (same UUID always)
 * 2. SIMPLICITY: No service discovery negotiation needed; Wand knows exactly what to look for
 * 3. SINGLE-PAIR MODEL: Current requirement is one Pack per Wand (not concurrent multi-pack)
 * 4. EFFICIENCY: Reduces BLE scanning complexity; Wand filters by known service UUID
 *
 * If we made these DYNAMIC in the future (Phase 2+), we would:
 * - Add a UUID registration mechanism (e.g., store in EEPROM)
 * - Allow Pack to advertise its UUID via WiFi mDNS (discovery pre-step)
 * - Require Wand to scan all possible UUIDs or use a "discovery mode"
 * - Trade: Complexity gain vs. benefit loss (only supports multi-pack scenarios)
 *
 * Current format: Bluetooth SIG reserved namespace with GPStar-specific suffixes
 * 0000ffe0-... = Primary service (chosen from Vendor-Defined space)
 * 0000ffe1-... = Command characteristic (Wand → Pack)
 * 0000ffe2-... = Status characteristic (Pack → Wand)
 *
 * To change these UUIDs:
 * - Update BOTH Pack/Wand Bluetooth.h files simultaneously (must match!)
 * - Use online UUID generator or reserve from Bluetooth SIG if commercializing
 * - Current values are placeholders; safe for private/hobbyist use
 */
const char* GPSTAR_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb";
const char* GPSTAR_COMMAND_CHAR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb";
const char* GPSTAR_STATUS_CHAR_UUID = "0000ffe2-0000-1000-8000-00805f9b34fb";

/*
 * Bluetooth LE Management Functions
 * 
 * OVERVIEW:
 * =========
 * The Pack is a BLE SERVER (peripheral) that advertises and accepts connections from the Wand (client/central).
 * This file defines:
 * 1. Callback classes that respond to BLE events (connection, disconnection, pairing, characteristic writes)
 * 2. Management functions to initialize BLE, advertise, and send data
 * 3. Callback handlers invoked by NimBLE when connection/pairing/data events occur
 *
 * TYPICAL FLOW:
 * =============
 * 1. startBluetooth() called from main.cpp setup() → initializes NimBLE, creates service/characteristics, starts advertising
 * 2. NimBLE advertises with Pack name "GPStar-Pack-XXXX" and service UUID 0000ffe0-...
 * 3. Wand scans for this service UUID, finds Pack, initiates connection
 * 4. Server callbacks (GPStarPackServerCallbacks) fire → onConnect, onAuthenticationComplete
 * 5. LE Secure Connections pairing occurs automatically
 * 6. After successful pairing → b_ble_connected = true
 * 7. Wand sends commands via command characteristic writes
 * 8. onWrite callback fires → GPStarPackCharacteristicCallbacks::onWrite receives command bytes
 * 9. Pack sends status via status characteristic notifications
 * 10. bleSendData() function notifies Wand with serialized status data
 * 11. If Wand disconnects → onDisconnect() callback fires → b_ble_connected = false → continues advertising
 */

// Server-level callback class to handle connection/disconnection/authentication events
// INVOKED BY: NimBLE framework when connection state changes
// NOT CALLED MANUALLY - NimBLE handles this asynchronously
class GPStarPackServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo& connInfo) override {
    // FIRED WHEN: Device successfully connects to Pack BLE server
    // ACTION: Log connection and mark ready for commands
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Pack MAC: "));
      debugln(NimBLEDevice::getAddress().toString().c_str());
      debug(F("[BLE] Wand MAC: "));
      debugln(connInfo.getAddress().toString().c_str());
      debugln(F("[BLE] WAND CONNECTED"));
    #endif
    
    // Mark ready for commands immediately after connection
    b_ble_connected = true;
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
    // FIRED WHEN: Device disconnects from Pack BLE server (device powered off, out of range, or user disconnect)
    // ACTION: Clear connection flag, log reason code, restart advertising for next connection
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Device disconnected (reason: "));
      debug(reason);
      debugln(F(")"));
    #endif
    b_ble_connected = false;
    
    // Ensure advertising restarts for next connection attempt
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    if(pAdvertising && !pAdvertising->isAdvertising()) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] Restarting BLE advertising..."));
      #endif
      pAdvertising->start();
    }
  }

  void onAuthenticationComplete(NimBLEConnInfo &connInfo) override {
    // FIRED WHEN: Passkey pairing successfully completes with Wand
    // ACTION: Log pairing success, connection now bonded and encrypted
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] *** PAIRING COMPLETE with Wand ***"));
      debugln(F("  Connection is now bonded and encrypted"));
    #endif
  }
};

// Characteristic-level callback class to handle GATT write events from the Wand
// INVOKED BY: NimBLE notification task when Wand writes to command characteristic
// NOT CALLED MANUALLY - NimBLE handles this asynchronously when Wand writes data
class GPStarPackCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override {
    // FIRED WHEN: Wand sends data via BLE characteristic write
    // PARAM pCharacteristic: The command characteristic being written to
    // PARAM connInfo: Connection info (MAC address, connection handle, etc)
    // ACTION: Process command or identity data
    
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      uint8_t packetType = (uint8_t)rxValue[0];
      
      // Check for identity packet (validates this is a Wand)
      if(packetType == PACKET_IDENTITY) {
        debugln(F("[BLE] !!! IDENTITY PACKET RECEIVED !!!"));
        debug(F("[BLE] Packet length: "));
        debugln(rxValue.length());
        
        if(rxValue.length() >= 4) {
          uint8_t deviceType = (uint8_t)rxValue[1];
          uint8_t deviceIDHi = (uint8_t)rxValue[2];
          uint8_t deviceIDLo = (uint8_t)rxValue[3];
          uint16_t deviceID = ((uint16_t)deviceIDHi << 8) | deviceIDLo;
          
          debug(F("[BLE] Identity packet contents: [0x"));
          debug(packetType, HEX);
          debug(F(", 0x"));
          debug(deviceType, HEX);
          debug(F(", 0x"));
          debug(deviceIDHi, HEX);
          debug(F(", 0x"));
          debug(deviceIDLo, HEX);
          debugln(F("]"));
          
          debug(F("[BLE] Device Type: 0x"));
          debug(deviceType, HEX);
          debug(F(", Device ID: 0x"));
          debugln(deviceID, HEX);
          
          // Validate this is a Wand (IR_DEVICE_NEUTRONA_WAND = 0x0)
          if(deviceType == 0x00) {
            b_ble_connected = true;
            debugln(F("[BLE] *** WAND IDENTITY VERIFIED ***"));
            debugln(F("[BLE] BLE connection is now ACTIVE!"));
          } else {
            debugln(F("[BLE] *** IDENTITY REJECTED - Invalid device type ***"));
          }
        } else {
          debug(F("[BLE] ERROR: Identity packet too short (expected 4, got "));
          debug(rxValue.length());
          debugln(F(")"));
        }
        return;
      }
      
      // Only process other commands if we've verified this is a Wand
      if(!b_ble_connected) {
        #if defined(DEBUG_BLUETOOTH)
          debugln(F("[BLE] Command received before identity verification - ignoring"));
        #endif
        return;
      }
      
      // Queue the command for processing by checkWand() in main loop
      if(rxValue.length() <= 32) {
        debugln(F("[PACK-CALLBACK] Write callback fired"));
        memcpy(g_ble_rx_buffer, rxValue.c_str(), rxValue.length());
        g_ble_rx_length = rxValue.length();
        b_ble_rx_ready = true;
      }
    }
  }
};

// Parsed BLE packet structure
struct BLEPacket {
  uint8_t packetType;  // PACKET_COMMAND, PACKET_DATA, PACKET_WAND, PACKET_SMOKE, or PACKET_UNKNOWN
  uint8_t start;       // Frame start marker
  uint16_t cmd;        // Command ID (for PACKET_COMMAND and PACKET_DATA)
  uint16_t d1;         // Data value 1 (for PACKET_COMMAND)
  uint8_t d[3];        // Data array (for PACKET_DATA)
  uint8_t end;         // Frame end marker
  size_t length;       // Total packet length
};

// Parse raw BLE bytes into structured packet
// Returns packet with packetType = PACKET_UNKNOWN if parse fails
BLEPacket bleHandleData(const uint8_t* pData, size_t length) {
  BLEPacket packet = {PACKET_UNKNOWN, 0, 0, 0, {0, 0, 0}, 0, length};
  
  if(length < 4) {
    return packet;
  }
  
  packet.start = pData[0];
  packet.end = pData[length - 1];
  
  // Validate frame markers
  if(packet.start != A_COM_START || packet.end != A_COM_END) {
    return packet;
  }
  
  // Determine packet type based on length
  if(length == 6) {
    // PACKET_COMMAND: (s, c:2, d1:2, e)
    packet.packetType = PACKET_COMMAND;
    packet.cmd = (uint16_t)pData[1] | ((uint16_t)pData[2] << 8);
    packet.d1 = (uint16_t)pData[3] | ((uint16_t)pData[4] << 8);
  } 
  else if(length == 7) {
    // PACKET_DATA: (s, c:2, d[3], e)
    packet.packetType = PACKET_DATA;
    packet.cmd = (uint16_t)pData[1] | ((uint16_t)pData[2] << 8);
    packet.d[0] = pData[3];
    packet.d[1] = pData[4];
    packet.d[2] = pData[5];
  }
  else if(length > 7) {
    // Could be PACKET_WAND or PACKET_SMOKE (preference structs)
    // Just mark as data, don't try to validate size
    if(length > 10) {
      packet.packetType = PACKET_WAND;  // Assume WAND if larger
    }
  }
  
  return packet;
}

// Process incoming BLE notifications from Wand
// Called from main loop to parse and handle queued BLE command bytes
void processBLENotification() {
  if(!b_ble_rx_ready || g_ble_rx_length == 0) {
    return;  // No notification queued
  }
  
  debugln(F("[PACK-PROCESS] Processing BLE notification"));
  
  // Clear the ready flag
  b_ble_rx_ready = false;
  
  // Parse the queued notification
  BLEPacket packet = bleHandleData(g_ble_rx_buffer, g_ble_rx_length);
  
  #if defined(DEBUG_BLUETOOTH)
    // Parsed packet structure
    debug(F("[BLE-RX] Parsed: type="));
    switch(packet.packetType) {
      case PACKET_COMMAND: debug(F("COMMAND(1)")); break;
      case PACKET_DATA: debug(F("DATA(2)")); break;
      case PACKET_WAND: debug(F("WAND(3)")); break;
      case PACKET_SMOKE: debug(F("SMOKE(4)")); break;
      default: debug(F("UNKNOWN(0)")); break;
    }
    debug(F(" | cmd="));
    debug(packet.cmd);
    debug(F(" d1="));
    debug(packet.d1);
    debug(F(" | start="));
    debug(packet.start);
    debug(F(" end="));
    debugln(packet.end);
  #endif

  // Deserialize BLE buffer into same global structs used by UART
  if(packet.packetType > 0) {
    switch(packet.packetType) {
      case PACKET_COMMAND:
        if(packet.cmd > 0) {
          recvCmdW.s = packet.start;
          recvCmdW.c = packet.cmd;
          recvCmdW.d1 = packet.d1;
          recvCmdW.e = packet.end;
        }
        break;
        
      case PACKET_DATA:
        if(packet.cmd > 0) {
          recvDataW.s = packet.start;
          recvDataW.c = packet.cmd;
          recvDataW.d[0] = packet.d[0];
          recvDataW.d[1] = packet.d[1];
          recvDataW.d[2] = packet.d[2];
          recvDataW.e = packet.end;
        }
        break;
        
      case PACKET_WAND:
        memcpy(&wandConfig, g_ble_rx_buffer, g_ble_rx_length);
        break;
        
      case PACKET_SMOKE:
        memcpy(&smokeConfig, g_ble_rx_buffer, g_ble_rx_length);
        break;
    }
    
    // Ensure BLE connection state is synchronized (UART does this via serial handshake)
    // BLE bypass: directly update state to allow handlers to process commands
    if(WAND_CONN_STATE == WAND_DISCONNECTED || WAND_CONN_STATE == WAND_MISMATCH) {
      WAND_CONN_STATE = WAND_CONNECTED;
    }
    
    // Route to central packet handler (same dispatcher as UART)
    handleWandPacket(packet.packetType);
  }
  
  // Clear the buffer
  g_ble_rx_length = 0;
}

bool startBluetooth() {
  if(!b_ble_enabled) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] BLE disabled, startup skipped"));
    #endif
    return false;
  }

  if(b_ble_initialized) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Already initialized"));
    #endif
    return true;
  }

  #if defined(DEBUG_BLUETOOTH)
    debugln();
    debugln(F("========== BLE Server (Pack) Initialization =========="));
  #endif

  try {
    // Initialize NimBLE with device name based on Pack ID
    String deviceName = "GPStar-Pack-" + String(wirelessMgr->getDeviceID(), HEX);
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Initializing NimBLE device as: "));
      debugln(deviceName);
    #endif
    
    NimBLEDevice::init(deviceName.c_str());

    // Configure automatic pairing with LE Secure Connections
    // This enables automatic bonding when a Wand connects
    NimBLEDevice::setSecurityAuth(true, true, false);  // bonding, MITM, passkey pairing (not SC)
    NimBLEDevice::setSecurityPasskey(BLE_PAIRING_PASSKEY); // Set passkey (must match Wand)
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // Display passkey only
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Security: LE Secure Connections with bonding enabled (MITM disabled for testing)"));
    #endif

    // Create the BLE Server (this device is the server/peripheral)
    g_pBLEServer = NimBLEDevice::createServer();
    
    if(!g_pBLEServer) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] *** ERROR: Failed to create BLE Server"));
      #endif
      return false;
    }
    
    // Set server callbacks for connection state changes
    // Create and store callback object globally (must persist for server lifetime)
    if(!g_pPackServerCallbacks) {
      g_pPackServerCallbacks = new GPStarPackServerCallbacks();
    }
    g_pBLEServer->setCallbacks(g_pPackServerCallbacks);

    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] BLE Server created successfully"));
    #endif

    // Create the GPStar service
    g_pGPStarService = g_pBLEServer->createService(GPSTAR_SERVICE_UUID);
    
    if(!g_pGPStarService) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] *** ERROR: Failed to create GPStar service"));
      #endif
      return false;
    }
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] GPStar service created (UUID: "));
      debugln(GPSTAR_SERVICE_UUID);
    #endif

    // Create command characteristic (Wand writes commands to Pack)
    g_pCommandCharacteristic = g_pGPStarService->createCharacteristic(
      GPSTAR_COMMAND_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    
    if(!g_pCommandCharacteristic) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] *** ERROR: Failed to create command characteristic"));
      #endif
      return false;
    }
    
    // Create and store callback object globally (must persist for characteristic lifetime)
    if(!g_pPackCharacteristicCallbacks) {
      g_pPackCharacteristicCallbacks = new GPStarPackCharacteristicCallbacks();
    }
    g_pCommandCharacteristic->setCallbacks(g_pPackCharacteristicCallbacks);
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Command characteristic created (Wand → Pack)"));
    #endif

    // Create status characteristic (Pack notifies Wand of state changes)
    g_pStatusCharacteristic = g_pGPStarService->createCharacteristic(
      GPSTAR_STATUS_CHAR_UUID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    
    if(!g_pStatusCharacteristic) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] *** ERROR: Failed to create status characteristic"));
      #endif
      return false;
    }

    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Status characteristic created (Pack → Wand)"));
    #endif

    // Set up advertising
    // IMPLEMENTATION NOTES:
    // - Advertising occurs in the BLE controller (FreeRTOS task on core 0)
    // - No core pinning needed; NimBLE manages its own task scheduling
    // - WiFi and BLE coexistence is automatic via Espressif's controller
    // - BLE stack tasks run independently of application tasks
    // - Device name only (no UUID in advertisement to stay within 31-byte limit)
    // - Wand discovers service via GATT discovery after connection
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(deviceName.c_str());
    pAdvertising->start();

    #if defined(DEBUG_BLUETOOTH)
      debugln();
      debugln(F("========== BLE Server (Pack) Initialization =========="));
      debug(F("[BLE] MAC: "));
      debugln(NimBLEDevice::getAddress().toString().c_str());
      debug(F("[BLE] Name: "));
      debugln(deviceName);
      debugln(F("[BLE] Waiting for Wand to connect..."));
      debugln();
    #endif

    b_ble_initialized = true;

    return true;
  }
  catch (const std::exception &e) {
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] *** EXCEPTION: "));
      debugln(e.what());
    #endif
    return false;
  }
}

// Send serialized data via BLE characteristic (receives same buffer that was sent via UART)
// The function checks BLE state and sends the same bytes via characteristic
void bleSendData(const uint8_t* pData, size_t length) {
  if(!b_ble_enabled || !b_ble_connected || !g_pStatusCharacteristic) {
    return;  // BLE not ready, function decides silently
  }
  
  g_pStatusCharacteristic->setValue((uint8_t*)pData, length);
  g_pStatusCharacteristic->notify();
  
  #if defined(DEBUG_BLUETOOTH)
    debug(F("[BLE-TX] Sent "));
    debug(length);
    debugln(F(" bytes via BLE"));
  #endif
}
