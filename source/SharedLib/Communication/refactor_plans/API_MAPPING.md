# API-First Message Alignment

## Problem Statement

API commands have grown organically from the start of the project, primarily built around the Proton Pack and Neutrona Wand communication. Eventually a new set of commands was created for the Attenuator, though these now overlap with a majority of the commands already in existence. There is a need to identify and consolidate the API calls to eliminate redundancies and ensure that a singular set of commands exists to cater to these 3 connected devices.

## Proposed Solution

By mapping out the total listing of API commands as found in the file `source/SharedLib/Communication/include/Communication.h` it should be possible to utilize the `A_`-prefixed commands as the source of truth and either replace unique `P_`-prefixed or `W_`-prefixed commands by creating new `A_`-prefixed commands.

One more potential area for cleanup: condensing commands which exist for distinct tasks by using a single command enum value with a `uint16_t` parameter. The parameter is sent via the `*SerialSend(command, uint16_t_value)` function on each device, which encodes the value in the `d1` field of the CommandPacket (PACKET_COMMAND envelope). The reason for this is that we should keep to a single list of API commands which must fit within a `uint8_t` data type, so that we maintain a maximum of 254 potential values.

Examples of possible consolidations:

- There are commands `A_ION_ARM_SWITCH_ON` and `A_ION_ARM_SWITCH_OFF` which could be condensed to just `A_ION_ARM_SWITCH_STATE` and pass a 1 or 0 to indicate the ON/OFF state.
- Similarly, there are commands `A_POWER_LEVEL_1` and `W_POWER_LEVEL_1` which could be instead represented as just `A_SET_POWER_LEVEL` which accepts a numeric value 1-5.
- We also have P_PACK_GPSTAR_AUDIO_LED_DISABLED/P_PACK_GPSTAR_AUDIO_LED_ENABLED and W_WAND_GPSTAR_AUDIO_LED_DISABLED/W_WAND_GPSTAR_AUDIO_LED_ENABLED but these just serve the same purpose (to toggle the LED preference for the GPStar Audio device) but respective to the pack or wand devices overall. Ideally these could be A_PACK_GPSTAR_AUDIO_LED and A_WAND_GPSTAR_AUDIO_LED with a 1/0 data value passed.

For purposes of consolidation we should consider the ENUMs defined within the files at `source/SharedLib/DeviceState/include/Streams.h`, `source/SharedLib/DeviceState/include/Themes.h`, and `source/SharedLib/DeviceState/include/Vibration.h`.

## Serial Send Functions by Device

All devices have distinct functions for sending commands and complex data structures:

### Proton Pack (source/ProtonPack/include/Serial.h)
- **`packSerialSend(uint8_t command, uint16_t value)`** - Send command with optional uint16_t parameter
  - Envelope: PACKET_COMMAND (CommandPacket: c=command, d1=value)
  - Use for: consolidated commands, state changes, modes with parameters
- **`packSerialSendData(uint8_t message)`** - Send complex data payloads
  - Envelope: PACKET_DATA, PACKET_WAND, PACKET_SMOKE, PACKET_SYNC
  - Use for: preferences, sync data, special messages with d[3] array

### Neutrona Wand (source/NeutronaWand/include/Serial.h)
- **`wandSerialSend(uint8_t command, uint16_t value)`** - Send command with optional uint16_t parameter
  - Envelope: PACKET_COMMAND (CommandPacket: c=command, d1=value)
  - Use for: consolidated commands, state changes, modes with parameters
- **`wandSerialSendData(uint8_t message)`** - Send complex data payloads
  - Envelope: PACKET_DATA, PACKET_WAND, PACKET_SMOKE, PACKET_SYNC
  - Use for: preferences, sync data, special messages with d[3] array

### Attenuator (source/Attenuator/include/Serial.h)
- **`attenuatorSerialSend(uint8_t command, uint16_t value)`** - Send command with optional uint16_t parameter
  - Envelope: PACKET_COMMAND (CommandPacket: c=command, d1=value)
  - Use for: consolidated commands, state changes, modes with parameters
- **`attenuatorSerialSendData(uint8_t message)`** - Send complex data payloads
  - Envelope: PACKET_DATA, PACKET_PACK, PACKET_WAND, PACKET_SMOKE, PACKET_SYNC
  - Use for: preferences, sync data, special messages with d[3] array

## Command Outline

Rules used:
- API-first rows in original API order (excluding A_NO_OP until final row).
- Pack/Wand columns aligned by base-name match (prefix removed).
- Remaining non-API Pack/Wand items are appended after API rows.
- NO_OP is forced to the final row as a sentinel value.

## API-First Alignment Tables

| Index | API Message | Pack Message | Wand Message |
|---:|---|---|---|
| 0 | A_NULL | P_NULL | W_NULL |
| 1 | A_HANDSHAKE | P_HANDSHAKE | W_HANDSHAKE |
| 2 | A_SYNC_START | P_SYNC_START | - |
| 3 | A_SYNC_DATA | P_SYNC_DATA | - |
| 4 | A_SYNC_END | P_SYNC_END | - |
| 5 | A_WAND_ON | - | - |
| 6 | A_WAND_OFF | - | - |
| 7 | A_FIRING | - | W_FIRING |
| 8 | A_FIRING_STOPPED | - | W_FIRING_STOPPED |
| 9 | A_FIRING_CTS | - | - |
| 10 | A_FIRING_CTS_STOPPED | - | - |
| 11 | A_SYSTEM_LOCKOUT | P_SYSTEM_LOCKOUT | - |
| 12 | A_CANCEL_LOCKOUT | P_CANCEL_LOCKOUT | - |
| 13 | A_STREAM_FLAGS | - | W_STREAM_FLAGS |
| 14 | A_SET_STREAM_MODE | P_SET_STREAM_MODE | W_SET_STREAM_MODE |
| 15 | A_VENTING | - | W_VENTING |
| 16 | A_VENTING_FINISHED | P_VENTING_FINISHED | - |
| 17 | A_OVERHEATING | - | W_OVERHEATING |
| 18 | A_OVERHEATING_FINISHED | P_OVERHEATING_FINISHED | - |
| 19 | A_WARNING_CANCELLED | P_WARNING_CANCELLED | - |
| 20 | A_CYCLOTRON_LID_ON | P_CYCLOTRON_LID_ON | - |
| 21 | A_CYCLOTRON_LID_OFF | P_CYCLOTRON_LID_OFF | - |
| 22 | A_CYCLOTRON_NORMAL_SPEED | - | W_CYCLOTRON_NORMAL_SPEED |
| 23 | A_CYCLOTRON_INCREASE_SPEED | - | W_CYCLOTRON_INCREASE_SPEED |
| 24 | A_CYCLOTRON_DIRECTION_TOGGLE | - | W_CYCLOTRON_DIRECTION_TOGGLE |
| 25 | A_POWER_LEVEL_1 | - | W_POWER_LEVEL_1 |
| 26 | A_POWER_LEVEL_2 | - | W_POWER_LEVEL_2 |
| 27 | A_POWER_LEVEL_3 | - | W_POWER_LEVEL_3 |
| 28 | A_POWER_LEVEL_4 | - | W_POWER_LEVEL_4 |
| 29 | A_POWER_LEVEL_5 | - | W_POWER_LEVEL_5 |
| 30 | A_MUSIC_TRACK_LOOP_TOGGLE | - | W_MUSIC_TRACK_LOOP_TOGGLE |
| 31 | A_MUSIC_TRACK_SHUFFLE_TOGGLE | - | W_MUSIC_TRACK_SHUFFLE_TOGGLE |
| 32 | A_VOLUME_SOUND_EFFECTS_INCREASE | P_VOLUME_SOUND_EFFECTS_INCREASE | W_VOLUME_SOUND_EFFECTS_INCREASE |
| 33 | A_VOLUME_SOUND_EFFECTS_DECREASE | P_VOLUME_SOUND_EFFECTS_DECREASE | W_VOLUME_SOUND_EFFECTS_DECREASE |
| 34 | A_VOLUME_MUSIC_INCREASE | - | W_VOLUME_MUSIC_INCREASE |
| 35 | A_VOLUME_MUSIC_DECREASE | - | W_VOLUME_MUSIC_DECREASE |
| 36 | A_MUSIC_NEXT_TRACK | - | W_MUSIC_NEXT_TRACK |
| 37 | A_MUSIC_PREV_TRACK | - | W_MUSIC_PREV_TRACK |
| 38 | A_VOLUME_DECREASE | - | W_VOLUME_DECREASE |
| 39 | A_VOLUME_INCREASE | - | W_VOLUME_INCREASE |
| 40 | A_VOLUME_SET | - | - |
| 41 | A_VOLUME_SYNC | - | - |
| 42 | A_SAVE_EEPROM_SETTINGS_PACK | - | - |
| 43 | A_SAVE_EEPROM_SETTINGS_WAND | - | - |
| 44 | A_YEAR_FROZEN_EMPIRE | P_YEAR_FROZEN_EMPIRE | - |
| 45 | A_YEAR_AFTERLIFE | P_YEAR_AFTERLIFE | - |
| 46 | A_YEAR_1989 | P_YEAR_1989 | - |
| 47 | A_YEAR_1984 | P_YEAR_1984 | - |
| 48 | A_ALARM_ON | P_ALARM_ON | - |
| 49 | A_ALARM_OFF | P_ALARM_OFF | - |
| 50 | A_PACK_ON | - | - |
| 51 | A_PACK_OFF | - | - |
| 52 | A_TURN_PACK_ON | - | - |
| 53 | A_TURN_PACK_OFF | - | - |
| 54 | A_SPECTRAL_COLOUR_DATA | - | - |
| 55 | A_MUSIC_START_STOP | - | - |
| 56 | A_TOGGLE_MUTE | - | W_TOGGLE_MUTE |
| 57 | A_TOGGLE_SMOKE | - | - |
| 58 | A_TOGGLE_VIBRATION | - | - |
| 59 | A_BARREL_EXTENDED | - | W_BARREL_EXTENDED |
| 60 | A_BARREL_RETRACTED | - | W_BARREL_RETRACTED |
| 61 | A_MODE_SUPER_HERO | P_MODE_SUPER_HERO | - |
| 62 | A_MODE_ORIGINAL | P_MODE_ORIGINAL | - |
| 63 | A_ION_ARM_SWITCH_ON | P_ION_ARM_SWITCH_ON | - |
| 64 | A_ION_ARM_SWITCH_OFF | P_ION_ARM_SWITCH_OFF | - |
| 65 | A_MANUAL_OVERHEAT | P_MANUAL_OVERHEAT | - |
| 66 | A_MANUAL_QUICK_VENT | P_MANUAL_QUICK_VENT | - |
| 67 | A_MUSIC_TRACK_COUNT_SYNC | - | - |
| 68 | A_MUSIC_PAUSE_RESUME | - | - |
| 69 | A_MUSIC_IS_PLAYING | - | - |
| 70 | A_MUSIC_IS_NOT_PLAYING | - | - |
| 71 | A_MUSIC_IS_PAUSED | - | - |
| 72 | A_MUSIC_IS_NOT_PAUSED | - | - |
| 73 | A_MUSIC_PLAY_TRACK | - | - |
| 74 | A_BATTERY_VOLTAGE_PACK | - | - |
| 75 | A_TEMPERATURE_PACK | - | - |
| 76 | A_WAND_POWER_AMPS | - | - |
| 77 | A_WAND_CONNECTED | - | - |
| 78 | A_WAND_DISCONNECTED | - | - |
| 79 | A_WAND_AUDIO_VERSION | - | W_WAND_AUDIO_VERSION |
| 80 | A_RESET_WIFI_PASSWORD | - | W_RESET_WIFI_PASSWORD |
| 81 | A_REQUEST_PREFERENCES_PACK | - | - |
| 82 | A_REQUEST_PREFERENCES_WAND | - | - |
| 83 | A_REQUEST_PREFERENCES_SMOKE | - | - |
| 84 | A_SEND_PREFERENCES_PACK | - | - |
| 85 | A_SEND_PREFERENCES_WAND | P_SEND_PREFERENCES_WAND | W_SEND_PREFERENCES_WAND |
| 86 | A_SEND_PREFERENCES_SMOKE | P_SEND_PREFERENCES_SMOKE | W_SEND_PREFERENCES_SMOKE |
| 87 | A_SAVE_PREFERENCES_PACK | - | - |
| 88 | A_SAVE_PREFERENCES_WAND | P_SAVE_PREFERENCES_WAND | - |
| 89 | A_SAVE_PREFERENCES_SMOKE | P_SAVE_PREFERENCES_SMOKE | - |
| 90 | A_RESET_EEPROM_SETTINGS_PACK | - | - |
| 91 | A_RESET_EEPROM_SETTINGS_WAND | - | - |
| 92 | A_SET_FIRING_MODE | - | W_SET_FIRING_MODE |

| Index | API Message | Pack Message | Wand Message |
|---:|---|---|---|
| 93 | - | P_ON | W_ON |
| 94 | - | P_OFF | W_OFF |
| 95 | - | P_VIBRATION_ENABLED | W_VIBRATION_ENABLED |
| 96 | - | P_VIBRATION_DISABLED | W_VIBRATION_DISABLED |
| 97 | - | P_PACK_VIBRATION_ENABLED | - |
| 98 | - | P_PACK_VIBRATION_DISABLED | - |
| 99 | - | P_PACK_VIBRATION_FIRING_ENABLED | - |
| 100 | - | P_PACK_VIBRATION_DEFAULT | - |
| 101 | - | P_PACK_MOTORIZED_CYCLOTRON_ENABLED | - |
| 102 | - | P_VIDEO_GAME_MODE_COLOURS_ENABLED | - |
| 103 | - | P_VIDEO_GAME_MODE_POWER_CELL_ENABLED | - |
| 104 | - | P_VIDEO_GAME_MODE_CYCLOTRON_ENABLED | - |
| 105 | - | P_VIDEO_GAME_MODE_COLOURS_DISABLED | - |
| 106 | - | P_MODE_FROZEN_EMPIRE | - |
| 107 | - | P_MODE_AFTERLIFE | - |
| 108 | - | P_MODE_1989 | - |
| 109 | - | P_MODE_1984 | - |
| 110 | - | P_SMOKE_DISABLED | - |
| 111 | - | P_SMOKE_ENABLED | - |
| 112 | - | P_CYCLOTRON_COUNTER_CLOCKWISE | - |
| 113 | - | P_CYCLOTRON_CLOCKWISE | - |
| 114 | - | P_CYCLOTRON_SINGLE_LED | - |
| 115 | - | P_CYCLOTRON_THREE_LED | - |
| 116 | - | P_MUSIC_STATUS | - |
| 117 | - | P_MUSIC_LOOP_STATUS | - |
| 118 | - | P_MUSIC_SHUFFLE_STATUS | - |
| 119 | - | P_MASTER_AUDIO_STATUS | - |
| 120 | - | P_POWERCELL_DIMMING | - |
| 121 | - | P_CYCLOTRON_DIMMING | - |
| 122 | - | P_INNER_CYCLOTRON_DIMMING | - |
| 123 | - | P_CYCLOTRON_PANEL_DIMMING | - |
| 124 | - | P_DIMMING | - |
| 125 | - | P_PROTON_STREAM_IMPACT_ENABLED --> REMOVED | - |
| 126 | - | P_PROTON_STREAM_IMPACT_DISABLED --> REMOVED | - |
| 127 | - | P_RGB_INNER_CYCLOTRON_LEDS | - |
| 128 | - | P_GRB_INNER_CYCLOTRON_LEDS | - |
| 129 | - | P_CYCLOTRON_LEDS_40 | - |
| 130 | - | P_CYCLOTRON_LEDS_36 | - |
| 131 | - | P_CYCLOTRON_LEDS_20 | - |
| 132 | - | P_CYCLOTRON_LEDS_12 | - |
| 133 | - | P_POWERCELL_LEDS_15 | - |
| 134 | - | P_POWERCELL_LEDS_13 | - |
| 135 | - | P_INNER_CYCLOTRON_LEDS_23 | - |
| 136 | - | P_INNER_CYCLOTRON_LEDS_24 | - |
| 137 | - | P_INNER_CYCLOTRON_LEDS_26 | - |
| 138 | - | P_INNER_CYCLOTRON_LEDS_35 | - |
| 139 | - | P_INNER_CYCLOTRON_LEDS_36 | - |
| 140 | - | P_INNER_CYCLOTRON_LEDS_12 | - |
| 141 | - | P_CYCLOTRON_FADING_DISABLED | - |
| 142 | - | P_CYCLOTRON_FADING_ENABLED | - |
| 143 | - | P_CYCLOTRON_SIMULATE_RING_DISABLED | - |
| 144 | - | P_CYCLOTRON_SIMULATE_RING_ENABLED | - |
| 145 | - | P_OVERHEAT_STROBE_ENABLED | - |
| 146 | - | P_OVERHEAT_STROBE_DISABLED | - |
| 147 | - | P_OVERHEAT_LIGHTS_OFF_ENABLED | - |
| 148 | - | P_OVERHEAT_LIGHTS_OFF_DISABLED | - |
| 149 | - | P_OVERHEAT_SYNC_FAN_DISABLED | - |
| 150 | - | P_OVERHEAT_SYNC_FAN_ENABLED | - |
| 151 | - | P_YEAR_MODE_DEFAULT | - |
| 152 | - | P_DEMO_LIGHT_MODE_ENABLED | - |
| 153 | - | P_DEMO_LIGHT_MODE_DISABLED | - |
| 154 | - | P_CONTINUOUS_SMOKE_5_ENABLED | - |
| 155 | - | P_CONTINUOUS_SMOKE_4_ENABLED | - |
| 156 | - | P_CONTINUOUS_SMOKE_3_ENABLED | - |
| 157 | - | P_CONTINUOUS_SMOKE_2_ENABLED | - |
| 158 | - | P_CONTINUOUS_SMOKE_1_ENABLED | - |
| 159 | - | P_CONTINUOUS_SMOKE_5_DISABLED | - |
| 160 | - | P_CONTINUOUS_SMOKE_4_DISABLED | - |
| 161 | - | P_CONTINUOUS_SMOKE_3_DISABLED | - |
| 162 | - | P_CONTINUOUS_SMOKE_2_DISABLED | - |
| 163 | - | P_CONTINUOUS_SMOKE_1_DISABLED | - |
| 164 | - | P_SOUND_SUPER_HERO | - |
| 165 | - | P_SOUND_MODE_ORIGINAL | - |
| 166 | - | P_SAVE_EEPROM_WAND | - |
| 167 | - | P_RESET_EEPROM_WAND | - |
| 168 | - | P_INNER_CYCLOTRON_PANEL_DISABLED | - |
| 169 | - | P_INNER_CYCLOTRON_PANEL_STATIC | - |
| 170 | - | P_INNER_CYCLOTRON_PANEL_DYNAMIC | - |
| 171 | - | P_POWERCELL_NOT_INVERTED | - |
| 172 | - | P_POWERCELL_INVERTED | - |
| 173 | - | P_PACK_GPSTAR_AUDIO_LED_DISABLED | - |
| 174 | - | P_PACK_GPSTAR_AUDIO_LED_ENABLED | - |
| 175 | - | P_REQUEST_BEEP_SYNC | - |
| 176 | - | P_QUICK_BOOTUP_ENABLED | - |
| 177 | - | P_QUICK_BOOTUP_DISABLED | - |
| 178 | - | P_TURN_WAND_ON | - |
| 179 | - | P_POST_FINISH | - |

| Index | API Message | Pack Message | Wand Message |
|---:|---|---|---|
| 180 | - | - | W_SYNC_NOW |
| 181 | - | - | W_SYNCHRONIZED |
| 182 | - | - | W_BUTTON_MASHING |
| 183 | - | - | W_BEEP_START |
| 184 | - | - | W_FIRING_INTENSIFY_MIX |
| 185 | - | - | W_FIRING_INTENSIFY_STOPPED_MIX |
| 186 | - | - | W_FIRING_ALT_MIX |
| 187 | - | - | W_FIRING_ALT_STOPPED_MIX |
| 188 | - | - | W_FIRING_CROSSING_THE_STREAMS_1984 |
| 189 | - | - | W_FIRING_CROSSING_THE_STREAMS_MIX_1984 |
| 190 | - | - | W_FIRING_CROSSING_THE_STREAMS_STOPPED_MIX_1984 |
| 191 | - | - | W_FIRING_CROSSING_THE_STREAMS_2021 |
| 192 | - | - | W_FIRING_CROSSING_THE_STREAMS_MIX_2021 |
| 193 | - | - | W_FIRING_CROSSING_THE_STREAMS_STOPPED_MIX_2021 |
| 194 | - | - | W_YEAR_MODES_CYCLE |
| 195 | - | - | W_VIDEO_GAME_MODE_COLOUR_TOGGLE |
| 196 | - | - | W_CROSS_THE_STREAMS |
| 197 | - | - | W_CROSS_THE_STREAMS_MIX |
| 198 | - | - | W_VIBRATION_FIRING_ENABLED |
| 199 | - | - | W_VIBRATION_DEFAULT |
| 200 | - | - | W_VIBRATION_CYCLE_TOGGLE |
| 201 | - | - | W_VIBRATION_CYCLE_TOGGLE_EEPROM |
| 202 | - | - | W_SMOKE_TOGGLE |
| 203 | - | - | W_VIDEO_GAME_MODE |
| 204 | - | - | W_CYCLOTRON_LED_TOGGLE |
| 205 | - | - | W_OVERHEATING_DISABLED |
| 206 | - | - | W_OVERHEATING_ENABLED |
| 207 | - | - | W_MUSIC_TOGGLE |
| 208 | - | - | W_MENU_LEVEL_1 |
| 209 | - | - | W_MENU_LEVEL_2 |
| 210 | - | - | W_MENU_LEVEL_3 |
| 211 | - | - | W_MENU_LEVEL_4 |
| 212 | - | - | W_MENU_LEVEL_5 |
| 213 | - | - | W_DIMMING_TOGGLE |
| 214 | - | - | W_DIMMING_INCREASE |
| 215 | - | - | W_DIMMING_DECREASE |
| 216 | - | - | W_PROTON_STREAM_IMPACT_TOGGLE --> W_SET_PROTON_STREAM_IMPACT |
| 217 | - | - | W_CLEAR_LED_EEPROM_SETTINGS |
| 218 | - | - | W_SAVE_LED_EEPROM_SETTINGS |
| 219 | - | - | W_TOGGLE_CYCLOTRON_LEDS |
| 220 | - | - | W_TOGGLE_POWERCELL_LEDS |
| 221 | - | - | W_TOGGLE_INNER_CYCLOTRON_LEDS |
| 222 | - | - | W_TOGGLE_RGB_INNER_CYCLOTRON_LEDS |
| 223 | - | - | W_EEPROM_LED_MENU |
| 224 | - | - | W_EEPROM_CONFIG_MENU |
| 225 | - | - | W_CLEAR_CONFIG_EEPROM_SETTINGS |
| 226 | - | - | W_SAVE_CONFIG_EEPROM_SETTINGS |
| 227 | - | - | W_EXTRA_WAND_SOUNDS_STOP |
| 228 | - | - | W_AFTERLIFE_GUN_RAMP_1 |
| 229 | - | - | W_AFTERLIFE_GUN_RAMP_2 |
| 230 | - | - | W_AFTERLIFE_RAMP_LOOP_2_STOP |
| 231 | - | - | W_AFTERLIFE_GUN_LOOP_1 |
| 232 | - | - | W_AFTERLIFE_GUN_LOOP_2 |
| 233 | - | - | W_AFTERLIFE_GUN_RAMP_DOWN_2 |
| 234 | - | - | W_AFTERLIFE_GUN_RAMP_DOWN_1 |
| 235 | - | - | W_AFTERLIFE_GUN_RAMP_DOWN_2_FADE_OUT |
| 236 | - | - | W_AFTERLIFE_GUN_RAMP_2_FADE_IN |
| 237 | - | - | W_VOICE_NEUTRONA_WAND_SOUNDS_ENABLED |
| 238 | - | - | W_VOICE_NEUTRONA_WAND_SOUNDS_DISABLED |
| 239 | - | - | W_CYCLOTRON_SIMULATE_RING_TOGGLE |
| 240 | - | - | W_SPECTRAL_MODES_ENABLED |
| 241 | - | - | W_SPECTRAL_MODES_DISABLED |
| 242 | - | - | W_SPECTRAL_INNER_CYCLOTRON_CUSTOM_DECREASE |
| 243 | - | - | W_SPECTRAL_CYCLOTRON_CUSTOM_DECREASE |
| 244 | - | - | W_SPECTRAL_POWERCELL_CUSTOM_DECREASE |
| 245 | - | - | W_SPECTRAL_POWERCELL_CUSTOM_INCREASE |
| 246 | - | - | W_SPECTRAL_CYCLOTRON_CUSTOM_INCREASE |
| 247 | - | - | W_SPECTRAL_INNER_CYCLOTRON_CUSTOM_INCREASE |
| 248 | - | - | W_SPECTRAL_LIGHTS_ON |
| 249 | - | - | W_SPECTRAL_LIGHTS_OFF |
| 250 | - | - | W_QUICK_VENT_ENABLED |
| 251 | - | - | W_QUICK_VENT_DISABLED |
| 252 | - | - | W_BOOTUP_ERRORS_ENABLED |
| 253 | - | - | W_BOOTUP_ERRORS_DISABLED |
| 254 | - | - | W_BARREL_LEDS_2 --> REMOVED |
| 255 | - | - | W_BARREL_LEDS_5 --> REMOVED |
| 256 | - | - | W_BARREL_LEDS_48 --> REMOVED |
| 257 | - | - | W_BARREL_LEDS_50 --> REMOVED |
| 258 | - | - | W_BARGRAPH_INVERTED |
| 259 | - | - | W_BARGRAPH_NOT_INVERTED |
| 260 | - | - | W_OVERHEAT_STROBE_TOGGLE |
| 261 | - | - | W_OVERHEAT_LIGHTS_OFF_TOGGLE |
| 262 | - | - | W_OVERHEAT_SYNC_TO_FAN_TOGGLE |
| 263 | - | - | W_YEAR_MODES_CYCLE_EEPROM |
| 264 | - | - | W_OVERHEAT_INCREASE_LEVEL_1 |
| 265 | - | - | W_OVERHEAT_INCREASE_LEVEL_2 |
| 266 | - | - | W_OVERHEAT_INCREASE_LEVEL_3 |
| 267 | - | - | W_OVERHEAT_INCREASE_LEVEL_4 |
| 268 | - | - | W_OVERHEAT_INCREASE_LEVEL_5 |
| 269 | - | - | W_OVERHEAT_DECREASE_LEVEL_1 |
| 270 | - | - | W_OVERHEAT_DECREASE_LEVEL_2 |
| 271 | - | - | W_OVERHEAT_DECREASE_LEVEL_3 |
| 272 | - | - | W_OVERHEAT_DECREASE_LEVEL_4 |
| 273 | - | - | W_OVERHEAT_DECREASE_LEVEL_5 |
| 274 | - | - | W_BARGRAPH_OVERHEAT_BLINK_ENABLED |
| 275 | - | - | W_BARGRAPH_OVERHEAT_BLINK_DISABLED |
| 276 | - | - | W_MODE_BEEP_LOOP_ENABLED |
| 277 | - | - | W_MODE_BEEP_LOOP_DISABLED |
| 278 | - | - | W_DEFAULT_BARGRAPH |
| 279 | - | - | W_MODE_ORIGINAL_BARGRAPH |
| 280 | - | - | W_SUPER_HERO_BARGRAPH |
| 281 | - | - | W_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH |
| 282 | - | - | W_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH |
| 283 | - | - | W_DEFAULT_FIRING_ANIMATIONS_BARGRAPH |

## Phase 3: LED Count Consolidation

The following enums represent distinct LED count selections and should be consolidated using `A_SET_*` commands with `d1` parameter values instead of individual enums.

### Item 1: Cyclotron LED Count (4 enums → 1 consolidated)

**Current State (Enums 129-132):**
- P_CYCLOTRON_LEDS_40 (enum 129)
- P_CYCLOTRON_LEDS_36 (enum 130)
- P_CYCLOTRON_LEDS_20 (enum 131)
- P_CYCLOTRON_LEDS_12 (enum 132)

**Proposed Consolidation:**
- Replace with: `A_SET_CYCLOTRON_LED_COUNT` (d1: 12, 20, 36, 40)
- **Enum Slots Saved: 3**

**Implementation Details:**
- ProtonPack/Serial.h: Replace cyclotron LED toggle cases with single A_SET_CYCLOTRON_LED_COUNT case, passing LED count as d1
- NeutronaWand/Command.h: Replace 4 LED count cases with single A_SET_CYCLOTRON_LED_COUNT case, voice effect based on d1 value

### Item 2: Powercell LED Count (2 enums → 1 consolidated)

**Current State (Enums 133-134):**
- P_POWERCELL_LEDS_15 (enum 133)
- P_POWERCELL_LEDS_13 (enum 134)

**Proposed Consolidation:**
- Replace with: `A_SET_POWERCELL_LED_COUNT` (d1: 13, 15)
- **Enum Slots Saved: 1**

**Implementation Details:**
- ProtonPack/Serial.h: Replace powercell LED toggle cases with single A_SET_POWERCELL_LED_COUNT case, passing LED count as d1
- NeutronaWand/Command.h: Replace 2 LED count cases with single A_SET_POWERCELL_LED_COUNT case, voice effect based on d1 value

### Item 3: Inner Cyclotron LED Count (6 enums → 1 consolidated)

**Current State (Enums 135-140):**
- P_INNER_CYCLOTRON_LEDS_23 (enum 135)
- P_INNER_CYCLOTRON_LEDS_24 (enum 136)
- P_INNER_CYCLOTRON_LEDS_26 (enum 137)
- P_INNER_CYCLOTRON_LEDS_35 (enum 138)
- P_INNER_CYCLOTRON_LEDS_36 (enum 139)
- P_INNER_CYCLOTRON_LEDS_12 (enum 140)

**Proposed Consolidation:**
- Replace with: `A_SET_INNER_CYCLOTRON_LED_COUNT` (d1: 12, 23, 24, 26, 35, 36)
- **Enum Slots Saved: 5**

**Implementation Details:**
- ProtonPack/Serial.h: Replace inner cyclotron LED toggle cases with single A_SET_INNER_CYCLOTRON_LED_COUNT case, passing LED count as d1
- NeutronaWand/Command.h: Replace 6 LED count cases with single A_SET_INNER_CYCLOTRON_LED_COUNT case, voice effect based on d1 value

### Phase 3 Summary

- **Total Enums Being Consolidated: 12**
- **New Consolidated A_SET_* Enums: 3**
- **Total Enum Slots Saved: 9**
- **Communication Pattern:** LED count commands use the A_SET_* enum with d1 parameter carrying the actual LED count value
- **Voice Feedback:** Receivers play appropriate voice effect based on d1 LED count value
- **No Functional Change:** User experience and functionality remain identical; only serial communication becomes more efficient
| 284 | - | - | W_NEUTRONA_WAND_1984_MODE |
| 285 | - | - | W_NEUTRONA_WAND_1989_MODE |
| 286 | - | - | W_NEUTRONA_WAND_AFTERLIFE_MODE |
| 287 | - | - | W_NEUTRONA_WAND_FROZEN_EMPIRE_MODE |
| 288 | - | - | W_NEUTRONA_WAND_DEFAULT_MODE |
| 289 | - | - | W_DEMO_LIGHT_MODE_TOGGLE |
| 290 | - | - | W_CTS_DEFAULT |
| 291 | - | - | W_CTS_1984 |
| 292 | - | - | W_CTS_AFTERLIFE |
| 293 | - | - | W_MODE_TOGGLE |
| 294 | - | - | W_OVERHEAT_LEVEL_5_ENABLED |
| 295 | - | - | W_OVERHEAT_LEVEL_4_ENABLED |
| 296 | - | - | W_OVERHEAT_LEVEL_3_ENABLED |
| 297 | - | - | W_OVERHEAT_LEVEL_2_ENABLED |
| 298 | - | - | W_OVERHEAT_LEVEL_1_ENABLED |
| 299 | - | - | W_OVERHEAT_LEVEL_5_DISABLED |
| 300 | - | - | W_OVERHEAT_LEVEL_4_DISABLED |
| 301 | - | - | W_OVERHEAT_LEVEL_3_DISABLED |
| 302 | - | - | W_OVERHEAT_LEVEL_2_DISABLED |
| 303 | - | - | W_OVERHEAT_LEVEL_1_DISABLED |
| 304 | - | - | W_CONTINUOUS_SMOKE_TOGGLE_5 |
| 305 | - | - | W_CONTINUOUS_SMOKE_TOGGLE_4 |
| 306 | - | - | W_CONTINUOUS_SMOKE_TOGGLE_3 |
| 307 | - | - | W_CONTINUOUS_SMOKE_TOGGLE_2 |
| 308 | - | - | W_CONTINUOUS_SMOKE_TOGGLE_1 |
| 309 | - | - | W_VOLUME_DECREASE_EEPROM |
| 310 | - | - | W_VOLUME_INCREASE_EEPROM |
| 311 | - | - | W_SOUND_OVERHEAT_SMOKE_DURATION_LEVEL_5 |
| 312 | - | - | W_SOUND_OVERHEAT_SMOKE_DURATION_LEVEL_4 |
| 313 | - | - | W_SOUND_OVERHEAT_SMOKE_DURATION_LEVEL_3 |
| 314 | - | - | W_SOUND_OVERHEAT_SMOKE_DURATION_LEVEL_2 |
| 315 | - | - | W_SOUND_OVERHEAT_SMOKE_DURATION_LEVEL_1 |
| 316 | - | - | W_SOUND_OVERHEAT_START_TIMER_LEVEL_5 |
| 317 | - | - | W_SOUND_OVERHEAT_START_TIMER_LEVEL_4 |
| 318 | - | - | W_SOUND_OVERHEAT_START_TIMER_LEVEL_3 |
| 319 | - | - | W_SOUND_OVERHEAT_START_TIMER_LEVEL_2 |
| 320 | - | - | W_SOUND_OVERHEAT_START_TIMER_LEVEL_1 |
| 321 | - | - | W_SOUND_DEFAULT_SYSTEM_VOLUME_ADJUSTMENT |
| 322 | - | - | W_GB1_WAND_BARREL_EXTEND |
| 323 | - | - | W_AFTERLIFE_WAND_BARREL_EXTEND |
| 324 | - | - | W_WAND_BARREL_RETRACT |
| 325 | - | - | W_WAND_BOOTUP_SOUND |
| 326 | - | - | W_WAND_BOOTUP_SHORT_SOUND |
| 327 | - | - | W_WAND_SHUTDOWN_SOUND |
| 328 | - | - | W_WAND_MASH_ERROR_SOUND |
| 329 | - | - | W_WAND_BEEP_SOUNDS |
| 330 | - | - | W_WAND_BEEP_BARGRAPH |
| 331 | - | - | W_MODE_ORIGINAL_HEATUP_STOP |
| 332 | - | - | W_MODE_ORIGINAL_HEATUP |
| 333 | - | - | W_MODE_ORIGINAL_HEATDOWN_STOP |
| 334 | - | - | W_MODE_ORIGINAL_HEATDOWN |
| 335 | - | - | W_BEEPS_ALT |
| 336 | - | - | W_WAND_BEEP_STOP |
| 337 | - | - | W_WAND_BEEP_STOP_LOOP |
| 338 | - | - | W_WAND_BEEP_START |
| 339 | - | - | W_WAND_BEEP |
| 340 | - | - | W_MASH_ERROR_LOOP |
| 341 | - | - | W_MASH_ERROR_RESTART |
| 342 | - | - | W_BOSON_DART_SOUND |
| 343 | - | - | W_SHOCK_BLAST_SOUND |
| 344 | - | - | W_SLIME_TETHER_SOUND |
| 345 | - | - | W_MESON_COLLIDER_SOUND |
| 346 | - | - | W_MESON_FIRE_PULSE |
| 347 | - | - | W_TOGGLE_INNER_CYCLOTRON_PANEL |
| 348 | - | - | W_WAND_BOOTUP_1989 |
| 349 | - | - | W_TOGGLE_POWERCELL_DIRECTION |
| 350 | - | - | W_TOGGLE_CYCLOTRON_FADING |
| 351 | - | - | W_TOGGLE_PACK_WIFI |
| 352 | - | - | W_WAND_WIFI_RESET |
| 353 | - | - | W_WAND_WIFI_DISABLED |
| 354 | - | - | W_WAND_WIFI_ENABLED |
| 355 | - | - | W_BARREL_ERROR_SOUND |
| 356 | - | - | W_BARREL_SWITCH_DEFAULT |
| 357 | - | - | W_BARREL_SWITCH_INVERTED |
| 358 | - | - | W_BARREL_SWITCH_DISABLED |
| 359 | - | - | W_BARGRAPH_28_SEGMENTS |
| 360 | - | - | W_BARGRAPH_30_SEGMENTS |
| 361 | - | - | W_RGB_VENT_DISABLED |
| 362 | - | - | W_RGB_VENT_ENABLED |
| 363 | - | - | W_AUTO_VENT_INTENSITY_DISABLED |
| 364 | - | - | W_AUTO_VENT_INTENSITY_ENABLED |
| 365 | - | - | W_GPSTAR_AUDIO_LED_TOGGLE |
| 366 | - | - | W_WAND_GPSTAR_AUDIO_LED_DISABLED |
| 367 | - | - | W_WAND_GPSTAR_AUDIO_LED_ENABLED |
| 368 | - | - | W_QUICK_BOOTUP_TOGGLE |
| 369 | - | - | W_IMPACT_SOUND |
| 370 | - | - | W_COM_SOUND_NUMBER |
| 371 | - | - | W_VENT_LIGHT_COLOURS_DISABLED |
| 372 | - | - | W_VENT_LIGHT_COLOURS_ENABLED |

| Index | API Message | Pack Message | Wand Message |
|---:|---|---|---|
| 373 | A_NO_OP | P_NO_OP | W_NO_OP |

## Implementation Plan

There are 2 main aspects to consider in this proposed effort:

1. Aligning any one-off `P_` and `W_` prefixed API calls.
2. Condensing API calls which are intended to enable/disable or set a specific value.
3. Ignore any ancillary cleanup until after all consolidations and renames have taken place.

### Phase 1: High-Impact Consolidations (Estimated 36-37 enum slots saved)

These consolidations follow a consistent pattern: **NAME carries semantic meaning (what is being controlled), DATA carries state/mode (0/1 or mode enum)**. This achieves ~50% reduction for enable/disable pairs while maintaining clarity.

#### 1. CONTINUOUS SMOKE (10 P_ commands + 5 W_ commands) **Save 10 slots**
- **Current Pack**: P_CONTINUOUS_SMOKE_5_ENABLED, P_CONTINUOUS_SMOKE_5_DISABLED, ..., P_CONTINUOUS_SMOKE_1_DISABLED (10 entries)
- **Current Wand**: W_CONTINUOUS_SMOKE_TOGGLE_5, ..., W_CONTINUOUS_SMOKE_TOGGLE_1 (5 entries)
- **Proposed**: Consolidate to 5 P_CONTINUOUS_SMOKE_X commands; sender uses `packSerialSend(P_CONTINUOUS_SMOKE_5, 1)` for on, `packSerialSend(P_CONTINUOUS_SMOKE_5, 0)` for off; remove W_CONTINUOUS_SMOKE_TOGGLE_X entries
- **Rationale**: NAME (P_CONTINUOUS_SMOKE_5) identifies the level; DATA in d1 parameter (0/1) identifies the state. This is clearer than collapsing to a single command—always know which level is being set.
- **Implementation**: Replace P_CONTINUOUS_SMOKE_X_ENABLED/DISABLED pairs with single P_SET_CONTINUOUS_SMOKE_X commands. Remove W_CONTINUOUS_SMOKE_TOGGLE_X from WAND_MESSAGE; handlers call P_SET_CONTINUOUS_SMOKE_X with toggled data instead.
- **Savings**: 5 slots in PACK_MESSAGE (10→5) + 5 slots in WAND_MESSAGE (5→0) = **10 total**

#### 2. OVERHEAT LEVELS (10 W_ commands) **Save 5 slots**
- **Current Wand**: W_OVERHEAT_LEVEL_5_ENABLED, W_OVERHEAT_LEVEL_5_DISABLED, ..., W_OVERHEAT_LEVEL_1_DISABLED (10 entries)
- **Proposed**: Consolidate to 5 W_OVERHEAT_LEVEL_X commands; sender uses `wandSerialSend(W_OVERHEAT_LEVEL_5, 1)` for enabled, `wandSerialSend(W_OVERHEAT_LEVEL_5, 0)` for disabled
- **Rationale**: NAME (W_OVERHEAT_LEVEL_5) identifies the level; DATA in d1 parameter (0/1) identifies the state. Pack receives these and interprets data value accordingly.
- **Implementation**: Replace all W_OVERHEAT_LEVEL_X_ENABLED/DISABLED pairs with single W_SET_OVERHEAT_LEVEL_X commands. Update Pack Serial.h handlers to extract data for state.
- **Savings**: 5 slots in WAND_MESSAGE (10→5) = **5 total**

#### 3. POWER LEVELS (5 A_ commands) **Save 4 slots**
- **Current API**: A_POWER_LEVEL_1, A_POWER_LEVEL_2, A_POWER_LEVEL_3, A_POWER_LEVEL_4, A_POWER_LEVEL_5
- **Proposed**: Replace with single `A_SET_POWER_LEVEL`; sender uses `attenuatorSerialSend(A_SET_POWER_LEVEL, 1-5)` with level 1-5 in d1 parameter
- **Rationale**: Already unified under A_; consolidate to single parameterized version for consistency.
- **Implementation**: Update Wand W_POWER_LEVEL_1-5 sends to use A_SET_POWER_LEVEL (data=level). Update Pack/Attenuator handlers accordingly.
- **Savings**: 4 slots in API_MESSAGE (5→1) = **4 total**

#### 4. PACK VIBRATION (4 P_ commands) **Save 3 slots**
- **Current Pack**: P_PACK_VIBRATION_ENABLED, P_PACK_VIBRATION_DISABLED, P_PACK_VIBRATION_FIRING_ENABLED, P_PACK_VIBRATION_DEFAULT
- **Proposed**: Replace with single `P_SET_PACK_VIBRATION_MODE`; sender uses `packSerialSend(P_SET_PACK_VIBRATION_MODE, mode)` with VIBRATION_MODES enum value in d1 parameter
- **Rationale**: Multiple vibration modes; consolidate to single command where NAME identifies purpose and DATA identifies mode. Reference [VIBRATION_MODES](source/SharedLib/DeviceState/include/Vibration.h) enum: VIBRATION_ALWAYS (1), VIBRATION_FIRING_ONLY (2), VIBRATION_NEVER (3), VIBRATION_DEFAULT (4).
- **Implementation**: Update Wand Command.h handlers for P_PACK_VIBRATION_* to send P_SET_PACK_VIBRATION_MODE(data=mode).
- **Savings**: 3 slots in PACK_MESSAGE (4→1) = **3 total**

#### 5. WAND VIBRATION (4 W_ commands → 1 A_ command) **Save 3 slots**
- **Current Wand**: W_VIBRATION_ENABLED, W_VIBRATION_DISABLED, W_VIBRATION_FIRING_ENABLED, W_VIBRATION_DEFAULT
- **Proposed**: `A_SET_WAND_VIBRATION_MODE`; sender uses `attenuatorSerialSend(A_SET_WAND_VIBRATION_MODE, mode)` with VIBRATION_MODES enum value in d1 parameter
- **Rationale**: Multiple vibration modes for wand hardware. Consolidate into single command with mode identifier.
- **Implementation**: Update Pack Serial.h case handlers for W_VIBRATION_* to send A_SET_WAND_VIBRATION_MODE based on mode parameter.

#### 6. DEMO LIGHT MODE (3 commands → 1 A_ command) **Save 2 slots**
- **Current Pack**: P_DEMO_LIGHT_MODE_ENABLED, P_DEMO_LIGHT_MODE_DISABLED
- **Current Wand**: W_DEMO_LIGHT_MODE_TOGGLE
- **Proposed**: `A_SET_DEMO_LIGHT_MODE`; sender uses `attenuatorSerialSend(A_SET_DEMO_LIGHT_MODE, 0|1)` with state in d1 parameter
- **Rationale**: Pack uses discrete on/off; Wand uses toggle. Unified command accepts data.
- **Implementation**: Replace P_DEMO_LIGHT_MODE_* handlers in Wand Command.h. Replace W_DEMO_LIGHT_MODE_TOGGLE handler in Pack Serial.h to toggle then send A_SET_DEMO_LIGHT_MODE.

#### 7. CYCLOTRON SIMULATE RING (3 commands → 1 A_ command) **Save 2 slots**
- **Current Pack**: P_CYCLOTRON_SIMULATE_RING_DISABLED, P_CYCLOTRON_SIMULATE_RING_ENABLED
- **Current Wand**: W_CYCLOTRON_SIMULATE_RING_TOGGLE
- **Proposed**: `A_SET_CYCLOTRON_SIMULATE_RING`; sender uses `attenuatorSerialSend(A_SET_CYCLOTRON_SIMULATE_RING, 0|1)` with state in d1 parameter
- **Implementation**: Similar to Demo Light Mode consolidation.

#### 8. OVERHEAT STROBE (3 commands → 1 A_ command) **Save 2 slots**
- **Current Pack**: P_OVERHEAT_STROBE_ENABLED, P_OVERHEAT_STROBE_DISABLED
- **Current Wand**: W_OVERHEAT_STROBE_TOGGLE
- **Proposed**: `A_SET_OVERHEAT_STROBE`; sender uses `attenuatorSerialSend(A_SET_OVERHEAT_STROBE, 0|1)` with state in d1 parameter
- **Implementation**: Similar pattern to previous ENABLED/DISABLED + TOGGLE consolidations.

#### 9. OVERHEAT LIGHTS OFF (3 commands → 1 A_ command) **Save 2 slots**
- **Current Pack**: P_OVERHEAT_LIGHTS_OFF_ENABLED, P_OVERHEAT_LIGHTS_OFF_DISABLED
- **Current Wand**: W_OVERHEAT_LIGHTS_OFF_TOGGLE
- **Proposed**: `A_SET_OVERHEAT_LIGHTS_OFF`; sender uses `attenuatorSerialSend(A_SET_OVERHEAT_LIGHTS_OFF, 0|1)` with state in d1 parameter
- **Implementation**: Similar pattern.

#### 10. OVERHEAT SYNC FAN (2 P_ commands → 1 A_ command) **Save 1 slot**
- **Current Pack**: P_OVERHEAT_SYNC_FAN_DISABLED, P_OVERHEAT_SYNC_FAN_ENABLED
- **Note**: No Wand equivalent exists (consistency gap)
- **Proposed**: `A_SET_OVERHEAT_SYNC_FAN`; sender uses `attenuatorSerialSend(A_SET_OVERHEAT_SYNC_FAN, 0|1)` with state in d1 parameter
- **Implementation**: Update Wand Command.h handlers to use parameterized version.

#### 11. BARREL STATE (4 commands → 1 A_ command) **Save 2 slots**
- **Current API**: A_BARREL_EXTENDED, A_BARREL_RETRACTED
- **Current Wand**: W_BARREL_EXTENDED, W_BARREL_RETRACTED
- **Proposed**: `A_SET_BARREL_STATE`; sender uses `attenuatorSerialSend(A_SET_BARREL_STATE, 0|1)` with state (0=retracted, 1=extended) in d1 parameter
- **Rationale**: Remove redundant W_ variants; consolidate to single A_ command.
- **Implementation**: Remove W_BARREL_EXTENDED/RETRACTED from WAND_MESSAGE enum. Update all handlers to use A_SET_BARREL_STATE.

**Phase 1 Total Savings: ~45 enum slots**

---

### Phase 2: Consistency & Gap Fixes (Additional 3 slots)

#### 1. QUICK BOOTUP (Unused Wand variant)
- **Current Pack**: P_QUICK_BOOTUP_ENABLED, P_QUICK_BOOTUP_DISABLED
- **Current Wand**: W_QUICK_BOOTUP_TOGGLE (defined but no handler in Pack Serial.h)
- **Issue**: Wand variant exists but Pack doesn't handle it consistently.
- **Proposed**: `A_SET_QUICK_BOOTUP`; sender uses `attenuatorSerialSend(A_SET_QUICK_BOOTUP, 0|1)` with state (0=disabled, 1=enabled) in d1 parameter
- **Implementation**: Replace P_QUICK_BOOTUP_* handlers in Wand Command.h. Add handler in Pack Serial.h for W_QUICK_BOOTUP_TOGGLE if needed.

#### 2. GPSTAR AUDIO LED (Device-specific variants)
- **Current Pack**: P_PACK_GPSTAR_AUDIO_LED_DISABLED, P_PACK_GPSTAR_AUDIO_LED_ENABLED
- **Current Wand**: W_WAND_GPSTAR_AUDIO_LED_DISABLED, W_WAND_GPSTAR_AUDIO_LED_ENABLED
- **Issue**: Four commands for same feature across two device types.
- **Proposed**: `A_SET_PACK_GPSTAR_AUDIO_LED` and `A_SET_WAND_GPSTAR_AUDIO_LED`; sender uses `attenuatorSerialSend(A_SET_PACK_GPSTAR_AUDIO_LED, 0|1)` and `attenuatorSerialSend(A_SET_WAND_GPSTAR_AUDIO_LED, 0|1)` with state (0=disabled, 1=enabled) in d1 parameter
- **Implementation**: Replace P_PACK_GPSTAR_AUDIO_LED_* and W_WAND_GPSTAR_AUDIO_LED_* with two A_ commands.

**Phase 2 Total Savings: 3 slots (4 commands → 2 commands, net -2; but improves consistency)**

---

### Phase 3: Separate Wand-Internal Commands (Do NOT consolidate into API)

The following commands are wand-specific and should remain separate from the cross-device API:

**Wand Sound/Audio Commands** (~30 commands):
- W_WAND_BOOTUP_SOUND, W_WAND_BOOTUP_SHORT_SOUND, W_WAND_SHUTDOWN_SOUND, W_WAND_MASH_ERROR_SOUND
- W_WAND_BEEP_SOUNDS, W_WAND_BEEP_BARGRAPH, W_WAND_BEEP_STOP, W_WAND_BEEP_STOP_LOOP, W_WAND_BEEP_START, W_WAND_BEEP
- W_MASH_ERROR_LOOP, W_MASH_ERROR_RESTART
- W_BOSON_DART_SOUND, W_SHOCK_BLAST_SOUND, W_SLIME_TETHER_SOUND, W_MESON_COLLIDER_SOUND, W_MESON_FIRE_PULSE, W_IMPACT_SOUND
- W_MODE_ORIGINAL_HEATUP_STOP, W_MODE_ORIGINAL_HEATUP, W_MODE_ORIGINAL_HEATDOWN_STOP, W_MODE_ORIGINAL_HEATDOWN
- W_BEEPS_ALT

**Wand Hardware Configuration** (~15 commands):
- W_BARGRAPH_28_SEGMENTS, W_BARGRAPH_30_SEGMENTS
- W_BARREL_ERROR_SOUND, W_BARREL_SWITCH_DEFAULT, W_BARREL_SWITCH_INVERTED, W_BARREL_SWITCH_DISABLED
- W_WAND_BOOTUP_1989, W_GB1_WAND_BARREL_EXTEND, W_AFTERLIFE_WAND_BARREL_EXTEND, W_WAND_BARREL_RETRACT

**Wand Timing/Duration Commands** (~10 commands):
- W_SOUND_OVERHEAT_SMOKE_DURATION_LEVEL_1-5 (5 commands)
- W_SOUND_OVERHEAT_START_TIMER_LEVEL_1-5 (5 commands)

**Action**: These should remain in WAND_MESSAGE enum but NOT be added to API_MESSAGE. They are device-internal commands, not cross-device API calls. Consider creating a separate `WAND_INTERNAL_MESSAGE` enum or documentation marking to clarify scope.

---

### Implementation Order

1. **Start with Phase 1, Item 1 (Continuous Smoke)**: Highest impact (14 slots), well-understood pattern.
2. **Continue Phase 1, Items 2-3**: High impact items (OVERHEAT_LEVELS, POWER_LEVELS).
3. **Phase 1, Items 4-11**: Remaining enable/disable consolidations in order of impact.
4. **Phase 2**: Address consistency gaps (QUICK_BOOTUP, GPSTAR_AUDIO_LED).
5. **Phase 3**: Document and separate wand-internal commands (no code changes needed; just documentation/scoping).

**Expected Final Result**:

- Current: 94 A_ commands + 119 P_ commands + 235 W_ commands = 448 total (exceeds uint8 limit per device)
- After Phase 1 & 2: ~94 + 48 + 3 = 145 A_ commands, P_/W_ reduced to ~40 legacy/deprecated, Net API commands under 254 ✓

---

## Phase 4: ENABLED/DISABLED Pair Consolidation (Proposed)

### Overview
The API_COMMAND enum still contains **16 ENABLED/DISABLED binary pairs** that consume **32 enum slots**. These can be consolidated into 16 A_SET_* enums with d1 parameters (0=DISABLED, 1=ENABLED).

**Target Impact**: ~16 slots freed in API_COMMAND (bringing it from ~288 to ~272)
**Result**: All configuration toggles use consistent A_SET_* pattern with d1 values

### Consolidation Candidates

#### Category 1: ENABLED/DISABLED Configuration Pairs (12 consolidations)
These are feature toggles with clear binary states:

| Current Pair | Proposed A_SET_* | d1 Values | Slots Saved |
|---|---|---|---|
| A_QUICK_VENT_ENABLED / A_QUICK_VENT_DISABLED | A_SET_QUICK_VENT | 0=DISABLED, 1=ENABLED | 1 |
| A_BOOTUP_ERRORS_ENABLED / A_BOOTUP_ERRORS_DISABLED | A_SET_BOOTUP_ERRORS | 0=DISABLED, 1=ENABLED | 1 |
| A_SMOKE_DISABLED / A_SMOKE_ENABLED | A_SET_SMOKE | 0=DISABLED, 1=ENABLED | 1 |
| A_BARGRAPH_OVERHEAT_BLINK_ENABLED / A_BARGRAPH_OVERHEAT_BLINK_DISABLED | A_SET_BARGRAPH_OVERHEAT_BLINK | 0=DISABLED, 1=ENABLED | 1 |
| A_MODE_BEEP_LOOP_ENABLED / A_MODE_BEEP_LOOP_DISABLED | A_SET_MODE_BEEP_LOOP | 0=DISABLED, 1=ENABLED | 1 |
| A_RGB_VENT_DISABLED / A_RGB_VENT_ENABLED | A_SET_RGB_VENT | 0=DISABLED, 1=ENABLED | 1 |
| A_AUTO_VENT_INTENSITY_DISABLED / A_AUTO_VENT_INTENSITY_ENABLED | A_SET_AUTO_VENT_INTENSITY | 0=DISABLED, 1=ENABLED | 1 |
| A_VENT_LIGHT_COLOURS_DISABLED / A_VENT_LIGHT_COLOURS_ENABLED | A_SET_VENT_LIGHT_COLOURS | 0=DISABLED, 1=ENABLED | 1 |
| A_WAND_WIFI_DISABLED / A_WAND_WIFI_ENABLED | A_SET_WAND_WIFI | 0=DISABLED, 1=ENABLED | 1 |
| A_VIDEO_GAME_MODE_COLOURS_ENABLED / A_VIDEO_GAME_MODE_COLOURS_DISABLED | A_SET_VIDEO_GAME_MODE_COLOURS | 0=DISABLED, 1=ENABLED | 1 |
| A_SPECTRAL_MODES_ENABLED / A_SPECTRAL_MODES_DISABLED | A_SET_SPECTRAL_MODES | 0=DISABLED, 1=ENABLED | 1 |
| A_VOICE_NEUTRONA_WAND_SOUNDS_ENABLED / A_VOICE_NEUTRONA_WAND_SOUNDS_DISABLED | A_SET_VOICE_NEUTRONA_WAND_SOUNDS | 0=DISABLED, 1=ENABLED | 1 |

#### Category 2: ON/OFF and INVERTED Pairs (4 consolidations)
These follow similar binary logic:

| Current Pair | Proposed A_SET_* | d1 Values | Slots Saved |
|---|---|---|---|
| A_SPECTRAL_LIGHTS_OFF / A_SPECTRAL_LIGHTS_ON | A_SET_SPECTRAL_LIGHTS | 0=OFF, 1=ON | 1 |
| A_OVERHEATING_DISABLED / A_OVERHEATING_ENABLED | A_SET_OVERHEATING | 0=DISABLED, 1=ENABLED | 1 |
| A_BARGRAPH_NOT_INVERTED / A_BARGRAPH_INVERTED | A_SET_BARGRAPH_INVERT | 0=NOT_INVERTED, 1=INVERTED | 1 |
| A_POWERCELL_NOT_INVERTED / A_POWERCELL_INVERTED | A_SET_POWERCELL_INVERT | 0=NOT_INVERTED, 1=INVERTED | 1 |

**Subtotal: 16 slots saved**

### Status Messages (Recommended to KEEP as pairs)
These appear to be device state reporting rather than configuration commands. Recommend keeping as discrete enums:

- A_PACK_ON / A_PACK_OFF (device power state)
- A_WAND_ON / A_WAND_OFF (device power state)
- A_CYCLOTRON_LID_ON / A_CYCLOTRON_LID_OFF (sensor position)
- A_ALARM_ON / A_ALARM_OFF (alert state)
- A_ION_ARM_SWITCH_ON / A_ION_ARM_SWITCH_OFF (switch position)
- A_TURN_PACK_ON / A_TURN_PACK_OFF (power transition)
