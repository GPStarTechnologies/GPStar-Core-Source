/*
 * BLE Library - Umbrella Header
 * 
 * This header provides the complete public interface for the Bluetooth
 * message queue and packet creation functionality.
 * 
 * Usage:
 *   #include <BLE.h>  // Automatically includes all BLE components
 */

#ifndef BLE_H
#define BLE_H

// Core data structures and constants
#include "BLEConstants.h"
#include "BLEMessage.h"
#include "BLEMessageQueue.h"

// Queue management functions
#include "BLEQueueManager.h"

#endif  // BLE_H
