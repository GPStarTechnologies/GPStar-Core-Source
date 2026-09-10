/**
 *   GPStar Neutrona Wand - Ghostbusters Proton Pack & Neutrona Wand.
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

// BLE Message Queue Library (Phase 3 refactoring) - umbrella header
#include <BLE.h>

// Declare external reference to WirelessManager pointer (allocated in main.cpp after NVS init)
extern WirelessManager* wirelessMgr;

/*
 * BLE Security Configuration
 */
#define BLE_PAIRING_PASSKEY 777888  // Passkey for Wand-Pack pairing (must match on both devices)

/*
 * Bluetooth LE Client State Variables (Wand as GATT Client/Central)
 */
NimBLEClient *g_pBLEClient = nullptr;
NimBLERemoteService *g_pRemoteGPStarService = nullptr;
NimBLERemoteCharacteristic *g_pRemoteCommandChar = nullptr;
NimBLERemoteCharacteristic *g_pRemoteStatusChar = nullptr;
bool b_ble_enabled = true; // Master enable/disable flag; set to false to turn off all BLE scanning and connection
bool b_ble_initialized = false; // NimBLE device initialized and scan configured
bool b_ble_connected = false; // Currently connected and bonded with Pack
uint16_t i_target_pack_id = 0; // Pack device ID extracted from advertisement (reserved for future use)

// Pack discovery info (stored by scan callback, used by main thread to connect)
NimBLEAddress g_packAddress;  // MAC address of discovered Pack
bool b_pack_found = false;    // Set by scan callback, cleared by main thread after connecting

// BLE message queues (message-level queueing per BLE_TRANSPORT.md)
// Wand TX: messages from Wand to Pack (written to PackRX characteristic)
// Wand RX: messages from Pack to Wand (received via notifications)
// Note: Queues are zero-initialized by default in C; no explicit init needed before use
BLEMessageQueue g_ble_tx_queue = {};
BLEMessageQueue g_ble_rx_queue = {};

// BLE notification queue (store incoming Pack notifications for main loop processing)
uint8_t g_ble_rx_buffer[32] = {0};  // Buffer for received BLE bytes
size_t g_ble_rx_length = 0;         // Number of bytes in buffer

// Serial RX buffer (incoming BLE-sourced serial frames for fallback processing)
uint8_t g_serial_rx_buffer[32] = {0};
size_t g_serial_rx_length = 0;
bool b_serial_rx_ready = false;

// Global callback objects (must persist for lifetime of BLE client)
NimBLEClientCallbacks *g_pWandClientCallbacks = nullptr;
NimBLEScanCallbacks *g_pWandScanCallbacks = nullptr;

/*
 * BLE UUIDs (defined in shared BLE library)
 * 
 * These constants come from BLEConstants.h in the shared Bluetooth library.
 * They are hardcoded per BLE_TRANSPORT.md specification for stable Pack discovery.
 * 
 * UUID layout:
 * - BLE_SERIALDATA_SERVICE_UUID: Service advertised by Pack
 * - BLE_SERIALDATA_PACKTX_CHAR_UUID: Pack → Wand (indications)
 * - BLE_SERIALDATA_PACKRX_CHAR_UUID: Wand → Pack (writes)
 * 
 * DO NOT DUPLICATE these in application code - use the shared library constants!
 */

// Forward declaration of characteristic discovery function (defined after callbacks)
void discoverRemoteCharacteristics();

/*
 * Bluetooth LE Management Functions
 * 
 * OVERVIEW:
 * =========
 * The Wand is a BLE CLIENT (central) that discovers and connects to the Pack (server/peripheral).
 * This file defines:
 * 1. Callback classes that respond to BLE events (connected, disconnected, pairing, scan results)
 * 2. Management functions to initialize BLE, start scanning, and send data
 * 3. A periodic state machine (updateBLEConnection) called from mainLoop every iteration
 *
 * TYPICAL FLOW:
 * =============
 * 1. startBluetooth() called from main.cpp setup() → initializes NimBLE, configures scan
 * 2. updateBLEConnection() called every mainLoop iteration → periodically starts scan
 * 3. Scan callbacks (GPStarWandScanCallbacks) fire → detect Pack advertisements
 * 4. onDiscResult() → creates NimBLEClient, initiates connection to Pack
 * 5. Client callbacks (GPStarWandClientCallbacks) fire → onConnect, onConfirmPIN, onAuthenticationComplete
 * 6. After successful pairing → b_ble_connected = true
 * 7. Commands sent via packSerialSend() → calls bleQueueSerialData() which writes to Pack
 * 8. If Pack disconnects → onDisconnect() callback fires → b_ble_connected = false → scan resumes
 */

// Client-level callback class to handle Pack connection/disconnection/authentication events
// INVOKED BY: NimBLE framework when connection state changes
// NOT CALLED MANUALLY - NimBLE handles this asynchronously
class GPStarWandClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    // FIRED WHEN: Wand successfully establishes BLE connection with Pack
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE-STATE] CONNECTED"));
    #endif
  }

  void onDisconnect(NimBLEClient *pClient, int reason) {
    // FIRED WHEN: Wand loses connection with Pack (Pack powered off, out of range, or error)
    // ACTION: Clear connection flag, log reason code, allow scan to resume
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Disconnected from Pack (reason: "));
      debug(reason);
      debugln(F(")"));
    #endif
    b_ble_connected = false;
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Ready to reconnect when Pack reappears"));
    #endif
  }

  bool onConfirmPIN(uint32_t pin) {
    // NOT USED in passkey pairing - only fires with LE Secure Connections (SC=true)
    // This would be for numeric comparison, which we're NOT using
    return true;
  }

  void onPassKeyEntry(NimBLEConnInfo& connInfo) {
    // FIRED WHEN: Passkey pairing requires client to enter the passkey displayed on server
    // ACTION: Inject the passkey - must match server's passkey
    debugln(F("[BLE] >>> onPassKeyEntry() fired <<<"));
    debug(F("[BLE] Injecting passkey: "));
    debugln(BLE_PAIRING_PASSKEY);
    NimBLEDevice::injectPassKey(connInfo, BLE_PAIRING_PASSKEY);
  }
  void onAuthenticationComplete(NimBLEConnInfo &connInfo) {
    // FIRED WHEN: Passkey pairing successfully completes
    // ACTION: Mark connected, ready for commands
    // NOTE: Do NOT call blocking operations (getService, getCharacteristic) in callbacks!
    //       Characteristic discovery will be lazy-loaded in main thread or when needed
    debugln(F("[BLE] onAuthenticationComplete CALLBACK FIRED"));
    debugln(F("[BLE] PAIRING COMPLETE"));
    debugln(F("[BLE] Connection is now bonded and encrypted"));
    
    // Mark connection ready for command/control
    b_ble_connected = true;
    debugln(F("[BLE] BLE link is ACTIVE and ready for commands"));
  }
};

// Scan callback class to handle discovery of Pack advertisements
// INVOKED BY: NimBLE scan task when advertisements are received
// NOT CALLED MANUALLY - NimBLE scan task handles this in background
class GPStarWandScanCallbacks : public NimBLEScanCallbacks {
  void onDiscResult(const NimBLEAdvertisedDevice *advertisedDevice) {
    // FIRED WHEN: BLE scan receives advertisement from any nearby device
    // CALLED FROM: Background NimBLE scan task (not main loop)
    // ACTION: Check if device is Pack (by device name), FLAG it for main thread to connect
    // CRITICAL: Do NOT call pClient->connect() from scan callback!
    //           Calling connect() from BLE task blocks BLE task, preventing callbacks from firing
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Found device: "));
      debug(advertisedDevice->getName().c_str());
      debug(F(" | RSSI: "));
      debug(advertisedDevice->getRSSI());
      debugln();
    #endif

    // Look for Pack by device name (format: "GPStar-Pack-XXXX")
    String deviceName = advertisedDevice->getName().c_str();
    if(deviceName.indexOf("GPStar-Pack-") == 0) {
      debugln(F("[BLE] PACK FOUND"));
      debug(F("[BLE] Pack name: "));
      debugln(advertisedDevice->getName().c_str());
      debug(F("[BLE] Pack address: "));
      debugln(advertisedDevice->getAddress().toString().c_str());
      
      // Store the pack MAC address (not pointer - pointer is temporary!)
      g_packAddress = advertisedDevice->getAddress();
      b_pack_found = true;  // Signal main thread to connect
      
      debugln(F("[BLE] Pack flagged for connection (main thread will call connect)"));
      
      // Stop scan - main thread will stop scanning and initiate connection
      NimBLEScan *pScan = NimBLEDevice::getScan();
      if(pScan && pScan->isScanning()) {
        debugln(F("[BLE] Stopping scan from callback..."));
        pScan->stop();
      }
    }
  }

  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) {
    // Called for each advertising packet
    onDiscResult(advertisedDevice);
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


// Check and process incoming BLE notifications from Pack
// Called every main loop iteration to poll for new status characteristic data
// Uses value change detection (compares with last known value) to detect notifications
// Validates sequence numbers to detect lost packets from rapid sends
void bleProcessData() {
  if(!b_ble_connected || !g_pRemoteStatusChar) {
    return;
  }
  
  // Get current characteristic value (format: [seq_byte][packet_data])
  NimBLEAttValue value = g_pRemoteStatusChar->getValue();
  
  // Only process if we have data (at least seq + minimal packet)
  if(value.length() > 1 && value.length() <= 33) {  // 1 seq + 32 max packet
    // Change detection: compare full buffer to detect actual data changes
    static uint8_t lastBuffer[33] = {0};
    static size_t lastLength = 0;
    static uint8_t lastSeq = 0;
    
    bool changed = (value.length() != lastLength) || 
                   (memcmp(value.data(), lastBuffer, value.length()) != 0);
    
    if(changed) {
      // Extract sequence byte
      uint8_t seq = value.data()[0];
      uint8_t source_type = (seq >> 6) & 0x03;
      uint8_t seq_num = seq & 0x3F;
      
      // Check for skipped packets
      static bool firstPacket = true;
      if(!firstPacket && seq_num != (lastSeq + 1)) {
        debug(F("[PACK-TX-RX] PACKET LOSS: expected seq#"));
        debug(lastSeq + 1);
        debug(F(" got seq#"));
        debug(seq_num);
        debugln(F(""));
      }
      firstPacket = false;
      lastSeq = seq_num;
      
      // Copy to buffer (without sequence byte - just the packet part)
      size_t packet_length = value.length() - 1;
      memcpy(g_ble_rx_buffer, value.data() + 1, packet_length);  // Skip first seq byte
      g_ble_rx_length = packet_length;
      
      // Update static for next comparison
      memcpy(lastBuffer, value.data(), value.length());
      lastLength = value.length();
      
      // Parse the packet (now contains only packet data, not seq byte)
      BLEPacket packet = bleHandleData(g_ble_rx_buffer, g_ble_rx_length);
      
      #if defined(DEBUG_BLUETOOTH)
        debug(F("[PACK-TX-RX] Seq#"));
        debug(seq_num);
        debug(F(" Type="));
        switch(packet.packetType) {
          case PACKET_COMMAND:
            debug(F("COMMAND"));
            break;
          case PACKET_DATA:
            debug(F("DATA"));
            break;
          case PACKET_WAND:
            debug(F("WAND"));
            break;
          case PACKET_SMOKE:
            debug(F("SMOKE"));
            break;
          case PACKET_SYNC:
            debug(F("SYNC"));
            break;
          default:
            debug(F("UNKNOWN("));
            debug(packet.packetType);
            debug(F(")"));
            break;
        }
        debug(F(" Size="));
        debug(packet_length);
        debug(F("B"));
        
        // Show packet details
        if(packet.packetType == PACKET_COMMAND) {
          debug(F(" cmd="));
          debug(packet.cmd);
          debug(F(" d1="));
          debug(packet.d1);
        } else if(packet.packetType == PACKET_DATA) {
          debug(F(" cmd="));
          debug(packet.cmd);
          debug(F(" d[0,1,2]="));
          debug(packet.d[0]);
          debug(F(","));
          debug(packet.d[1]);
          debug(F(","));
          debug(packet.d[2]);
        }
        debugln(F(""));
      #endif
      
      // Deserialize BLE buffer into global structs (same as UART path)
      if(packet.packetType > 0) {
        switch(packet.packetType) {
          case PACKET_COMMAND:
            if(packet.cmd > 0) {
              recvCmd.s = packet.start;
              recvCmd.c = packet.cmd;
              recvCmd.d1 = packet.d1;
              recvCmd.e = packet.end;
            }
            break;
            
          case PACKET_DATA:
            if(packet.cmd > 0) {
              recvData.s = packet.start;
              recvData.c = packet.cmd;
              recvData.d[0] = packet.d[0];
              recvData.d[1] = packet.d[1];
              recvData.d[2] = packet.d[2];
              recvData.e = packet.end;
            }
            break;
            
          case PACKET_WAND:
            memcpy(&wandConfig, g_ble_rx_buffer, g_ble_rx_length);
            break;
            
          case PACKET_SMOKE:
            memcpy(&smokeConfig, g_ble_rx_buffer, g_ble_rx_length);
            break;
            
          case PACKET_SYNC:
            memcpy(&wandSyncData, g_ble_rx_buffer, g_ble_rx_length);
            break;
        }
        
        // Sync connection state for BLE (UART does this via handshake)
        if(WAND_CONN_STATE == PACK_DISCONNECTED || WAND_CONN_STATE == PACK_MISMATCH) {
          WAND_CONN_STATE = PACK_CONNECTED;
        }
        
        // Route to main packet handler (same dispatcher as UART)
        handlePacket(packet.packetType);
      }
      
      // Clear the buffer
      g_ble_rx_length = 0;
    }
  }
}

// Discover remote Pack service and characteristics
// CALLED AFTER: Pairing completes (from onAuthenticationComplete)
// ACTION: Retrieve remote service UUID, find command and status characteristics, subscribe for notifications
void discoverRemoteCharacteristics() {
  if(!g_pBLEClient) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: No BLE client available for service discovery"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
    debugln(F("[BLE] Discovering remote GPStar service..."));
  #endif

  // Get the remote service by UUID
  g_pRemoteGPStarService = g_pBLEClient->getService(BLE_SERIALDATA_SERVICE_UUID);
  
  if(!g_pRemoteGPStarService) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Could not find remote GPStar service"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
    debugln(F("[BLE] Found remote GPStar service"));
  #endif

  // Get the command characteristic (Wand → Pack) - Write target is PACKRX
  g_pRemoteCommandChar = g_pRemoteGPStarService->getCharacteristic(BLE_SERIALDATA_PACKRX_CHAR_UUID);
  
  if(!g_pRemoteCommandChar) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Could not find remote command characteristic"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Found remote command characteristic Wand to Pack"));
  #endif

  // Get the status characteristic (Pack → Wand) - Receive via indications on PACKTX
  g_pRemoteStatusChar = g_pRemoteGPStarService->getCharacteristic(BLE_SERIALDATA_PACKTX_CHAR_UUID);
  
  if(!g_pRemoteStatusChar) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Could not find remote status characteristic"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Found remote status characteristic Pack to Wand"));
  #endif

  // Enable notifications for status characteristic
  // Note: NimBLE delivers notifications automatically; we poll the characteristic value in bleProcessData()
  if(g_pRemoteStatusChar->canNotify()) {
    // Subscribe to enable notifications
    // We poll g_pRemoteStatusChar->getValue() in bleProcessData() to read new data
    g_pRemoteStatusChar->subscribe();
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Subscribed to status notifications"));
      debugln(F("[BLE] READY FOR DATA EXCHANGE"));
    #endif
  } else {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Remote status characteristic does not support notifications"));
    #endif
  }
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
    debugln(F("========== BLE Client (Wand) Initialization =========="));
  #endif

  try {
    // Initialize NimBLE with device name based on Wand ID
    String deviceName = "GPStar-Wand-" + String(wirelessMgr->getDeviceID(), DEC);
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Initializing NimBLE device as: "));
      debugln(deviceName);
    #endif
    
    NimBLEDevice::init(deviceName.c_str());

    // Configure automatic pairing with LE Secure Connections
    NimBLEDevice::setSecurityAuth(true, true, false);  // bonding, MITM, passkey pairing (not SC)
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // Enter passkey via keyboard
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); // Numeric comparison
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Security: LE Secure Connections with bonding enabled (MITM disabled for testing)"));
    #endif

    // Set up scanning for Pack advertisements
    NimBLEScan *pScan = NimBLEDevice::getScan();
    
    if(!pScan) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] ERROR Failed to get BLE scan instance"));
      #endif
      return false;
    }

    // Configure scan parameters
    // Scan interval 100ms, window 30ms = 30% duty cycle
    // This balances discovery speed with power consumption
    pScan->setInterval(100);
    pScan->setWindow(30);
    pScan->setActiveScan(false); // Passive scan (less power, no scan requests)
    pScan->setMaxResults(0);     // Don't store results in buffer (we use callbacks)
    
    // Create and store scan callback object globally (must persist for scan lifetime)
    if(!g_pWandScanCallbacks) {
      g_pWandScanCallbacks = new GPStarWandScanCallbacks();
    }
    pScan->setScanCallbacks(g_pWandScanCallbacks, false); // Use persistent callbacks, don't delete

    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Scan configured:"));
      debugln(F("  - Interval: 100ms"));
      debugln(F("  - Window: 30ms (30% duty)"));
      debugln(F("  - Mode: Passive (no scan requests)"));
      debugln(F("  - Target: Pack advertising GPStar service UUID"));
    #endif

    b_ble_initialized = true;

    #if defined(DEBUG_BLUETOOTH)
      debugln();
      debugln(F("========== BLE Client (Wand) Initialization =========="));
      debug(F("[BLE] MAC: "));
      debugln(NimBLEDevice::getAddress().toString().c_str());
      debug(F("[BLE] Name: "));
      debugln(deviceName);
      debugln(F("[BLE] Scanning for Pack..."));
      debugln();
    #endif

    return true;
  }
  catch (const std::exception &e) {
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] EXCEPTION "));
      debugln(e.what());
    #endif
    return false;
  }
}

// Wand BLE scanning/connecting handler - continuously attempts to find and connect to Pack
void handleBLEWandConnection() {
  if(!b_ble_initialized) {
    return;
  }

  // Check if already connected
  if(b_ble_connected) {
    return;  // Already connected, nothing to do
  }

  // Check if scan found a Pack (set by scan callback)
  if(b_pack_found) {
    debugln(F("[BLE] Pack found, initiating connection from main thread..."));
    b_pack_found = false;  // Clear flag
    
    debugln(F("[BLE] Creating BLE client..."));
    NimBLEClient *pClient = NimBLEDevice::createClient();
    if(pClient) {
      debugln(F("[BLE] Client created, registering callbacks..."));
      
      if(!g_pWandClientCallbacks) {
        g_pWandClientCallbacks = new GPStarWandClientCallbacks();
        debugln(F("[BLE] Created new callback object"));
      }
      pClient->setClientCallbacks(g_pWandClientCallbacks);
      
      debug(F("[BLE] Connecting to Pack at: "));
      debugln(g_packAddress.toString().c_str());
      
      // BLOCKING connect - waits for connection to complete
      if(pClient->connect(g_packAddress)) {
        g_pBLEClient = pClient;
        
        #if defined(DEBUG_BLUETOOTH)
          debugln(F("[BLE-STATE] CONN_SYNC_DONE"));
          debugln(F("[BLE-STATE] PAIRING_START"));
        #endif
        
        // NOW initiate pairing - call secureConnection() right after connect() returns
        // This is BLOCKING but necessary for pairing handshake
        if(pClient->secureConnection()) {
          #if defined(DEBUG_BLUETOOTH)
            debugln(F("[BLE-STATE] PAIR_INITIATED"));
          #endif
          b_ble_connected = true;
        } else {
          #if defined(DEBUG_BLUETOOTH)
            debugln(F("[BLE-STATE] ERROR secureConnection"));
          #endif
          b_ble_connected = false;
        }
      } else {
        #if defined(DEBUG_BLUETOOTH)
          debugln(F("[BLE-STATE] ERROR connect"));
        #endif
        NimBLEDevice::deleteClient(pClient);
        g_pBLEClient = nullptr;
      }
    } else {
      debugln(F("[BLE] ERROR: Failed to create BLE client!"));
    }
    return;
  }

  // Not connected and no Pack found - start scanning
  NimBLEScan *pScan = NimBLEDevice::getScan();
  if(!pScan) {
    debugln(F("[BLE] ERROR: Cannot get scan instance"));
    return;
  }

  static bool b_scan_started = false;
  if(!b_scan_started) {
    debugln(F("[BLE] Starting BLE scan for Pack..."));
    pScan->start(0, false);  // 0 = continuous scan
    b_scan_started = true;
  }

  // If scan found a Pack, callback will flag it and this function will connect on next call
  // If Pack is not found, scan continues in background
}

// Queue serialized data for transmission at end of loop
// Accumulates frames, then bleFlushQueues() sends once with metadata byte
/*
 * Queue a serialized packet for transmission to Pack.
 * 
 * Phase 3 (Refactored): Uses BLE library message queueing.
 * Per BLE_TRANSPORT.md: Each packet becomes one BLEMessage with frame markers preserved.
 * Sequence counter is managed by library; status tracking per message.
 */
void bleQueueSerialData(const uint8_t* pData, size_t length) {
  if(!b_ble_enabled || !pData || length == 0) {
    return;  // BLE disabled or invalid input, silently drop
  }
  
  // Create a BLEMessage from the packet data
  // The packet already has frame markers (0x02 start, 0x04 end)
  BLEMessage msg;
  uint8_t result = BLEPacketParser_CreateMessage(
    BLEPacketParser_GetPacketType(pData, length),
    0,  // Sequence will be managed by library if needed
    pData,
    length,
    &msg
  );
  
  if(result != BLE_PACKET_INVALID) {
    // Enqueue the message for transmission
    uint8_t enqueue_result = BLEQueueManager_Enqueue(&g_ble_tx_queue, &msg);
    
    #if defined(DEBUG_BLUETOOTH)
      if(enqueue_result == BLE_QUEUE_OK) {
        debug(F("[WAND→PACK-Q] Message queued, depth="));
        debug(BLEQueueManager_GetCount(&g_ble_tx_queue));
        debug(F("/"));
        debug(BLE_QUEUE_SIZE);
        debugln(F(" msgs"));
      } else if(enqueue_result == BLE_QUEUE_FULL) {
        debug(F("[WAND→PACK-Q] OVERFLOW: queue full, overflows="));
        debug(g_ble_tx_queue.overflowCount);
        debugln();
      }
    #endif
  } else {
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[WAND→PACK-Q] Invalid packet: start=0x"));
      if(length > 0) debug(pData[0], HEX);
      debug(F(" end=0x"));
      if(length > 0) debug(pData[length-1], HEX);
      debug(F(" len="));
      debug(length);
      debugln();
    #endif
  }
}

/*
 * Flush queued messages as BLE writes (one message per write).
 * 
 * Phase 3 (Refactored): Dequeues and sends individual BLEMessages.
 * Per BLE_TRANSPORT.md: Each write carries one complete message.
 * Uses write-without-response for efficiency (Wand→Pack characteristic).
 */
void bleFlushQueues() {
  if(!b_ble_enabled || !b_ble_connected || !g_pBLEClient) {
    return;  // BLE not ready, queues stay intact for next loop
  }
  
  // Lazy-load remote characteristics on first use
  if(!g_pRemoteCommandChar) {
    debugln(F("[WAND→PACK] First flush - discovering Pack command characteristic..."));
    NimBLERemoteService *pService = g_pBLEClient->getService(BLE_SERIALDATA_SERVICE_UUID);
    if(pService) {
      // Write commands to PACKRX (ffe2)
      g_pRemoteCommandChar = pService->getCharacteristic(BLE_SERIALDATA_PACKRX_CHAR_UUID);
      if(!g_pRemoteCommandChar) {
        debugln(F("[WAND→PACK] ERROR: Could not find Pack command characteristic"));
        return;
      }
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[WAND→PACK] Pack command characteristic discovered successfully"));
      #endif
    } else {
      debugln(F("[WAND→PACK] ERROR: Could not find Pack GPStar service"));
      return;
    }
  }
  
  // Process all queued messages (max 16 per queue per spec)
  while(!BLEQueueManager_IsEmpty(&g_ble_tx_queue)) {
    BLEMessage msg;
    uint8_t result = BLEQueueManager_Dequeue(&g_ble_tx_queue, &msg);
    
    if(result != BLE_QUEUE_OK) {
      break;  // Queue empty or error
    }
    
    // Send message as write-without-response (message payload already has frame markers)
    // Metadata format: [7:6]=source, [5:0]=sequence
    uint8_t ble_buffer[258];  // 1 byte metadata + 257 (1B seq + 256B payload max)
    ble_buffer[0] = (0x00 << 6) | (msg.sequence & 0x3F);
    
    // Copy message payload (includes frame markers 0x02...0x04)
    memcpy(&ble_buffer[1], msg.payload, msg.length);
    
    // Validate characteristic is still available and writable
    if(!g_pRemoteCommandChar || !g_pRemoteCommandChar->canWrite()) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[WAND→PACK] WARNING: Pack characteristic unavailable, re-queueing message"));
      #endif
      // Re-queue the message we just dequeued since we couldn't send it
      BLEQueueManager_Enqueue(&g_ble_tx_queue, &msg);
      // Clear the characteristic pointer to force rediscovery on next flush
      g_pRemoteCommandChar = nullptr;
      break;
    }
    
    // Attempt to write the message
    if(g_pRemoteCommandChar->writeValue(ble_buffer, msg.length + 1, false)) {
      #if defined(DEBUG_BLUETOOTH)
        debug(F("[WAND→PACK-SEND] Seq#"));
        debug(msg.sequence);
        debug(F(" PktType="));
        debug(msg.packetType);
        debug(F(" Len="));
        debug(msg.length);
        debug(F("B Depth="));
        debug(BLEQueueManager_GetCount(&g_ble_tx_queue));
        debug(F("/"));
        debug(BLE_QUEUE_SIZE);
        debugln();
      #endif
    } else {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[WAND→PACK] ERROR: Failed to write value, re-queueing message"));
      #endif
      // Re-queue the message and force rediscovery
      BLEQueueManager_Enqueue(&g_ble_tx_queue, &msg);
      g_pRemoteCommandChar = nullptr;
      break;
    }
  }
}

// Apply queued serial data from BLE (fallback when UART unavailable)
// Processes queued frame identical to UART path
void bleApplySerialData() {
  if(!b_serial_rx_ready || g_serial_rx_length == 0) {
    return;  // No queued data
  }
  
  // Parse the frame
  BLEPacket packet = bleHandleData(g_serial_rx_buffer, g_serial_rx_length);
  
  if(packet.packetType > 0) {
    // Deserialize into same global structs as UART
    switch(packet.packetType) {
      case PACKET_COMMAND:
        if(packet.cmd > 0) {
          recvCmd.s = packet.start;
          recvCmd.c = packet.cmd;
          recvCmd.d1 = packet.d1;
          recvCmd.e = packet.end;
        }
        break;
        
      case PACKET_DATA:
        if(packet.cmd > 0) {
          recvData.s = packet.start;
          recvData.c = packet.cmd;
          recvData.d[0] = packet.d[0];
          recvData.d[1] = packet.d[1];
          recvData.d[2] = packet.d[2];
          recvData.e = packet.end;
        }
        break;
        
      case PACKET_WAND:
        memcpy(&wandConfig, g_serial_rx_buffer, g_serial_rx_length);
        break;
        
      case PACKET_SMOKE:
        memcpy(&smokeConfig, g_serial_rx_buffer, g_serial_rx_length);
        break;
    }
    
    // Ensure Pack connection state is synchronized
    if(WAND_CONN_STATE == PACK_DISCONNECTED || WAND_CONN_STATE == PACK_MISMATCH) {
      WAND_CONN_STATE = PACK_CONNECTED;
    }
    
    // Route to central packet handler (same as UART)
    handlePacket(packet.packetType);
  }
  
  // Clear for next frame
  b_serial_rx_ready = false;
  g_serial_rx_length = 0;
}

// Periodic BLE connection manager - call from mainLoop every iteration
// Continuously attempts Pack discovery and connection until successful
//
// STATE MACHINE BEHAVIOR:
// ======================
// Phase 1 (initialization): b_ble_initialized = false
//   - BLE is not running; this function returns immediately
//   - Call startBluetooth() to initialize NimBLE device and scan
//
// Phase 2 (scanning disconnected): b_ble_initialized = true, b_ble_connected = false
//   - Every 500ms, this function calls handleBLEWandConnection()
//   - handleBLEWandConnection() starts a continuous BLE scan (duration=0, non-blocking)
//   - Scan runs in background NimBLE task; does not block main loop
//   - Scan callback (GPStarWandScanCallbacks::onDiscResult) monitors for Pack advertisements
//   - When Pack is found:
//     a. Verifies Pack is advertising GPStar service UUID (0000ffe0-...)
//     b. Extracts Pack ID from device name (format "GPStar-Pack-XXXX")
//     c. Creates NimBLEClient and initiates connection
//     d. Sets b_ble_connected = true immediately (optimistic)
//     e. Connection event: onConnect() callback fires → logs "[BLE] *** CONNECTED TO PACK ***"
//     f. LE Secure Connections pairing begins automatically
//     g. onConfirmPIN() callback auto-confirms pairing PIN (numeric comparison)
//     h. onAuthenticationComplete() fires → logs "*** PAIRING COMPLETE ***"
//     i. Connection becomes bonded and encrypted
//   - If Pack is NOT found:
//     * Scan continues indefinitely in background (no timeout, no retry backoff)
//     * No error logged; scan runs silently
//     * Serial console shows "[BLE] Starting BLE scan for Pack..." once at startup
//     * handleBLEWandConnection() becomes a no-op on subsequent 500ms calls (b_scan_started=true)
//     * When Pack eventually appears (powered on later), scan callback immediately initiates connection
//     * If Pack never appears, Wand remains in "scanning disconnected" state forever
//
// Phase 3 (connected): b_ble_initialized = true, b_ble_connected = true
//   - b_ble_connected flag is set by onConnect() callback
//   - handleBLEWandConnection() returns early (skips starting a new scan)
//   - Connection is maintained by BLE controller; periodic heartbeat ensures link health
//   - If connection drops (Pack powered off, out of range, manual disconnect):
//     * onDisconnect() callback fires → sets b_ble_connected = false, logs reason code
//     * Returns to Phase 2 (scanning disconnected)
//     * Scan resumes on next 500ms updateBLEConnection() call
//
// HARDCODED PACK ID:
// ==================
// Phase 1 uses no stored or configurable Pack ID. The Wand accepts the FIRST Pack it finds
// advertising the GPStar service UUID (0000ffe0-...). This works for single-user demos but
// does not support:
//   - User selecting which Pack to pair with (if multiple Packs nearby)
//   - Reconnecting to the same Pack after power reset
//   - Distinguishing "wrong Pack found" from "no Pack found"
//
// Future versions (Phase 1B+) should:
//   - Store a persistent i_target_pack_id in NVS
//   - Extract Pack ID from advertisement name: strtol(packName.substring(12), NULL, 16)
//   - Filter: only connect if found Pack ID matches i_target_pack_id
//   - Implement connection timeout and exponential backoff if target Pack not found
//   - Provide web interface to select and configure target Pack ID during pairing
//
// NO TIMEOUT / NO BACKOFF (CURRENT PHASE 1):
// ===========================================
// If Pack is not found:
//   - Wand scans forever without stopping, reducing power efficiency
//   - No exponential backoff; scan continues at full 30% duty cycle (100ms interval, 30ms window)
//   - User gets no indication of failure; app appears "stuck" if Pack is unavailable
//   - Serial console shows one "[BLE] Starting BLE scan for Pack..." message, then silence
//   - If Pack appears hours later, connection succeeds immediately
//
// FUTURE IMPROVEMENTS (Phase 1B):
//   - Connection timeout: if Pack not found after 30 seconds, stop scanning or reduce duty cycle
//   - Exponential backoff: 30s scan at 30% duty, then 2min scan at 5% duty, then 10min at 1%
//   - UI feedback: Wand web interface shows "Searching for Pack..." or "Pack not found"
//   - Serial alert: every 30 seconds, print "[BLE] Still searching for Pack ID 0xXXXX..."
//   - UART fallback: if BLE scan times out, check for UART connection before giving up
//
// SERIAL DEBUG OUTPUT (with DEBUG_BLUETOOTH enabled):
// =========================================================
// On successful connection:
//   [BLE] Starting BLE scan for Pack...
//   [BLE] Found device: GPStar-Pack-XXXX | RSSI: -45 | UUID: 0000ffe0-...
//   [BLE] *** PACK FOUND! ***
//   [BLE] Pack name: GPStar-Pack-XXXX
//   [BLE] Pack address: XX:XX:XX:XX:XX:XX
//   [BLE] Pack ID: 0xXXXX
//   [BLE] Initiating connection...
//   [BLE] *** CONNECTED TO PACK ***
//   [BLE] *** PAIRING PIN RECEIVED: 123456 ***
//   [BLE] Confirming pairing (automatic)...
//   [BLE] *** PAIRING COMPLETE ***
//   [BLE] Connection is now bonded and encrypted
//
// On failed connection (Pack not found):
//   [BLE] Starting BLE scan for Pack...
//   (silence - scan runs in background with no output)
//   (if DEBUG_BLUETOOTH is on, you see advertisements from other BLE devices, but not your Pack)
//   (after several hours, if you power on the Pack:)
//   [BLE] Found device: GPStar-Pack-XXXX | RSSI: -50 | UUID: 0000ffe0-...
//   [BLE] *** PACK FOUND! ***
//   (connection proceeds as above)
//
void updateBLEConnection() {
  if(!b_ble_enabled || !b_ble_initialized) {
    return;
  }

  static millisDelay ms_bleCheck;
  static bool b_timer_initialized = false;
  
  if(!b_timer_initialized) {
    ms_bleCheck.start(0); // Start immediately on first call
    b_timer_initialized = true;
  }
  
  if(ms_bleCheck.justFinished()) {
    handleBLEWandConnection(); // Start/maintain BLE scan (every 500ms)
    ms_bleCheck.start(500);
  }
}
