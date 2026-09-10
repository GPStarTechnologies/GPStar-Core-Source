/**
 *   Bluetooth Manager - BLE Packet Parser Implementation
 *   GPStar Shared Library
 *
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
 */

#include "BLEPacketParser.h"
#include "BLEConstants.h"
#include "../../Communication/include/Communication.h"
#include <string.h>

/*
 * Validate that a payload has correct frame markers.
 */
uint8_t BLEPacketParser_Validate(const uint8_t* payload, size_t length) {
  if(!payload) return BLE_PACKET_INVALID;
  
  // Minimum: start marker + end marker + 2 data bytes
  if(length < 4) {
    return BLE_PACKET_INVALID;
  }
  
  // Check start marker (0x02 = A_COM_START)
  if(payload[0] != BLE_FRAME_START_MARKER) {
    return BLE_PACKET_INVALID;
  }
  
  // Check end marker (0x04 = A_COM_END)
  if(payload[length - 1] != BLE_FRAME_END_MARKER) {
    return BLE_PACKET_INVALID;
  }
  
  return BLE_QUEUE_OK;
}

/*
 * Determine the packet type based on payload structure.
 * 
 * Packet structures from Communication.h:
 *   PACKET_COMMAND: 6 bytes (s=0x02, cmd_hi, cmd_lo, d1_hi, d1_lo, e=0x04)
 *   PACKET_DATA:    7 bytes (s=0x02, cmd_hi, cmd_lo, d[0], d[1], d[2], e=0x04)
 *   PACKET_PACK:    32 bytes (26 data + frame markers)
 *   PACKET_WAND:    22 bytes (16 data + frame markers)
 *   PACKET_SMOKE:   18 bytes (12 data + frame markers)
 *   PACKET_SYNC:    15 bytes (9 data + frame markers)
 */
uint8_t BLEPacketParser_GetPacketType(const uint8_t* payload, size_t length) {
  if(!payload || length < 4) {
    return PACKET_UNKNOWN;
  }
  
  // Determine type based on payload length
  // These sizes match the packet structs in Communication.h
  switch(length) {
    case 6:
      // CommandPacket: s + c(2) + d1(2) + e
      return PACKET_COMMAND;
      
    case 7:
      // DataPacket: s + c(2) + d[3] + e
      return PACKET_DATA;
      
    case 22:
      // WandPrefs: s + 16 data bytes + e
      return PACKET_WAND;
      
    case 18:
      // SmokePrefs: s + 12 data bytes + e
      return PACKET_SMOKE;
      
    case 15:
      // WandSyncData: s + 9 data bytes + e
      return PACKET_SYNC;
      
    case 32:
      // PackPrefs: s + 26 data bytes + e
      return PACKET_PACK;
      
    default:
      // Unknown packet type
      return PACKET_UNKNOWN;
  }
}

/*
 * Create a BLE message from a raw payload.
 */
uint8_t BLEPacketParser_CreateMessage(uint8_t packetType, uint8_t sequence, 
                                       const uint8_t* payload, size_t length, 
                                       BLEMessage* msg) {
  if(!payload || !msg) {
    return BLE_PACKET_INVALID;
  }
  
  // Validate the payload structure
  if(BLEPacketParser_Validate(payload, length) != BLE_QUEUE_OK) {
    return BLE_PACKET_INVALID;
  }
  
  // Verify packet type is not unknown
  if(packetType == PACKET_UNKNOWN) {
    return BLE_PACKET_INVALID;
  }
  
  // Check payload size fits in message buffer
  if(length > BLE_MESSAGE_PAYLOAD_SIZE) {
    return BLE_PACKET_INVALID;
  }
  
  // Populate message structure
  msg->packetType = packetType;
  msg->sequence = sequence;
  msg->length = length;
  msg->status = BLE_MSG_STATUS_QUEUED;
  
  // Copy payload bytes
  memcpy(msg->payload, payload, length);
  
  return BLE_QUEUE_OK;
}
