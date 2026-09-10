/*
 * BLE Library - Umbrella Header
 * 
 * This header provides the complete public interface for the Bluetooth
 * message queue and packet parsing functionality.
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

// Queue management and packet parsing functions
#include "BLEQueueManager.h"
#include "BLEPacketParser.h"

#endif  // BLE_H
