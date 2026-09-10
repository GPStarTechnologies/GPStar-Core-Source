/**
 *   Bluetooth Manager - BLE Packet Parser
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
 *
 *   Purpose:
 *   --------
 *   Packet validation and parsing for BLE SerialData transport.
 *   Validates frame markers, determines packet type, and creates message envelopes.
 *   
 *   Per BLE_TRANSPORT.md rule #1: "BLE SerialData transports the existing logical 
 *   packet protocol; it does not redefine it."
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "BLEConstants.h"
#include "BLEMessage.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Validate that a payload has correct frame markers.
 * 
 * Args:
 *   payload - pointer to packet bytes
 *   length  - number of bytes in payload
 * 
 * Returns:
 *   BLE_QUEUE_OK      - payload has valid frame markers (0x02 start, 0x04 end)
 *   BLE_PACKET_INVALID - payload is too short or has invalid markers
 * 
 * Checks:
 *   - length >= 4 (minimum: start marker + 2 bytes + end marker)
 *   - payload[0] == BLE_FRAME_START_MARKER (0x02)
 *   - payload[length-1] == BLE_FRAME_END_MARKER (0x04)
 */
uint8_t BLEPacketParser_Validate(const uint8_t* payload, size_t length);

/*
 * Determine the packet type based on payload structure.
 * 
 * Args:
 *   payload - pointer to packet bytes (must pass Validate first)
 *   length  - number of bytes in payload
 * 
 * Returns:
 *   Packet type (PACKET_COMMAND, PACKET_DATA, PACKET_PACK, PACKET_WAND, 
 *   PACKET_SMOKE, PACKET_SYNC, etc.) from Communication.h PACKET_TYPE enum
 *   
 *   PACKET_UNKNOWN - if length is invalid or type cannot be determined
 * 
 * Logic:
 *   - 6 bytes: PACKET_COMMAND (s, cmd_hi, cmd_lo, d1_hi, d1_lo, e)
 *   - 7 bytes: PACKET_DATA (s, cmd_hi, cmd_lo, d[0], d[1], d[2], e)
 *   - Other lengths: determined by payload structure or PACKET_UNKNOWN
 */
uint8_t BLEPacketParser_GetPacketType(const uint8_t* payload, size_t length);

/*
 * Create a BLE message from a raw payload.
 * 
 * Args:
 *   packetType - the packet type (PACKET_COMMAND, PACKET_DATA, etc.)
 *   sequence   - sequence number (0-255) for this message
 *   payload    - pointer to packet bytes
 *   length     - number of bytes in payload
 *   msg        - pointer to BLEMessage output buffer (receives created message)
 * 
 * Returns:
 *   BLE_QUEUE_OK      - message successfully created
 *   BLE_PACKET_INVALID - payload is invalid; msg is not modified
 * 
 * Validates the payload, then populates all BLEMessage fields:
 *   - packetType from input
 *   - sequence from input
 *   - payload (memcpy'd from input)
 *   - length from input
 *   - status = BLE_MSG_STATUS_QUEUED
 */
uint8_t BLEPacketParser_CreateMessage(uint8_t packetType, uint8_t sequence, 
                                       const uint8_t* payload, size_t length, 
                                       BLEMessage* msg);

#ifdef __cplusplus
}
#endif
