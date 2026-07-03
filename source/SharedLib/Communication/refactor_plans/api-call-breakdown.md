# API Command Usage Pattern Breakdown

## Distribution by Pattern Type

### Pattern Type: CONSTANT (specific literal values)
Total Occurrences: ~90
Examples:
- Power levels: 1, 2, 3, 4, 5
- Barrel types: 1, 2, 3, 4, 5
- Toggle states: 1, 2
- Configuration modes: 0, 1, 2

### Pattern Type: VARIABLE (ternary or boolean expression)
Total Occurrences: ~50
Examples:
- `b_smoke_enabled ? 2 : 1`
- `ribbonCableAttached() ? 1 : 0`
- `b_pack_shutting_down ? 1 : 0`
- `gpstarWand.isFiringModeCTSMix() ? FLAG_CTS_MIX_MODE : FLAG_CTS_MODE`

### Pattern Type: FUNCTION_CALL (method call returning value)
Total Occurrences: ~20
Examples:
- `gpstarWand.getStreamModeByte()`
- `gpstarPack.getStreamModeByte()`
- `gpstarWand.getStreamModeOpts()`
- `ribbonCableAttached()`

### Pattern Type: CALCULATED_VALUE (expression/formula)
Total Occurrences: ~8
Examples:
- `f_batt_volts * 100` (voltage calculation)
- `f_temperature_c * 100` (temperature calculation)

---

## Distribution by Device/Function

### packSerialSend() Calls with d1:
**Destination**: ProtonPack
**Count**: ~55 unique calls
**Common Patterns**:
- Music status (playing, paused, loop, shuffle): 8 calls
- Vibration modes (wand & pack): 12 calls
- Demo/overheat/bootup features: 8 calls
- Stream modes: 4 calls
- Firing modes: 2 calls
- Cyclotron simulation: 2 calls
- Audio LED status: 2 calls

### wandSerialSend() Calls with d1:
**Destination**: NeutronaWand  
**Count**: ~60 unique calls
**Common Patterns**:
- Stream mode/flags: 6 calls
- Firing mode: 4 calls
- Set functions (barrel, LED counts, vibration): 15 calls
- Configuration toggles: 5 calls
- Feature toggles: 4 calls
- Special functions (button mashing, impact sound): 2 calls

### attenuatorSerialSend() Calls with d1:
**Destination**: Attenuator
**Count**: ~50 unique calls
**Common Patterns**:
- Set power level: 9 calls
- Set stream mode: 4 calls
- Toggle commands (smoke, vibration, etc.): 12 calls
- Music/audio status: 8 calls
- Alarm states: 10 calls
- Battery/temperature data: 2 calls
- Vibration mode: 5 calls

---

## Device-to-Device Communication Matrix

```
ProtonPack                        NeutronaWand
    |                                  |
    |-- packSerialSend() -->          |
    |                                  | (UI/Button driven)
    |                                  |
    |<-- wandSerialSend() --          |
    |                                  |
    | (Serial comms)                  |
    |                                  |
    v                                  |
Attenuator <------ attenuatorSerialSend() ------
    (Display & Control Hub)
```

**Pack → Attenuator**: Power levels, stream modes, music status, alarm states, vibration modes, demo lights
**Wand → Attenuator**: (via Pack relay) Stream flags, firing mode, audio version
**Wand → Pack**: Firing mode, stream flags, audio version, button mashing, impact sounds
**Pack → Wand**: Vibration modes, stream modes (config)
**Attenuator → Pack**: (Command responses handled via command structure)

---

## Command Categories with Data Requirements

### 1. Configuration Commands (require specific value):
- A_SET_STREAM_MODE (stream byte)
- A_SET_FIRING_MODE (mode flag)
- A_SET_POWER_LEVEL (1-5)
- A_SET_BARREL_TYPE (1-5)
- A_SET_*_LED_COUNT (count or 2)
- A_SET_VIBRATION_MODE (1-6)
- A_SET_QUICK_BOOTUP (0/1/2)
- A_SET_DEMO_LIGHT_MODE (0/1/2)
- A_SET_OVERHEAT_STROBE (0/1/2)
- A_SET_OVERHEAT_LIGHTS_OFF (0/1/2)
- A_SET_OVERHEAT_SYNC_FAN (0/1/2)
- A_SET_PACK_GPSTAR_AUDIO_LED (0/1)
- A_SET_WAND_GPSTAR_AUDIO_LED (0/1)
- A_SET_PROTON_STREAM_IMPACT (state value)

### 2. Status Commands (carry state/data):
- A_MUSIC_IS_PLAYING (track#)
- A_MUSIC_IS_NOT_PLAYING (track#)
- A_MUSIC_PLAY_TRACK (track#)
- A_MUSIC_STATUS (play state: 1/2/3/4)
- A_MUSIC_LOOP_STATUS (1/2)
- A_MUSIC_SHUFFLE_STATUS (1/2)
- A_MASTER_AUDIO_STATUS (mute: 1/2)
- A_STREAM_FLAGS (options byte)
- A_ALARM_ON (ribbon state: 0/1)
- A_ALARM_OFF (ribbon state: 0/1)
- A_PACK_OFF (shutdown: 0/1)
- A_WAND_ON (mode state: 0/1)
- A_WAND_AUDIO_VERSION (version#)
- A_BATTERY_VOLTAGE_PACK (voltage*100)
- A_TEMPERATURE_PACK (temp*100)
- A_WAND_POWER_AMPS (amperage)
- A_VOLUME_SET (volume)
- A_SYSTEM_LOCKOUT (timeout)

### 3. Toggle Commands (1=OFF, 2=ON):
- A_TOGGLE_SMOKE (1/2)
- A_TOGGLE_VIBRATION (1/2)
- A_CYCLOTRON_DIRECTION_TOGGLE (1/2)
- A_TOGGLE_MUTE (1/2)
- A_MUSIC_TRACK_LOOP_TOGGLE (1/2)
- A_MUSIC_TRACK_SHUFFLE_TOGGLE (1/2)

### 4. Protocol Commands (PROTOCOL_SIGNATURE):
- A_HANDSHAKE (signature)
- A_SYNC_START (signature or state)
- A_SYNC_NOW (signature)

### 5. Event Commands (special values):
- A_BUTTON_MASHING (timeout)
- A_FIRING (mode: 1/2)
- A_IMPACT_SOUND (effect#)
- A_COM_SOUND_NUMBER (sound#)

---

## Recommendations Priority

### HIGH (Do First - Clear Data Requirement):
1. A_HANDSHAKE - always needs PROTOCOL_SIGNATURE (security/versioning)
2. A_SET_STREAM_MODE - always needs mode byte (configuration critical)
3. A_SET_FIRING_MODE - always needs mode (affects device behavior)
4. A_SET_POWER_LEVEL - always needs level (power control critical)

### MEDIUM (Should Do - Improves API Clarity):
1. A_SET_BARREL_TYPE - always needs barrel ID
2. A_SET_*_LED_COUNT - always needs count value
3. A_SET_VIBRATION_MODE - always needs mode
4. A_MUSIC_PLAY_TRACK - always needs track number

### LOW (Nice to Have - Consistency):
1. Split toggle commands into separate SET commands
2. Create explicit status report commands
3. Add explicit data wrapper commands for calculated values

---

## Files to Review/Modify

### Communication Layer:
- `/source/SharedLib/Communication/include/Communication.h` - Enum definitions

### Pack Implementation:
- `/source/ProtonPack/include/Serial.h` - Main serial send implementation
- `/source/ProtonPack/include/Command.h` - Command handlers
- `/source/ProtonPack/include/Audio.h` - Music/audio commands

### Wand Implementation:
- `/source/NeutronaWand/include/Serial.h` - Wand serial send
- `/source/NeutronaWand/include/Command.h` - Command handlers
- `/source/NeutronaWand/include/Actions.h` - Button/action handlers
- `/source/NeutronaWand/include/System.h` - System commands

### Attenuator Implementation:
- `/source/Attenuator/include/Serial.h` - Attenuator serial send
- `/source/Attenuator/include/Webhandler.h` - Web UI commands
- `/source/Attenuator/include/System.h` - System commands

---

## Summary
- **165 non-zero d1 parameter calls identified**
- **67+ unique API_COMMAND entries using meaningful data**
- **Clear patterns showing data requirements**
- **Multiple candidates for API_DATA enum migration**
- **API_COMMAND enum at ~300 entries (near limit)**

This analysis shows there's significant opportunity to improve the API design by:
1. Moving mandatory-data commands to API_DATA
2. Creating clear separation between stateless commands and data-carrying commands
3. Improving code clarity and reducing enum pollution
