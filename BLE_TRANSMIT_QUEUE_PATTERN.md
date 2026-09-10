# BLE Serial Command Queue - Implementation Guide

## Problem
Multiple `wandSerialSend()` calls in same loop cycle overflow BLE write buffer. Only first command arrives at Wand.

**Examples:**
- A_SYNC_END: 6 commands sent rapidly → only first arrives
- A_PACK_ON + A_ION_ARM_SWITCH_ON: sent back-to-back → only first arrives

## Solution
Queue all serial frames per loop, send once at end of loop.

---

## Data Format (The Wire)

Queued data looks like this:

```
[METADATA][PAYLOAD]
```

**METADATA (1 byte):**
```
Bits [7:6] = Data Source
  00 = Legacy serial (PACKET_COMMAND, PACKET_DATA, PACKET_WAND, PACKET_SMOKE)
  01 = Reserved for future use
  10 = Reserved for future use
  11 = Reserved for future use

Bits [5:0] = Sequence counter (0-63)
```

**PAYLOAD:** One or more frames concatenated  
- For serial: `[START][...][END][START][...][END]...` (repeating)
- For future types: format TBD when we add them

**Examples:**

Serial frames (legacy):
```
[0x00] [0xFF CMD1_DATA 0xFE] [0xFF CMD2_DATA 0xFE] [0xFF CMD3_DATA 0xFE]
 ↑
 Bits: 00xxxxxx = "this is legacy serial"
```

Future non-serial (if we add it):
```
[0x40] [PROP_ID VALUE...] [SENSOR_TYPE READING...]
 ↑
 Bits: 01xxxxxx = "this is type 1 (unknown what yet)"
```

**Why this works:**
- Wand sees first byte, checks top 2 bits
- "00" = parse frames until end of buffer (no markers needed)
- "01" = "dunno this type, log it, ignore it" (future-proof)
- Sequence counter detects loss regardless of type

---

## Three Functions

### 1. bleProcessData()
**When:** Start of loop()  
**What:** Check if BLE data arrived, route serial frames to queue

```cpp
void bleProcessData() {
  // Check if we have incoming BLE data with frame markers [START]...[END]
  if(b_ble_rx_ready && g_ble_rx_length > 0) {
    // Copy to serial RX buffer for processing
    memcpy(g_serial_rx_buffer, g_ble_rx_buffer, g_ble_rx_length);
    g_serial_rx_length = g_ble_rx_length;
    b_serial_rx_ready = true;
    b_ble_rx_ready = false;
  }
}
```

### 2. bleApplySerialData()
**When:** In checkWand() if UART not available  
**What:** Process queued serial frame from BLE (same as UART path)

```cpp
void bleApplySerialData() {
  // Only process if we have data
  if(!b_serial_rx_ready || g_serial_rx_length == 0) {
    return;
  }
  
  // Parse and handle - IDENTICAL to UART path
  BLEPacket packet = bleHandleData(g_serial_rx_buffer, g_serial_rx_length);
  if(packet.packetType > 0) {
    handleWandPacket(packet.packetType);  // Same dispatcher as UART
  }
  
  // Clear
  b_serial_rx_ready = false;
  g_serial_rx_length = 0;
}
```

### 3. bleFlushQueues()
**When:** End of loop() (very last thing)  
**What:** Send all queued frames as single indication

```cpp
void bleFlushQueues() {
  // Only send if we have data and BLE is ready
  if(g_serial_tx_queue_length == 0 || !b_ble_connected) {
    return;
  }
  
  // Prepend metadata byte: 00xxxxxx (legacy serial) + sequence counter
  uint8_t ble_buffer[257];
  ble_buffer[0] = (0x00 << 6) | (g_ble_tx_sequence & 0x3F);
  g_ble_tx_sequence++;
  memcpy(&ble_buffer[1], g_serial_tx_queue, g_serial_tx_queue_length);
  
  // Send as one indication - blocks until Wand ACKs
  g_pStatusCharacteristic->setValue(ble_buffer, g_serial_tx_queue_length + 1);
  g_pStatusCharacteristic->indicate();
  
  // Clear queue for next loop
  g_serial_tx_queue_length = 0;
}
```

---

## Queue Variables (Bluetooth.h)

```cpp
#define SERIAL_TX_QUEUE_SIZE 256
uint8_t g_serial_tx_queue[SERIAL_TX_QUEUE_SIZE] = {0};
size_t g_serial_tx_queue_length = 0;

uint8_t g_ble_tx_sequence = 0;  // 6-bit sequence counter (0-63)

// For receiving BLE frames
uint8_t g_serial_rx_buffer[32] = {0};
size_t g_serial_rx_length = 0;
bool b_serial_rx_ready = false;
```

---

## Changes to wandSerialSend() (Serial.h)

**OLD:**
```cpp
wandSerialSend(CMD) 
  → bleSendData()      // Send immediately
     → indicate()      // Blocks here
```

**NEW:**
```cpp
wandSerialSend(CMD) 
  → bleQueueSerialData(frame)  // Append to queue
     → memcpy to g_serial_tx_queue
     → return immediately (no blocking)
```

---

## Loop Structure (main.cpp)

```cpp
void loop() {
  // PHASE 1: Receive
  bleProcessData();           // Move BLE data to queue if available
  
  // PHASE 2: Execute (unchanged)
  checkWand();                // UART first, fallback to BLE queue
  checkSwitches();
  checkAnimation();
  // ... rest of loop ...
  
  // PHASE 3: Flush (LAST)
  bleFlushQueues();           // Send ALL queued frames now
}
```

---

## checkWand() Logic

```cpp
void checkWand() {
  // UART first (existing behavior)
  if(WAND_CONN_STATE == WAND_CONNECTED) {
    // Process UART as always
    return;
  }
  
  // Fallback: BLE queue
  bleApplySerialData();
}
```

---

## That's it.

Just:
1. Tag data with metadata byte (top 2 bits = type, bottom 6 bits = sequence)
2. Queue incoming BLE serial frames
3. Prioritize UART, fall back to BLE
4. Queue outgoing frames
5. Flush once per loop with metadata prepended

Wand receives: `[METADATA][FRAMES...]` and knows what it got.
Future types: Just use different top bits, don't have to redesign anything.
