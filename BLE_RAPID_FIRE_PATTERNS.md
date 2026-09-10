# BLE Rapid-Fire Command Patterns (Corrected)

## Problem Summary
Multiple successive `wandSerialSend()` calls (not mixing with attenuator sends) in the same code block saturate the BLE write buffer, causing command loss. The Wand's BLE client cannot queue writes fast enough; commands after the first are dropped.

---

## CRITICAL: Wand → Pack Rapid-Fire (7 commands lost)

**Call Chain:**
```
loop()
  └─ checkWand()  
       └─ handleWandCommand(A_SYNC_END, ...)  [in NeutronaWand/Serial.h:883]
            └─ case A_SYNC_END: (lines 889-931)
                 ├─ packSerialSend(A_SYNCHRONIZED);           // CMD 8
                 ├─ packSerialSend(A_WAND_AUDIO_VERSION, ...);// CMD 69
                 ├─ packSerialSend(A_STREAM_FLAGS, ...);      // CMD 19
                 ├─ if(condition) packSerialSend(A_SET_FIRING_MODE, ...); // CMD 272 or 247
                 ├─ packSerialSend(A_SET_PROTON_STREAM_IMPACT, ...);     // CMD 265
                 └─ packSerialSend(A_BARREL_EXTENDED/RETRACTED, ...);   // CMD 90
```

**File:** [NeutronaWand/include/Serial.h](NeutronaWand/include/Serial.h)  
**Location:** Lines 889-931 (in `handleWandCommand()`)  
**Trigger:** Wand receives `A_SYNC_END` packet from Pack during sync completion  
**Impact:** Pack (endpoint) receives only first command (CMD 8 = A_SYNCHRONIZED); commands 69, 19, 272/247, 265, 90 are dropped  
**Priority:** **CRITICAL** - This is the observed 7-command loss in logs

**Why it happens:**
- When Wand finishes sync, it immediately fires 6 rapid `packSerialSend()` calls
- Each write is queued to the BLE write buffer
- Pack's NimBLE server can only read so fast; buffer overflows
- Subsequent writes dropped before they reach the pack

---

## HIGH: Pack → Wand Rapid-Fire (A_PACK_ON → A_ION_ARM_SWITCH_ON)

**Call Chain:**
```
loop()
  └─ packStartup() [in ProtonPack/include/System.h:1167]
       └─ (end of function, line 1322)
            └─ wandSerialSend(A_PACK_ON);                // CMD 10
            
loop() [next iteration or same block]
  └─ checkSwitches()  [in ProtonPack/include/System.h:1935]
       └─ if(switch_power.isPressed()/isReleased()) (line 2176)
            └─ if(switch_power.getState() == LOW) (line 2198)
                 └─ wandSerialSend(A_ION_ARM_SWITCH_ON);  // CMD 128
```

**File:** [ProtonPack/include/System.h](ProtonPack/include/System.h)  
**Location:** Lines 1322 (A_PACK_ON) and 2207 (A_ION_ARM_SWITCH_ON)  
**Trigger:** Pack powers on, followed by ion arm switch state evaluation  
**Commands Sent in Rapid Succession:** 
  1. A_PACK_ON (CMD 10) - line 1322 in `packStartup()`
  2. A_ION_ARM_SWITCH_ON (CMD 128) - line 2207 in `checkSwitches()`

**Current Observed Behavior (13:23:42.107 from logs):**
```
Web: Turn Pack On
[PACK-TX] CMD 128 VAL 0
[PACK-TX] CMD 10 VAL 0
```
Both commands transmitted from Pack to Wand in same loop cycle. Need verification that both arrive at Wand side and are processed correctly.

**Priority:** MEDIUM-HIGH (occurs every Pack power-up)

---

## Rapid-Fire Patterns Identified

### Pattern 1: Wand Sync Data Transmission
**File:** [ProtonPack/include/Serial.h](ProtonPack/include/Serial.h)  
**Location:** Lines 1200-1208  
**Function:** `doWandSync()` → `wandSerialSend()` calls  
**Call Chain:**
```
loop()
  └─ checkWand()
       └─ handleWandPacket(PACKET_COMMAND)
            └─ handleWandCommand(A_SYNC_WAND, ...)
                 └─ doWandSync() [Serial.h:1133]
                      ├─ wandSerialSend(A_SYNC_START, signature);
                      ├─ wandSerialSend(A_SYNC_DATA);
                      ├─ if(alarm) wandSerialSend(A_ALARM_ON, ...);
                      └─ wandSerialSend(A_SYNC_END);
```

**Status:** Pack sends 3-4 commands in sequence during sync initialization  
**Impact:** Wand may miss ALARM_ON or SYNC_END  
**Priority:** LOW (fires only during initial sync, and Wand is idle listening)
