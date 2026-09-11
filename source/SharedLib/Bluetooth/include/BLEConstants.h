/**
 *   Bluetooth Manager - BLE Constants and Definitions
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
 *   Centralized definitions for BLE SerialData transport.
 *   References BLE_TRANSPORT.md architecture specification.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 * BLE SerialData Service and Characteristic UUIDs
 * 
 * These UUIDs are HARDCODED per BLE_TRANSPORT.md specification.
 * Pack advertises the SerialData service with these characteristics:
 * - PackTX: Pack → Wand (uses indications, reliable)
 * - PackRX: Wand → Pack (write target)
 */
#define BLE_SERIALDATA_SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define BLE_SERIALDATA_PACKTX_CHAR_UUID    "0000ffe1-0000-1000-8000-00805f9b34fb"
#define BLE_SERIALDATA_PACKRX_CHAR_UUID    "0000ffe2-0000-1000-8000-00805f9b34fb"

/*
 * BLE Message Queue Configuration
 * 
 * Per BLE_TRANSPORT.md:
 * "Use fixed-size circular/ring buffers. Initial size: TX queue: 16 messages, RX queue: 16 messages"
 */
#define BLE_QUEUE_SIZE                     16
#define BLE_MESSAGE_PAYLOAD_SIZE           256

/*
 * BLE Message Status Codes
 * 
 * Represents the lifecycle state of a queued message.
 * Per BLE_TRANSPORT.md rule #8: "A reliably transmitted message is removed from the TX queue 
 * only after the appropriate BLE completion/confirmation event."
 */
#define BLE_MSG_STATUS_QUEUED              0
#define BLE_MSG_STATUS_SENT                1
#define BLE_MSG_STATUS_CONFIRMED           2

/*
 * BLE Operation Result Codes
 * 
 * Returned by queue and parser functions to indicate success or failure.
 */
#define BLE_QUEUE_OK                       0
#define BLE_QUEUE_FULL                     1
#define BLE_QUEUE_EMPTY                    2
#define BLE_PACKET_INVALID                 3
