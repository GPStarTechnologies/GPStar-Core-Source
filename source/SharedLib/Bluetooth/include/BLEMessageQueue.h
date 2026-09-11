/**
 *   Bluetooth Manager - BLE Message Queue Structure
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
 *   Defines a circular (ring) buffer queue for BLE messages.
 *   Per BLE_TRANSPORT.md: "Use fixed-size circular/ring buffers."
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "BLEConstants.h"
#include "BLEMessage.h"

/*
 * BLE Message Queue Structure
 * 
 * A circular/ring buffer queue holding up to BLE_QUEUE_SIZE (16) complete messages.
 * 
 * Fields:
 *   messages       - Fixed array of BLE_QUEUE_SIZE message slots
 *   head           - Index of next message to transmit (0 to BLE_QUEUE_SIZE-1)
 *   tail           - Index of next empty slot to write (0 to BLE_QUEUE_SIZE-1)
 *   count          - Current number of queued messages (0 to BLE_QUEUE_SIZE)
 *   overflowCount  - Diagnostic counter: number of enqueue attempts that failed due to full queue
 * 
 * Queue Discipline (FIFO):
 *   - Messages are added at tail, removed from head
 *   - When count < BLE_QUEUE_SIZE, enqueue succeeds and increments count
 *   - When count >= BLE_QUEUE_SIZE, enqueue fails, overflowCount increments
 *   - Dequeue removes head message and advances head index
 *   - Head and tail wrap around at BLE_QUEUE_SIZE (modulo arithmetic)
 * 
 * Per BLE_TRANSPORT.md rule #10: "Queue overflow must be detectable and must never 
 * silently discard an older queued message." The overflowCount field allows detection.
 */
struct BLEMessageQueue {
  BLEMessage messages[BLE_QUEUE_SIZE];    // Fixed circular buffer
  uint8_t head;                            // Index of next message to transmit
  uint8_t tail;                            // Index of next empty slot to write
  uint8_t count;                           // Current message count (0 to BLE_QUEUE_SIZE)
  uint16_t overflowCount;                  // Diagnostic: rejected enqueues
};
