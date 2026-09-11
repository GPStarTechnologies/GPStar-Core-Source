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

// BLE Message Queue Library (Phase 3 refactoring) - umbrella header
#include <BLE.h>

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
static uint8_t g_ble_tx_sequence = 0;  // Sequence counter for BLE notifications (detect lost packets)
bool b_ble_enabled = true; // Master enable/disable flag; set to false to turn off all BLE scanning and connection
bool b_ble_initialized = false; // NimBLE device initialized, service/characteristics created, advertising started
bool b_ble_connected = false; // Indicates when the Wand has successfully connected and bonded with this Pack

// BLE message queues (message-level queueing per BLE_TRANSPORT.md)
// Pack TX: messages from Pack to Wand (using indications for reliability)
// Pack RX: messages from Wand to Pack (written to PackRX characteristic)
// Note: Queues are zero-initialized by default in C; no explicit init needed before use
BLEMessageQueue g_ble_tx_queue = {};
BLEMessageQueue g_ble_rx_queue = {};  // RX queue for incoming Wand messages

// Legacy RX buffer (kept for compatibility, but prefer using g_ble_rx_queue)
uint8_t g_ble_rx_buffer[32] = {0};  // Buffer for received BLE bytes
size_t g_ble_rx_length = 0;         // Number of bytes in buffer
bool b_ble_rx_ready = false;        // Flag: data ready to process

// Global callback objects (must persist for lifetime of BLE server)
NimBLEServerCallbacks *g_pPackServerCallbacks = nullptr;
NimBLECharacteristicCallbacks *g_pPackCharacteristicCallbacks = nullptr;

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
 * 10. bleQueueSerialData() function notifies Wand with serialized status data
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
      debugln(F("[BLE] PAIRING COMPLETE with Wand"));
      debugln(F("[BLE] Connection is now bonded and encrypted"));
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
      // if(packetType == PACKET_IDENTITY) {
      //   debugln(F("[WAND→PACK] IDENTITY PACKET RECEIVED"));
      //   debug(F("[WAND→PACK] Packet length: "));
      //   debugln(rxValue.length());
        
      //   if(rxValue.length() >= 4) {
      //     uint8_t deviceType = (uint8_t)rxValue[1];
      //     uint8_t deviceIDHi = (uint8_t)rxValue[2];
      //     uint8_t deviceIDLo = (uint8_t)rxValue[3];
      //     uint16_t deviceID = ((uint16_t)deviceIDHi << 8) | deviceIDLo;
          
      //     debug(F("[WAND→PACK] Identity: deviceType="));
      //     debug(deviceType);
      //     debug(F(" deviceID="));
      //     debug(deviceID, HEX);
      //     debugln(F(""));
          
      //     // Validate this is a Wand (IR_DEVICE_NEUTRONA_WAND = 0x0)
      //     if(deviceType == 0x00) {
      //       b_ble_connected = true;
      //       debugln(F("[WAND→PACK] WAND IDENTITY VERIFIED - Connection Active"));
      //     } else {
      //       debugln(F("[WAND→PACK] ERROR: Invalid device type - rejecting connection"));
      //     }
      //   } else {
      //     debug(F("[WAND→PACK] ERROR: Identity packet too short (expected 4 bytes, got "));
      //     debug(rxValue.length());
      //     debugln(F(")"));
      //   }
      //   return;
      // }
      
      // Only process other commands if we've verified this is a Wand
      if(!b_ble_connected) {
        #if defined(DEBUG_BLUETOOTH)
          debugln(F("[WAND→PACK] ERROR: Command received before identity verification - ignoring"));
        #endif
        return;
      }
      
      // Queue the command for processing by checkWand() in main loop
      if(rxValue.length() > 1) {  // Must have at least metadata + 1 byte payload
        // Extract metadata byte (byte[0])
        uint8_t metadata = (uint8_t)rxValue[0];
        uint8_t packetType = (metadata >> 5) & 0x07;  // bits [7:5]
        uint8_t sequence = metadata & 0x1F;            // bits [4:0]
        size_t payload_length = rxValue.length() - 1;
        
        // Validate packet type is within expected range (0-6)
        if(packetType > 6) {
          #if defined(DEBUG_BLUETOOTH)
            debug(F("[WAND→PACK] ERROR: Invalid packet type="));
            debug(packetType);
            debugln(F(""));
          #endif
          return;
        }
        
        // Copy payload to temporary buffer for message creation
        uint8_t tempPayload[256] = {0};
        if(payload_length > 0 && payload_length <= 256) {
          memcpy(tempPayload, rxValue.c_str() + 1, payload_length);
        }
        
        // Create BLE message and enqueue to RX queue
        if(payload_length <= 256) {
          BLEMessage msg;
          uint8_t result = BLEQueueManager_CreateMessage(
            packetType,
            sequence,
            tempPayload,
            payload_length,
            &msg
          );
          
          if(result != BLE_PACKET_INVALID) {
            uint8_t enqueue_result = BLEQueueManager_Enqueue(&g_ble_rx_queue, &msg);
            
            #if defined(DEBUG_BLUETOOTH)
              if(enqueue_result == BLE_QUEUE_OK) {
                debug(F("[PACK-RX] Seq#"));
                debug(sequence);
                debug(F(" Payload="));
                debug(payload_length);
                debug(F(" bytes | "));
                
                // Parse frame type for logging
                if(payload_length >= 4) {
                  uint8_t start = tempPayload[0];
                  uint8_t end = tempPayload[payload_length - 1];
                  
                  if(start == A_COM_START && end == A_COM_END) {
                    if(payload_length == 6) {
                      uint16_t cmd = (uint16_t)tempPayload[1] | ((uint16_t)tempPayload[2] << 8);
                      uint16_t d1 = (uint16_t)tempPayload[3] | ((uint16_t)tempPayload[4] << 8);
                      debug(F("COMMAND c="));
                      debug(cmd);
                      debug(F(" d1="));
                      debug(d1);
                    } else {
                      debug(F("("));
                      debug(payload_length);
                      debug(F("B frame)"));
                    }
                  } else {
                    debug(F("(invalid markers)"));
                  }
                }
                debugln();
              }
            #endif
          }
        } else {
          #if defined(DEBUG_BLUETOOTH)
            debug(F("[WAND→PACK] ERROR: Payload too large ("));
            debug(payload_length);
            debugln(F(" B exceeds 256B limit)"));
          #endif
        }
      } else {
        #if defined(DEBUG_BLUETOOTH)
          debugln(F("[WAND→PACK] ERROR: Received less than 2 bytes"));
        #endif
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
void bleProcessData() {
  // Dequeue and process all available RX messages
  while(!BLEQueueManager_IsEmpty(&g_ble_rx_queue)) {
    BLEMessage msg;
    uint8_t result = BLEQueueManager_Dequeue(&g_ble_rx_queue, &msg);
    
    if(result != BLE_QUEUE_OK) {
      break;  // Queue empty or error
    }
    
    // Parse the queued message payload
    BLEPacket packet = bleHandleData(msg.payload, msg.length);
    
    #if defined(DEBUG_BLUETOOTH)
      // Parsed packet structure
      debug(F("[BLE-RX] Parsed: type="));
      switch(packet.packetType) {
        case PACKET_COMMAND: debug(F("COMMAND(1)")); break;
        case PACKET_DATA: debug(F("DATA(2)")); break;
        case PACKET_PACK: debug(F("PACK(3)")); break;
        case PACKET_WAND: debug(F("WAND(4)")); break;
        case PACKET_SMOKE: debug(F("SMOKE(5)")); break;
        case PACKET_SYNC: debug(F("SYNC(6)")); break;
        default: debug(F("UNKNOWN(0)")); break;
      }
      debug(F(" | c="));
      debug(packet.cmd);
      debug(F(" d1="));
      debugln(packet.d1);
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
        memcpy(&wandConfig, msg.payload, msg.length);
        break;
        
      case PACKET_SMOKE:
        memcpy(&smokeConfig, msg.payload, msg.length);
        break;
        
      case PACKET_PACK:
        memcpy(&packConfig, msg.payload, msg.length);
        break;
        
      case PACKET_SYNC:
        // Sync packets from Wand (handled same as COMMAND)
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
    debugln(F("========== BLE Server (Pack) Initialization =========="));
  #endif

  try {
    // Initialize NimBLE with device name based on Pack ID
    String deviceName = "GPStar-Pack-" + String(wirelessMgr->getDeviceID(), DEC);
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] Initializing NimBLE device "));
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
        debugln(F("[BLE] ERROR Failed to create BLE Server"));
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
    g_pGPStarService = g_pBLEServer->createService(BLE_SERIALDATA_SERVICE_UUID);
    
    if(!g_pGPStarService) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] ERROR Failed to create GPSta Serial Data Service"));
      #endif
      return false;
    }
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[BLE] GPStar Serial Data Service created, UUID: "));
      debugln(BLE_SERIALDATA_SERVICE_UUID);
    #endif

    // Create command characteristic (Wand writes commands to Pack)
    // Use PACKRX UUID (ffe2) since this receives data FROM wand
    // WRITE_NR only: Wand calls writeValue(..., false) for write-without-response
    g_pCommandCharacteristic = g_pGPStarService->createCharacteristic(
      BLE_SERIALDATA_PACKRX_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    
    if(!g_pCommandCharacteristic) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] ERROR Failed to create command characteristic"));
      #endif
      return false;
    }
    
    // Create and store callback object globally (must persist for characteristic lifetime)
    if(!g_pPackCharacteristicCallbacks) {
      g_pPackCharacteristicCallbacks = new GPStarPackCharacteristicCallbacks();
    }
    g_pCommandCharacteristic->setCallbacks(g_pPackCharacteristicCallbacks);
    
    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Created Wand to Pack (PACKRX) Characteristic (WRITE/WRITE_NR)"));
    #endif

    // Create status characteristic (Pack sends status updates via indications)
    // Properties: INDICATE (enables indications) + READ (let Wand read value)
    // DO NOT include WRITE - this is receive-only (Pack → Wand)
    // DO NOT manually create CCCD - NimBLE auto-creates it with proper permissions when INDICATE is set
    // Use PACKTX UUID (ffe1) since this transmits data FROM pack
    g_pStatusCharacteristic = g_pGPStarService->createCharacteristic(
      BLE_SERIALDATA_PACKTX_CHAR_UUID,
      NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::READ
    );
    
    if(!g_pStatusCharacteristic) {
      #if defined(DEBUG_BLUETOOTH)
        debugln(F("[BLE] ERROR Failed to create status characteristic"));
      #endif
      return false;
    }

    #if defined(DEBUG_BLUETOOTH)
      debugln(F("[BLE] Created Pack to Wand (PACKTX) Characteristic (INDICATE/READ)"));
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
      debug(F("[BLE] EXCEPTION "));
      debugln(e.what());
    #endif
    return false;
  }
}

/*
 * Queue a serialized packet for transmission to Wand.
 * 
 * Phase 3 (Refactored): Uses BLE library message queueing.
 * Per BLE_TRANSPORT.md: Each packet becomes one BLEMessage with frame markers preserved.
 * Sequence counter is managed by library; status tracking per message.
 */
void bleQueueSerialData(const uint8_t* pData, size_t length, uint8_t packetType) {
  if(!b_ble_enabled || !pData || length == 0) {
    return;  // BLE disabled or invalid input, silently drop
  }
  
  // Create a BLEMessage from the packet data
  // The packet already has frame markers (0x02 start, 0x04 end)
  BLEMessage msg;
  uint8_t result = BLEQueueManager_CreateMessage(
    packetType,
    g_ble_tx_sequence++,  // Increment sequence for each message (wraps at 256)
    pData,
    length,
    &msg
  );
  
  if(result != BLE_PACKET_INVALID) {
    // Enqueue the message for transmission
    uint8_t enqueue_result = BLEQueueManager_Enqueue(&g_ble_tx_queue, &msg);
    
    #if defined(DEBUG_BLUETOOTH)
      if(enqueue_result == BLE_QUEUE_OK) {
        debug(F("[PACK-TX-Q] Message queued, depth="));
        debug(BLEQueueManager_GetCount(&g_ble_tx_queue));
        debug(F("/"));
        debug(BLE_QUEUE_SIZE);
        debugln(F(" msgs"));
      } else if(enqueue_result == BLE_QUEUE_FULL) {
        debug(F("[PACK-TX-Q] OVERFLOW: queue full, overflows="));
        debug(g_ble_tx_queue.overflowCount);
        debugln();
      }
    #endif
  } else {
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[PACK-TX-Q] Invalid packet: start=0x"));
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
 * Flush queued messages as BLE indications (one message per indication).
 * 
 * Phase 3 (Refactored): Dequeues and sends individual BLEMessages.
 * Per BLE_TRANSPORT.md: Each indication carries one complete message.
 * Confirmation/status handling deferred to integration layer.
 */
void bleFlushQueues() {
  if(!b_ble_enabled || !b_ble_connected || !g_pStatusCharacteristic) {
    return;  // BLE not ready, queues stay intact for next loop
  }
  
  // Process all queued messages (max 16 per queue per spec)
  while(!BLEQueueManager_IsEmpty(&g_ble_tx_queue)) {
    BLEMessage msg;
    uint8_t result = BLEQueueManager_Dequeue(&g_ble_tx_queue, &msg);
    
    if(result != BLE_QUEUE_OK) {
      break;  // Queue empty or error
    }
    
    // Send message as indication (message payload already has frame markers)
    // Metadata format: [7:6]=source, [5:0]=sequence
    uint8_t ble_buffer[258];  // 1 byte metadata + 257 (1B seq + 256B payload max)
    ble_buffer[0] = (0x00 << 6) | (msg.sequence & 0x3F);
    
    // Copy message payload (includes frame markers 0x02...0x04)
    memcpy(&ble_buffer[1], msg.payload, msg.length);
    
    g_pStatusCharacteristic->setValue(ble_buffer, msg.length + 1);
    g_pStatusCharacteristic->indicate();
    
    #if defined(DEBUG_BLUETOOTH)
      debug(F("[PACK-TX-SEND] Seq#"));
      debug(msg.sequence);
      debug(F(" PktType="));
      debug(msg.packetType);
      debug(F(" Len="));
      debug(msg.length);
      debug(F(" bytes Depth="));
      debug(BLEQueueManager_GetCount(&g_ble_tx_queue));
      debug(F("/"));
      debug(BLE_QUEUE_SIZE);
      debugln();
    #endif
  }
}
