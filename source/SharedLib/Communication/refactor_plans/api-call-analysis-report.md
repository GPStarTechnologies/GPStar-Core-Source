# API Call Analysis Report - Commands with Non-Zero d1 Parameters

## Summary Statistics
- **Total Function Calls Analyzed**: 165+ occurrences
- **Unique API_COMMAND Entries Used**: 67+ with meaningful d1 values
- **Files Scanned**: 12 header files across ProtonPack, NeutronaWand, Attenuator
- **Total Lines Analyzed**: ~10,000+ lines of code

---

## Critical Finding: Commands Used BOTH Ways

Many API_COMMAND entries are being called with AND without d1 parameters:

### Pattern 1: Boolean/State Toggle Commands (1 or 2)
These are used to indicate state changes where d1=1 means "OFF" and d1=2 means "ON":
- `A_TOGGLE_SMOKE` - d1: 1=OFF, 2=ON
- `A_TOGGLE_VIBRATION` - d1: 1=OFF, 2=ON  
- `A_CYCLOTRON_DIRECTION_TOGGLE` - d1: 1/2 state
- `A_TOGGLE_MUTE` - d1: 1=OFF, 2=ON
- `A_MUSIC_TRACK_LOOP_TOGGLE` - d1: 1=OFF, 2=ON
- `A_MUSIC_TRACK_SHUFFLE_TOGGLE` - d1: 1=OFF, 2=ON

### Pattern 2: Set/Configure Commands
These carry meaningful configuration data:
- `A_SET_STREAM_MODE` - d1: streaming mode byte (from function call)
- `A_SET_FIRING_MODE` - d1: FLAG_VG_MODE, FLAG_CTS_MODE, FLAG_CTS_MIX_MODE
- `A_SET_POWER_LEVEL` - d1: 1-5 (power levels)
- `A_SET_BARREL_TYPE` - d1: 1-5 (barrel types)
- `A_SET_CYCLOTRON_LED_COUNT` - d1: specific count or 2=toggle
- `A_SET_POWERCELL_LED_COUNT` - d1: specific count or 2=toggle
- `A_SET_INNER_CYCLOTRON_LED_COUNT` - d1: specific count or 2=toggle
- `A_SET_VIBRATION_MODE` - d1: 1-6 (ALWAYS, FIRING_ONLY, NEVER, DEFAULT, CYCLE, etc.)
- `A_SET_WAND_GPSTAR_AUDIO_LED` - d1: 0/1
- `A_SET_WAND_VIBRATION_MODE` - d1: vibration mode
- `A_SET_PACK_VIBRATION_MODE` - d1: vibration mode
- `A_SET_DEMO_LIGHT_MODE` - d1: 0/1/2
- `A_SET_OVERHEAT_STROBE` - d1: 0/1/2
- `A_SET_OVERHEAT_LIGHTS_OFF` - d1: 0/1/2
- `A_SET_OVERHEAT_SYNC_FAN` - d1: 0/1/2
- `A_SET_PACK_GPSTAR_AUDIO_LED` - d1: 0/1
- `A_SET_QUICK_BOOTUP` - d1: 0/1/2
- `A_SET_PROTON_STREAM_IMPACT` - d1: state value

### Pattern 3: Handshake/Sync Commands (PROTOCOL_SIGNATURE)
- `A_HANDSHAKE` - d1: PROTOCOL_SIGNATURE (version verification)
- `A_SYNC_START` - d1: PROTOCOL_SIGNATURE or post-finish state (1/2)
- `A_SYNC_NOW` - d1: PROTOCOL_SIGNATURE

### Pattern 4: Status/Data Commands
- `A_MUSIC_IS_PLAYING` - d1: current track number
- `A_MUSIC_IS_NOT_PLAYING` - d1: current track number
- `A_MUSIC_PLAY_TRACK` - d1: track number to play
- `A_MUSIC_STATUS` - d1: play state (1/2/3/4)
- `A_MUSIC_LOOP_STATUS` - d1: loop state (1/2)
- `A_MUSIC_SHUFFLE_STATUS` - d1: shuffle state (1/2)
- `A_MASTER_AUDIO_STATUS` - d1: mute state (1/2)
- `A_BATTERY_VOLTAGE_PACK` - d1: voltage * 100 (uint16_t)
- `A_TEMPERATURE_PACK` - d1: temperature_c * 100
- `A_WAND_POWER_AMPS` - d1: amperage value
- `A_STREAM_FLAGS` - d1: stream options byte
- `A_ALARM_ON` - d1: ribbon cable state (0/1)
- `A_ALARM_OFF` - d1: ribbon cable state (0/1)
- `A_PACK_OFF` - d1: shutdown state (0/1)
- `A_WAND_ON` - d1: mode-dependent state (0/1)
- `A_WAND_AUDIO_VERSION` - d1: version number
- `A_VOLUME_SET` - d1: volume value
- `A_SYSTEM_LOCKOUT` - d1: timeout value
- `A_BUTTON_MASHING` - d1: timeout value
- `A_FIRING` - d1: firing mode state (1/2)
- `A_IMPACT_SOUND` - d1: sound effect ID
- `A_COM_SOUND_NUMBER` - d1: sound number

---

## Recommendations for Enum Reorganization

### Commands That MUST Move to API_DATA
These should NEVER be called without a meaningful d1 value:
1. `A_HANDSHAKE` - always needs PROTOCOL_SIGNATURE
2. `A_SET_STREAM_MODE` - always needs mode byte
3. `A_SET_FIRING_MODE` - always needs mode flag
4. `A_SET_POWER_LEVEL` - always needs level (1-5)
5. `A_SET_BARREL_TYPE` - always needs type (1-5)
6. `A_SET_CYCLOTRON_LED_COUNT` - always needs count
7. `A_SET_POWERCELL_LED_COUNT` - always needs count
8. `A_SET_INNER_CYCLOTRON_LED_COUNT` - always needs count
9. `A_SET_VIBRATION_MODE` - always needs mode (1-6)

### Commands That Are HYBRID (Called Both Ways)
These can work either way and may need both single & dual signatures:
- `A_VOLUME_SET` - requires value
- `A_MUSIC_PLAY_TRACK` - requires track number
- `A_ALARM_ON` / `A_ALARM_OFF` - can work with/without ribbon state

### Commands That Are PURE Commands (No d1 Needed)
These should stay in API_COMMAND (called only with 0):
- `A_PACK_ON`, `A_PACK_OFF` - basic on/off (though sometimes called with state)
- `A_WAND_ON`, `A_WAND_OFF` - basic on/off
- `A_FIRING`, `A_FIRING_STOPPED` - state transitions
- `A_VENTING`, `A_OVERHEATING` - state indicators

---

## API Call Patterns Found

### High-Frequency Patterns:
1. **Protocol Signature**: Used in ~4 places (HANDSHAKE, SYNC_START, SYNC_NOW)
2. **Toggle Operations**: Used in ~30 places with 1/2 values
3. **Configuration Modes**: Used in ~50 places with specific enums/values
4. **Status Updates**: Used in ~30 places with variable data
5. **Direct Values**: Used in ~50 places with constants

---

## Data Types Used in d1 Parameter:
- **Constants**: PROTOCOL_SIGNATURE, FLAG_* enums, numeric literals (1-5)
- **Variables**: Booleans cast to ternary expressions (1 or 2)
- **Function Calls**: `getStreamModeByte()`, `getStreamModeOpts()`, `ribbonCableAttached()`
- **Calculated Values**: voltage*100, temperature_c*100, track numbers
- **Timeout Values**: Custom timeout durations

---

## Enum Consolidation Strategy

### Immediate Action Items:
1. Audit all API_COMMAND calls that use non-zero d1
2. Create corresponding API_DATA entries for mandatory-data commands
3. Update function signatures where needed to clarify data requirements
4. Add comments to API_COMMAND entries indicating if/how they use d1
5. Consider namespace splitting if enum continues to grow (currently ~300 entries)

### Future Consideration:
- The API_COMMAND enum is already at 300+ entries
- Consider splitting into multiple enums by device/functionality
- Consider a registry pattern that maps command to expected data format
