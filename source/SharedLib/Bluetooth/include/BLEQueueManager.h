/**
 *   Bluetooth Manager - BLE Message Queue Manager
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
 *   Queue management functions for BLE message queues.
 *   Provides FIFO enqueue/dequeue, peek, and status checking.
 *   
 *   Per BLE_TRANSPORT.md rule #4: "One queue entry represents one complete logical message."
 *   Per BLE_TRANSPORT.md rule #7: "Only the current message is handed to the BLE transport; 
 *   subsequent messages wait their turn."
 */

#pragma once

#include <stdint.h>
#include "BLEMessage.h"
#include "BLEMessageQueue.h"

/*
 * Initialize a message queue to empty state.
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue to initialize
 * 
 * Sets head=0, tail=0, count=0, overflowCount=0.
 * Zeros all message slots.
 */
void BLEQueueManager_Init(BLEMessageQueue* queue);

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
 *   BLE_QUEUE_OK       - message successfully created
 *   BLE_PACKET_INVALID - payload is invalid; msg is not modified
 * 
 * Validates the payload, then populates all BLEMessage fields:
 *   - packetType from input
 *   - sequence from input
 *   - payload (memcpy'd from input)
 *   - length from input
 *   - status = BLE_MSG_STATUS_QUEUED
 */
uint8_t BLEQueueManager_CreateMessage(uint8_t packetType,
                                      uint8_t sequence, 
                                      const uint8_t* payload,
                                      size_t length, 
                                      BLEMessage* msg);

/*
 * Enqueue a message to the tail of the queue.
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 *   msg   - pointer to BLEMessage to enqueue
 * 
 * Returns:
 *   BLE_QUEUE_OK   - message successfully added
 *   BLE_QUEUE_FULL - queue is full (count >= BLE_QUEUE_SIZE); message NOT added
 * 
 * If the queue is full, the message is not added and overflowCount is incremented.
 * Per BLE_TRANSPORT.md rule #10: never silently overwrites an existing message.
 */
uint8_t BLEQueueManager_Enqueue(BLEMessageQueue* queue, const BLEMessage* msg);

/*
 * Dequeue a message from the head of the queue (FIFO, destructive).
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 *   msg   - pointer to BLEMessage output buffer (receives dequeued message)
 * 
 * Returns:
 *   BLE_QUEUE_OK    - message successfully dequeued
 *   BLE_QUEUE_EMPTY - queue is empty; msg is not modified
 * 
 * Removes the head message and advances the head index.
 * Per BLE_TRANSPORT.md rule #8: "A reliably transmitted message is removed from 
 * the TX queue only after the appropriate BLE completion/confirmation event."
 */
uint8_t BLEQueueManager_Dequeue(BLEMessageQueue* queue, BLEMessage* msg);

/*
 * Peek at the head message without removing it (non-destructive read).
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 *   msg   - pointer to BLEMessage output buffer (receives peeked message)
 * 
 * Returns:
 *   BLE_QUEUE_OK    - message successfully peeked
 *   BLE_QUEUE_EMPTY - queue is empty; msg is not modified
 * 
 * Reads the head message without advancing head or changing count.
 * Useful for inspecting the next message to send without committing to removal.
 */
uint8_t BLEQueueManager_Peek(const BLEMessageQueue* queue, BLEMessage* msg);

/*
 * Check if the queue is full.
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 * 
 * Returns:
 *   Non-zero if count >= BLE_QUEUE_SIZE, zero otherwise
 */
uint8_t BLEQueueManager_IsFull(const BLEMessageQueue* queue);

/*
 * Check if the queue is empty.
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 * 
 * Returns:
 *   Non-zero if count == 0, zero otherwise
 */
uint8_t BLEQueueManager_IsEmpty(const BLEMessageQueue* queue);

/*
 * Get the current number of messages in the queue.
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 * 
 * Returns:
 *   Current message count (0 to BLE_QUEUE_SIZE)
 */
uint8_t BLEQueueManager_GetCount(const BLEMessageQueue* queue);

/*
 * Clear the queue (reset to empty state).
 * 
 * Args:
 *   queue - pointer to BLEMessageQueue
 * 
 * Sets head=0, tail=0, count=0 (does not reset overflowCount).
 */
void BLEQueueManager_Clear(BLEMessageQueue* queue);
