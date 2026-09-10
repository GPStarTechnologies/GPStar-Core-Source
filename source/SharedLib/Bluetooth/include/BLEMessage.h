/**
 *   Bluetooth Manager - BLE Message Structure
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
 *   Defines a complete BLE message structure representing one logical packet
 *   to be transmitted or received via the SerialData transport.
 *   
 *   Per BLE_TRANSPORT.md rule #4: "One queue entry represents one complete logical message."
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "BLEConstants.h"

/*
 * BLE Message Structure
 * 
 * Represents a single complete message to be transmitted or received.
 * 
 * Fields:
 *   packetType  - Type of packet (PACKET_COMMAND, PACKET_DATA, PACKET_SYNC, etc.)
 *                 Comes from Communication.h PACKET_TYPE enum
 *   sequence    - Sequence number (0-255) identifying this message in the stream
 *                 Used to detect lost packets and order messages
 *   payload     - Raw serialized packet bytes (preserved exactly as produced for serial)
 *                 Includes frame markers (0x02 start, 0x04 end) and all command/data
 *   length      - Number of bytes in payload (typically 4-256)
 *   status      - Current state of this message (QUEUED, SENT, CONFIRMED)
 * 
 * Per BLE_TRANSPORT.md: "The payload is preserved exactly as produced for the existing 
 * serial transport. The BLE transport does not reinterpret or rebuild this payload. 
 * It wraps and transports it intact."
 */
struct BLEMessage {
  uint8_t packetType;                      // Packet type identifier
  uint8_t sequence;                        // Message sequence number (0-255)
  uint8_t payload[BLE_MESSAGE_PAYLOAD_SIZE]; // Raw serialized packet bytes
  size_t length;                           // Number of bytes in payload
  uint8_t status;                          // Current message status (BLE_MSG_STATUS_*)
};
