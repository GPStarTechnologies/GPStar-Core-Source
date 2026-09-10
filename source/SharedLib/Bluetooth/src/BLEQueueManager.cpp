/**
 *   Bluetooth Manager - BLE Message Queue Manager Implementation
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

#include "BLEQueueManager.h"
#include "BLEConstants.h"
#include <string.h>

/*
 * Initialize a message queue to empty state.
 */
void BLEQueueManager_Init(BLEMessageQueue* queue) {
  if(!queue) return;
  
  queue->head = 0;
  queue->tail = 0;
  queue->count = 0;
  queue->overflowCount = 0;
  
  // Zero-initialize all message slots
  memset(queue->messages, 0, sizeof(queue->messages));
}

/*
 * Enqueue a message to the tail of the queue (FIFO).
 */
uint8_t BLEQueueManager_Enqueue(BLEMessageQueue* queue, const BLEMessage* msg) {
  if(!queue || !msg) return BLE_QUEUE_FULL;
  
  // Check if queue is full
  if(queue->count >= BLE_QUEUE_SIZE) {
    queue->overflowCount++;
    return BLE_QUEUE_FULL;
  }
  
  // Copy message to tail slot
  memcpy(&queue->messages[queue->tail], msg, sizeof(BLEMessage));
  
  // Advance tail index with wraparound
  queue->tail = (queue->tail + 1) % BLE_QUEUE_SIZE;
  
  // Increment message count
  queue->count++;
  
  return BLE_QUEUE_OK;
}

/*
 * Dequeue a message from the head of the queue (FIFO, destructive).
 */
uint8_t BLEQueueManager_Dequeue(BLEMessageQueue* queue, BLEMessage* msg) {
  if(!queue || !msg) return BLE_QUEUE_EMPTY;
  
  // Check if queue is empty
  if(queue->count == 0) {
    return BLE_QUEUE_EMPTY;
  }
  
  // Copy message from head slot
  memcpy(msg, &queue->messages[queue->head], sizeof(BLEMessage));
  
  // Advance head index with wraparound
  queue->head = (queue->head + 1) % BLE_QUEUE_SIZE;
  
  // Decrement message count
  queue->count--;
  
  return BLE_QUEUE_OK;
}

/*
 * Peek at the head message without removing it (non-destructive read).
 */
uint8_t BLEQueueManager_Peek(const BLEMessageQueue* queue, BLEMessage* msg) {
  if(!queue || !msg) return BLE_QUEUE_EMPTY;
  
  // Check if queue is empty
  if(queue->count == 0) {
    return BLE_QUEUE_EMPTY;
  }
  
  // Copy message from head slot (do not advance head or change count)
  memcpy(msg, &queue->messages[queue->head], sizeof(BLEMessage));
  
  return BLE_QUEUE_OK;
}

/*
 * Check if the queue is full.
 */
uint8_t BLEQueueManager_IsFull(const BLEMessageQueue* queue) {
  if(!queue) return 1;  // Null queue is treated as "full" (unusable)
  return (queue->count >= BLE_QUEUE_SIZE);
}

/*
 * Check if the queue is empty.
 */
uint8_t BLEQueueManager_IsEmpty(const BLEMessageQueue* queue) {
  if(!queue) return 1;
  return (queue->count == 0);
}

/*
 * Get the current number of messages in the queue.
 */
uint8_t BLEQueueManager_GetCount(const BLEMessageQueue* queue) {
  if(!queue) return 0;
  return queue->count;
}

/*
 * Clear the queue (reset to empty state).
 */
void BLEQueueManager_Clear(BLEMessageQueue* queue) {
  if(!queue) return;
  
  queue->head = 0;
  queue->tail = 0;
  queue->count = 0;
  // Note: overflowCount is NOT reset; it's diagnostic data
}
