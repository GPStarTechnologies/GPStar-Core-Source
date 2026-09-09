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
bool b_ble_scanning = false; // Scan is actively running (reserved for future use)
uint16_t i_target_pack_id = 0; // Pack device ID extracted from advertisement (reserved for future use)

// Global callback objects (must persist for lifetime of BLE client)
NimBLEClientCallbacks *g_pWandClientCallbacks = nullptr;
NimBLEScanCallbacks *g_pWandScanCallbacks = nullptr;

// Pack discovery info (stored by scan callback, used by main thread to connect)
NimBLEAddress g_packAddress;  // MAC address of discovered Pack
bool b_pack_found = false;    // Set by scan callback, cleared by main thread after connecting

// BLE notification queue (store incoming Pack notifications for main loop processing)
uint8_t g_ble_notification_buffer[32] = {0};  // Buffer for received BLE notification bytes
size_t g_ble_notification_length = 0;         // Number of bytes in buffer
bool b_ble_notification_ready = false;        // Flag: notification ready to process

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

// Forward declaration of characteristic discovery function (defined after callbacks)
void discoverRemoteCharacteristics();

// Forward declaration of notification callback (defined later in file)
static void wandNotifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

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
 * 7. Commands sent via packSerialSend() → calls bleSendData() which writes to Pack
 * 8. If Pack disconnects → onDisconnect() callback fires → b_ble_connected = false → scan resumes
 */

// Client-level callback class to handle Pack connection/disconnection/authentication events
// INVOKED BY: NimBLE framework when connection state changes
// NOT CALLED MANUALLY - NimBLE handles this asynchronously
class GPStarWandClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    // FIRED WHEN: Wand successfully establishes BLE connection with Pack
    debugln(F("[BLE] >>> onConnect() fired <<<"));
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
    debugln(F("[BLE] !!! onAuthenticationComplete() CALLBACK FIRED !!!"));
    debugln(F("[BLE] *** PAIRING COMPLETE ***"));
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
      debugln(F("[BLE] *** PACK FOUND! ***"));
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


// Notification callback for Pack status updates received via BLE
// This is a function-based callback (not class-based) as required by NimBLE's remote characteristic API
// INVOKED BY: NimBLE notification task when Pack sends status data
// NOT CALLED MANUALLY - NimBLE handles this asynchronously when notified
static void wandNotifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // FIRED WHEN: Pack sends a BLE notification on status characteristic
  // PARAM pData: Raw bytes of serialized Pack status (same format as UART)
  // PARAM length: Size of status data in bytes
  // NOTE: This runs in NimBLE task context, not main loop - queue the data for main loop processing
  
  debugln(F("[BLE-RX] CALLBACK FIRED"));
  debug(F("[BLE-RX] length="));
  debug(length);
  debug(F(" pData="));
  debug((uint32_t)pData);
  debugln();
  
  if(length > 0 && pData != nullptr && length <= 32) {
    // Queue notification for main loop processing (same approach as Pack's onWrite callback)
    memcpy(g_ble_notification_buffer, pData, length);
    g_ble_notification_length = length;
    b_ble_notification_ready = true;
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE-RX] Queued notification ("));
      debug(length);
      debugln(F(" bytes)"));
    #endif
  } else {
    debug(F("[BLE-RX] REJECTED: length="));
    debug(length);
    debug(F(" pData="));
    debug((uint32_t)pData);
    debugln();
  }
}

// Discover remote Pack service and characteristics, then register notification callback
// CALLED AFTER: Pairing completes (from onAuthenticationComplete)
// ACTION: Retrieve remote service UUID, find command and status characteristics, register for notifications
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
  g_pRemoteGPStarService = g_pBLEClient->getService(GPSTAR_SERVICE_UUID);
  
  if(!g_pRemoteGPStarService) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Could not find remote GPStar service"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
    debugln(F("[BLE] *** Found remote GPStar service ***"));
  #endif

  // Get the command characteristic (Wand → Pack)
  g_pRemoteCommandChar = g_pRemoteGPStarService->getCharacteristic(GPSTAR_COMMAND_CHAR_UUID);
  
  if(!g_pRemoteCommandChar) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Could not find remote command characteristic"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
    debugln(F("[BLE] *** Found remote command characteristic (Wand → Pack) ***"));
  #endif

  // Get the status characteristic (Pack → Wand)
  g_pRemoteStatusChar = g_pRemoteGPStarService->getCharacteristic(GPSTAR_STATUS_CHAR_UUID);
  
  if(!g_pRemoteStatusChar) {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Could not find remote status characteristic"));
    #endif
    return;
  }

  #if defined(DEBUG_BLUETOOTH)
    debugln(F("[BLE] *** Found remote status characteristic (Pack → Wand) ***"));
  #endif

  // Enable notifications for status characteristic
  // Note: NimBLE delivers notifications through client-level callbacks or by value updates
  // We subscribe to enable them, and wandNotifyCallback will be triggered by the BLE stack
  if(g_pRemoteStatusChar->canNotify()) {
    g_pRemoteStatusChar->subscribe();
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Subscribed to status notifications"));
      debugln(F("[BLE] *** BLE READY FOR DATA EXCHANGE ***"));
    #endif
  } else {
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] ERROR: Remote status characteristic does not support notifications"));
    #endif
  }
}

// Process incoming BLE notifications from Pack
// Called from main loop to parse and handle queued BLE notification bytes
void processBLENotification() {
  static unsigned long lastDebug = 0;
  if(millis() - lastDebug > 5000) {
    debugln(F("[BLE] processBLENotification() called (checking for queued notifications)"));
    lastDebug = millis();
  }
  
  if(!b_ble_notification_ready || g_ble_notification_length == 0) {
    return;  // No notification queued
  }
  
  debugln(F("[BLE] *** PROCESSING QUEUED NOTIFICATION ***"));
  
  // Clear the ready flag
  b_ble_notification_ready = false;
  
  // Parse the queued notification
  BLEPacket packet = bleHandleData(g_ble_notification_buffer, g_ble_notification_length);
  
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

  // Deserialize BLE notification into same global structs used by UART
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
        memcpy(&wandConfig, g_ble_notification_buffer, g_ble_notification_length);
        break;
        
      case PACKET_SMOKE:
        memcpy(&smokeConfig, g_ble_notification_buffer, g_ble_notification_length);
        break;
        
      case PACKET_SYNC:
        memcpy(&wandSyncData, g_ble_notification_buffer, g_ble_notification_length);
        break;
    }
    
    // Ensure BLE connection state is synchronized (UART does this via serial handshake)
    // BLE bypass: directly update state to allow handlers to process commands
    if(WAND_CONN_STATE == PACK_DISCONNECTED || WAND_CONN_STATE == PACK_MISMATCH) {
      WAND_CONN_STATE = PACK_CONNECTED;
    }
    
    // Route to central packet handler (same dispatcher as UART)
    handlePacket(packet.packetType);
  }
  
  // Clear the buffer
  g_ble_notification_length = 0;
}

bool startBluetooth() {
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
    String deviceName = "GPStar-Wand-" + String(wirelessMgr->getDeviceID(), HEX);
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Initializing NimBLE device as: "));
      debugln(deviceName);
    #endif
    
    NimBLEDevice::init(deviceName.c_str());

    // Configure automatic pairing with LE Secure Connections
    NimBLEDevice::setSecurityAuth(true, true, false);  // bonding, MITM, passkey pairing (not SC)
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);  // Enter passkey via keyboard
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); // Numeric comparison
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Security: LE Secure Connections with bonding enabled (MITM disabled for testing)"));
    #endif

    // Set up scanning for Pack advertisements
    NimBLEScan *pScan = NimBLEDevice::getScan();
    
    if(!pScan) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] *** ERROR: Failed to get BLE scan instance"));
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
      debug(F("[BLE] *** EXCEPTION: "));
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
    #if defined(DEBUG_BLUETOOTH)
      static unsigned long lastStatusTime = 0;
      if(millis() - lastStatusTime > 10000) {
        debugln(F("[BLE] *** BLE CONNECTED to Pack ***"));
        lastStatusTime = millis();
      }
    #endif
    return;
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
        debugln(F("[BLE] >>> CONNECTED <<<"));
        g_pBLEClient = pClient;
        
        // NOW initiate pairing - call secureConnection() right after connect() returns
        // This is BLOCKING but necessary for pairing handshake
        debugln(F("[BLE] Initiating secure connection (pairing)..."));
        if(pClient->secureConnection()) {
          debugln(F("[BLE] >>> PAIRING INITIATED <<<"));
          b_ble_connected = true;
        } else {
          debugln(F("[BLE] ERROR: secureConnection() failed"));
          b_ble_connected = false;
        }
      } else {
        debugln(F("[BLE] ERROR: connect() failed"));
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

// Send serialized data via BLE characteristic (receives same buffer that was sent via UART)
// This is called after packComs.txObj() serializes and packComs.sendData() sends via UART
// The function checks BLE state and sends the same bytes via characteristic
void bleSendData(const uint8_t* pData, size_t length) {
  if(!b_ble_connected || !g_pBLEClient) {
    return;  // BLE not ready, function decides silently
  }
  
  // Lazy-load remote characteristics on first use (skip discovery, just use UUID directly)
  if(!g_pRemoteCommandChar || !g_pRemoteStatusChar) {
    debugln(F("[BLE] First send - getting remote characteristics..."));
    NimBLERemoteService *pService = g_pBLEClient->getService("0000ffe0-0000-1000-8000-00805f9b34fb");
    if(pService) {
      // Get command characteristic (Wand → Pack)
      if(!g_pRemoteCommandChar) {
        g_pRemoteCommandChar = pService->getCharacteristic("0000ffe1-0000-1000-8000-00805f9b34fb");
        if(g_pRemoteCommandChar) {
          debugln(F("[BLE] OK: Got remote command characteristic"));
        } else {
          debugln(F("[BLE] ERROR: Could not get command characteristic"));
          return;
        }
      }
      
      // Get status characteristic (Pack → Wand) and subscribe to notifications
      if(!g_pRemoteStatusChar) {
        g_pRemoteStatusChar = pService->getCharacteristic("0000ffe2-0000-1000-8000-00805f9b34fb");
        if(g_pRemoteStatusChar) {
          debugln(F("[BLE] OK: Got remote status characteristic"));
          if(g_pRemoteStatusChar->canNotify()) {
            debugln(F("[BLE] Status characteristic supports notifications, subscribing..."));
            bool subResult = g_pRemoteStatusChar->subscribe(wandNotifyCallback);
            debug(F("[BLE] Subscribe result: "));
            debugln(subResult ? "SUCCESS" : "FAILED");
          } else {
            debugln(F("[BLE] ERROR: Status characteristic does NOT support notifications"));
          }
        } else {
          debugln(F("[BLE] ERROR: Could not get status characteristic"));
        }
      }
    } else {
      debugln(F("[BLE] ERROR: Could not get remote service"));
      return;
    }
  }
  
  // Now send the data
  if(g_pRemoteCommandChar && g_pRemoteCommandChar->canWrite()) {
    g_pRemoteCommandChar->writeValue((uint8_t*)pData, length, false);  // false = write without response
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE-TX] Sent "));
      debug(length);
      debugln(F(" bytes via BLE"));
    #endif
  }
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
