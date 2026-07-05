# Device API Flows

This document captures current, code-verified discoveries from serial handlers and send wrappers.

## Alphabetical API Listing with Directional Flow

The following is an alphabetical listing of all `API_COMMAND` enums found in `Communication.h`, with observed direction(s) inferred from send wrappers in the codebase.

| API Name                                         | P --> A | A --> P | P --> W | W --> P |
| ----------------------------------------------   | ------- | ------- | ------- | ------- |
| A_AFTERLIFE_GUN_LOOP_1                           |         |         |         |    X    |
| A_AFTERLIFE_GUN_LOOP_2                           |         |         |         |    X    |
| A_AFTERLIFE_GUN_RAMP_1                           |         |         |         |    X    |
| A_AFTERLIFE_GUN_RAMP_2                           |         |         |         |    X    |
| A_AFTERLIFE_GUN_RAMP_2_FADE_IN                   |         |         |         |    X    |
| A_AFTERLIFE_GUN_RAMP_DOWN_1                      |         |         |         |    X    |
| A_AFTERLIFE_GUN_RAMP_DOWN_2                      |         |         |         |    X    |
| A_AFTERLIFE_GUN_RAMP_DOWN_2_FADE_OUT             |         |         |         |    X    |
| A_AFTERLIFE_RAMP_LOOP_2_STOP                     |         |         |         |    X    |
| A_AFTERLIFE_WAND_BARREL_EXTEND                   |         |         |         |    X    |
| A_ALARM_OFF                                      |    X    |    X    |    X    |         |
| A_ALARM_ON                                       |    X    |    X    |    X    |         |
| A_BARGRAPH_28_SEGMENTS                           |         |         |         |    X    |
| A_BARGRAPH_30_SEGMENTS                           |         |         |         |    X    |
| A_BARREL_ERROR_SOUND                             |         |         |         |    X    |
| A_BARREL_EXTENDED                                |    X    |         |         |    X    |
| A_BARREL_RETRACTED                               |    X    |         |         |    X    |
| A_BATTERY_VOLTAGE_PACK                           |    X    |         |         |         |
| A_BEEPS_ALT                                      |         |         |         |    X    |
| A_BEEP_START                                     |         |         |         |    X    |
| A_BOSON_DART_SOUND                               |         |         |         |    X    |
| A_BUTTON_MASHING                                 |         |         |         |    X    |
| A_CANCEL_LOCKOUT                                 |    X    |    X    |    X    |         |
| A_CLEAR_CONFIG_EEPROM_SETTINGS                   |         |         |         |    X    |
| A_CLEAR_LED_EEPROM_SETTINGS                      |         |         |         |    X    |
| A_COM_SOUND_NUMBER                               |         |         |         |    X    |
| A_CROSS_THE_STREAMS                              |         |         |         |    X    |
| A_CROSS_THE_STREAMS_MIX                          |         |         |         |    X    |
| A_CTS_1984                                       |         |         |         |    X    |
| A_CTS_AFTERLIFE                                  |         |         |         |    X    |
| A_CTS_DEFAULT                                    |         |         |         |    X    |
| A_CYCLOTRON_CLOCKWISE                            |         |         |    X    |         |
| A_CYCLOTRON_COUNTER_CLOCKWISE                    |         |         |    X    |         |
| A_CYCLOTRON_DIMMING                              |         |         |    X    |         |
| A_CYCLOTRON_DIRECTION_TOGGLE                     |    X    |    X    |         |    X    |
| A_CYCLOTRON_INCREASE_SPEED                       |    X    |         |         |    X    |
| A_CYCLOTRON_LED_TOGGLE                           |         |         |         |    X    |
| A_CYCLOTRON_LID_OFF                              |    X    |         |    X    |         |
| A_CYCLOTRON_LID_ON                               |    X    |         |    X    |         |
| A_CYCLOTRON_NORMAL_SPEED                         |    X    |         |         |    X    |
| A_CYCLOTRON_PANEL_DIMMING                        |         |         |    X    |         |
| A_CYCLOTRON_SINGLE_LED                           |         |         |    X    |         |
| A_CYCLOTRON_THREE_LED                            |         |         |    X    |         |
| A_DEFAULT_BARGRAPH                               |         |         |         |    X    |
| A_DEFAULT_FIRING_ANIMATIONS_BARGRAPH             |         |         |         |    X    |
| A_DIMMING                                        |         |         |    X    |         |
| A_DIMMING_DECREASE                               |         |         |         |    X    |
| A_DIMMING_INCREASE                               |         |         |         |    X    |
| A_DIMMING_TOGGLE                                 |         |         |         |    X    |
| A_EXTRA_WAND_SOUNDS_STOP                         |         |         |         |    X    |
| A_FIRING                                         |    X    |         |         |    X    |
| A_FIRING_ALT_MIX                                 |         |         |         |    X    |
| A_FIRING_ALT_STOPPED_MIX                         |         |         |         |    X    |
| A_FIRING_CROSSING_THE_STREAMS_1984               |         |         |         |    X    |
| A_FIRING_CROSSING_THE_STREAMS_2021               |         |         |         |    X    |
| A_FIRING_CROSSING_THE_STREAMS_MIX_1984           |         |         |         |    X    |
| A_FIRING_CROSSING_THE_STREAMS_MIX_2021           |         |         |         |    X    |
| A_FIRING_CROSSING_THE_STREAMS_STOPPED_MIX_1984   |         |         |         |    X    |
| A_FIRING_CROSSING_THE_STREAMS_STOPPED_MIX_2021   |         |         |         |    X    |
| A_FIRING_CTS                                     |    X    |         |         |         |
| A_FIRING_CTS_STOPPED                             |    X    |         |         |         |
| A_FIRING_INTENSIFY_MIX                           |         |         |         |    X    |
| A_FIRING_INTENSIFY_STOPPED_MIX                   |         |         |         |    X    |
| A_FIRING_STOPPED                                 |    X    |         |         |    X    |
| A_GB1_WAND_BARREL_EXTEND                         |         |         |         |    X    |
| A_GRB_INNER_CYCLOTRON_LEDS                       |         |         |    X    |         |
| A_HANDSHAKE                                      |    X    |    X    |    X    |    X    |
| A_IMPACT_SOUND                                   |         |         |         |    X    |
| A_INNER_CYCLOTRON_DIMMING                        |         |         |    X    |         |
| A_INNER_CYCLOTRON_PANEL_DISABLED                 |         |         |    X    |         |
| A_INNER_CYCLOTRON_PANEL_DYNAMIC                  |         |         |    X    |         |
| A_INNER_CYCLOTRON_PANEL_STATIC                   |         |         |    X    |         |
| A_ION_ARM_SWITCH_OFF                             |    X    |         |    X    |         |
| A_ION_ARM_SWITCH_ON                              |    X    |         |    X    |         |
| A_MANUAL_OVERHEAT                                |         |    X    |    X    |         |
| A_MANUAL_QUICK_VENT                              |         |    X    |    X    |         |
| A_MASH_ERROR_LOOP                                |         |         |         |    X    |
| A_MASH_ERROR_RESTART                             |         |         |         |    X    |
| A_MASTER_AUDIO_STATUS                            |         |         |    X    |         |
| A_MESON_COLLIDER_SOUND                           |         |         |         |    X    |
| A_MESON_FIRE_PULSE                               |         |         |         |    X    |
| A_MODE_1984                                      |         |         |    X    |         |
| A_MODE_1989                                      |         |         |    X    |         |
| A_MODE_AFTERLIFE                                 |         |         |    X    |         |
| A_MODE_FROZEN_EMPIRE                             |         |         |    X    |         |
| A_MODE_ORIGINAL                                  |    X    |         |    X    |         |
| A_MODE_ORIGINAL_BARGRAPH                         |         |         |         |    X    |
| A_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH       |         |         |         |    X    |
| A_MODE_ORIGINAL_HEATDOWN                         |         |         |         |    X    |
| A_MODE_ORIGINAL_HEATDOWN_STOP                    |         |         |         |    X    |
| A_MODE_ORIGINAL_HEATUP                           |         |         |         |    X    |
| A_MODE_ORIGINAL_HEATUP_STOP                      |         |         |         |    X    |
| A_MODE_SUPER_HERO                                |    X    |         |    X    |         |
| A_MODE_TOGGLE                                    |         |         |         |    X    |
| A_MUSIC_IS_NOT_PAUSED                            |    X    |         |         |         |
| A_MUSIC_IS_NOT_PLAYING                           |    X    |         |         |         |
| A_MUSIC_IS_PAUSED                                |    X    |         |         |         |
| A_MUSIC_IS_PLAYING                               |    X    |         |         |         |
| A_MUSIC_NEXT_TRACK                               |         |    X    |         |    X    |
| A_MUSIC_PAUSE_RESUME                             |         |    X    |         |         |
| A_MUSIC_PLAY_TRACK                               |         |    X    |         |         |
| A_MUSIC_PREV_TRACK                               |         |    X    |         |    X    |
| A_MUSIC_START_STOP                               |         |    X    |         |         |
| A_MUSIC_STATUS                                   |         |         |    X    |         |
| A_MUSIC_TOGGLE                                   |         |         |         |    X    |
| A_MUSIC_TRACK_LOOP_STATUS                        |         |         |    X    |         |
| A_MUSIC_TRACK_LOOP_TOGGLE                        |    X    |    X    |         |    X    |
| A_MUSIC_TRACK_SHUFFLE_STATUS                     |         |         |    X    |         |
| A_MUSIC_TRACK_SHUFFLE_TOGGLE                     |    X    |    X    |         |    X    |
| A_NEUTRONA_WAND_1984_MODE                        |         |         |         |    X    |
| A_NEUTRONA_WAND_1989_MODE                        |         |         |         |    X    |
| A_NEUTRONA_WAND_AFTERLIFE_MODE                   |         |         |         |    X    |
| A_NEUTRONA_WAND_DEFAULT_MODE                     |         |         |         |    X    |
| A_NEUTRONA_WAND_FROZEN_EMPIRE_MODE               |         |         |         |    X    |
| A_NEUTRONA_WAND_VOLUME_ADJUSTMENT                |         |         |         |    X    |
| A_OVERHEATING                                    |    X    |         |         |    X    |
| A_OVERHEATING_FINISHED                           |    X    |         |    X    |         |
| A_OVERHEAT_DECREASE_LEVEL_1                      |         |         |         |    X    |
| A_OVERHEAT_DECREASE_LEVEL_2                      |         |         |         |    X    |
| A_OVERHEAT_DECREASE_LEVEL_3                      |         |         |         |    X    |
| A_OVERHEAT_DECREASE_LEVEL_4                      |         |         |         |    X    |
| A_OVERHEAT_DECREASE_LEVEL_5                      |         |         |         |    X    |
| A_OVERHEAT_INCREASE_LEVEL_1                      |         |         |         |    X    |
| A_OVERHEAT_INCREASE_LEVEL_2                      |         |         |         |    X    |
| A_OVERHEAT_INCREASE_LEVEL_3                      |         |         |         |    X    |
| A_OVERHEAT_INCREASE_LEVEL_4                      |         |         |         |    X    |
| A_OVERHEAT_INCREASE_LEVEL_5                      |         |         |         |    X    |
| A_PACK_OFF                                       |    X    |         |    X    |         |
| A_PACK_ON                                        |    X    |         |    X    |    X    |
| A_POST_FINISH                                    |         |         |    X    |         |
| A_POWERCELL_DIMMING                              |         |         |    X    |         |
| A_PROTON_PACK_VOLUME_ADJUSTMENT                  |         |         |         |    X    |
| A_REQUEST_BEEP_SYNC                              |         |         |    X    |         |
| A_REQUEST_PREFERENCES_PACK                       |         |    X    |         |         |
| A_REQUEST_PREFERENCES_SMOKE                      |         |    X    |         |         |
| A_REQUEST_PREFERENCES_WAND                       |         |    X    |         |         |
| A_RESET_EEPROM_SETTINGS_PACK                     |         |    X    |         |         |
| A_RESET_EEPROM_SETTINGS_WAND                     |         |    X    |    X    |         |
| A_RESET_WIFI_PASSWORD                            |    X    |         |         |    X    |
| A_RGB_INNER_CYCLOTRON_LEDS                       |         |         |    X    |         |
| A_SAVE_CONFIG_EEPROM_SETTINGS                    |         |         |         |    X    |
| A_SAVE_EEPROM_SETTINGS_PACK                      |         |    X    |         |         |
| A_SAVE_EEPROM_SETTINGS_WAND                      |         |    X    |    X    |         |
| A_SAVE_LED_EEPROM_SETTINGS                       |         |         |         |    X    |
| A_SAVE_PREFERENCES_PACK                          |         |    X    |         |         |
| A_SAVE_PREFERENCES_SMOKE                         |         |    X    |    X    |         |
| A_SAVE_PREFERENCES_WAND                          |         |    X    |    X    |         |
| A_SAY_EEPROM_CONFIG_MENU                         |         |         |         |    X    |
| A_SAY_EEPROM_LED_MENU                            |         |         |         |    X    |
| A_SAY_MENU_LEVEL                                 |         |         |         |    X    |
| A_SEND_PREFERENCES_PACK                          |    X    |         |         |         |
| A_SEND_PREFERENCES_SMOKE                         |    X    |         |    X    |    X    |
| A_SEND_PREFERENCES_WAND                          |    X    |         |    X    |    X    |
| A_SET_AUTO_VENT_INTENSITY                        |         |         |         |    X    |
| A_SET_BARGRAPH_INVERT                            |         |         |         |    X    |
| A_SET_BARGRAPH_OVERHEAT_BLINK                    |         |         |         |    X    |
| A_SET_BARREL_SWITCH                              |         |         |         |    X    |
| A_SET_BARREL_TYPE                                |         |         |         |    X    |
| A_SET_BOOTUP_ERRORS                              |         |         |         |    X    |
| A_SET_CONTINUOUS_SMOKE_1                         |         |         |         |    X    |
| A_SET_CONTINUOUS_SMOKE_2                         |         |         |         |    X    |
| A_SET_CONTINUOUS_SMOKE_3                         |         |         |         |    X    |
| A_SET_CONTINUOUS_SMOKE_4                         |         |         |         |    X    |
| A_SET_CONTINUOUS_SMOKE_5                         |         |         |         |    X    |
| A_SET_CYCLOTRON_FADING                           |         |         |    X    |    X    |
| A_SET_CYCLOTRON_LED_COUNT                        |         |         |         |    X    |
| A_SET_CYCLOTRON_SIMULATE_RING                    |         |         |    X    |    X    |
| A_SET_DEMO_LIGHT_MODE                            |         |         |    X    |    X    |
| A_SET_FIRING_MODE                                |    X    |         |         |    X    |
| A_SET_INNER_CYCLOTRON_LED_COUNT                  |         |         |         |    X    |
| A_SET_MODE_BEEP_LOOP                             |         |         |         |    X    |
| A_SET_OVERHEATING                                |         |         |         |    X    |
| A_SET_OVERHEAT_LEVEL_1                           |         |         |         |    X    |
| A_SET_OVERHEAT_LEVEL_2                           |         |         |         |    X    |
| A_SET_OVERHEAT_LEVEL_3                           |         |         |         |    X    |
| A_SET_OVERHEAT_LEVEL_4                           |         |         |         |    X    |
| A_SET_OVERHEAT_LEVEL_5                           |         |         |         |    X    |
| A_SET_OVERHEAT_LIGHTS_OFF                        |         |         |    X    |    X    |
| A_SET_OVERHEAT_STROBE                            |         |         |    X    |    X    |
| A_SET_OVERHEAT_SYNC_FAN                          |         |         |    X    |    X    |
| A_SET_PACK_GPSTAR_AUDIO_LED                      |         |         |    X    |    X    |
| A_SET_PACK_VIBRATION_MODE                        |         |         |    X    |         |
| A_SET_POWERCELL_INVERT                           |         |         |    X    |         |
| A_SET_POWERCELL_LED_COUNT                        |         |         |         |    X    |
| A_SET_POWER_LEVEL                                |    X    |         |         |    X    |
| A_SET_PROTON_STREAM_IMPACT                       |         |         |         |    X    |
| A_SET_QUICK_BOOTUP                               |         |         |    X    |    X    |
| A_SET_QUICK_VENT                                 |         |         |         |    X    |
| A_SET_RGB_VENT                                   |         |         |         |    X    |
| A_SET_SMOKE                                      |         |         |    X    |         |
| A_SET_SPECTRAL_LIGHTS                            |         |         |         |    X    |
| A_SET_SPECTRAL_MODES                             |         |         |         |    X    |
| A_SET_STREAM_MODE                                |    X    |    X    |    X    |    X    |
| A_SET_VENT_LIGHT_COLOURS                         |         |         |         |    X    |
| A_SET_VIBRATION_MODE                             |         |         |         |    X    |
| A_SET_VIDEO_GAME_MODE_COLOURS                    |         |         |    X    |         |
| A_SET_VOICE_NEUTRONA_WAND_SOUNDS                 |         |         |         |    X    |
| A_SET_WAND_GPSTAR_AUDIO_LED                      |         |         |         |    X    |
| A_SET_WAND_VIBRATION_MODE                        |         |         |    X    |         |
| A_SET_WAND_WIFI                                  |         |         |         |    X    |
| A_SHOCK_BLAST_SOUND                              |         |         |         |    X    |
| A_SLIME_TETHER_SOUND                             |         |         |         |    X    |
| A_SMOKE_TOGGLE                                   |         |         |         |    X    |
| A_SOUND_MODE_ORIGINAL                            |         |         |    X    |         |
| A_SOUND_OVERHEAT_SMOKE_DURATION                  |         |         |         |    X    |
| A_SOUND_OVERHEAT_START_TIMER                     |         |         |         |    X    |
| A_SOUND_SUPER_HERO                               |         |         |    X    |         |
| A_SPECTRAL_COLOUR_DATA                           |    X    |         |         |         |
| A_SPECTRAL_CYCLOTRON_CUSTOM_DECREASE             |         |         |         |    X    |
| A_SPECTRAL_CYCLOTRON_CUSTOM_INCREASE             |         |         |         |    X    |
| A_SPECTRAL_INNER_CYCLOTRON_CUSTOM_DECREASE       |         |         |         |    X    |
| A_SPECTRAL_INNER_CYCLOTRON_CUSTOM_INCREASE       |         |         |         |    X    |
| A_SPECTRAL_POWERCELL_CUSTOM_DECREASE             |         |         |         |    X    |
| A_SPECTRAL_POWERCELL_CUSTOM_INCREASE             |         |         |         |    X    |
| A_STREAM_FLAGS                                   |    X    |         |         |    X    |
| A_SUPER_HERO_BARGRAPH                            |         |         |         |    X    |
| A_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH          |         |         |         |    X    |
| A_SYNCHRONIZED                                   |         |         |         |    X    |
| A_SYNC_DATA                                      |    X    |         |    X    |         |
| A_SYNC_END                                       |    X    |    X    |    X    |         |
| A_SYNC_NOW                                       |         |         |         |    X    |
| A_SYNC_START                                     |    X    |    X    |    X    |         |
| A_SYSTEM_LOCKOUT                                 |    X    |    X    |    X    |         |
| A_TEMPERATURE_PACK                               |    X    |         |         |         |
| A_TOGGLE_INNER_CYCLOTRON_PANEL                   |         |         |         |    X    |
| A_TOGGLE_MUTE                                    |    X    |    X    |         |    X    |
| A_TOGGLE_PACK_WIFI                               |         |         |         |    X    |
| A_TOGGLE_POWERCELL_DIRECTION                     |         |         |         |    X    |
| A_TOGGLE_RGB_INNER_CYCLOTRON_LEDS                |         |         |         |    X    |
| A_TOGGLE_SMOKE                                   |    X    |    X    |         |         |
| A_TOGGLE_VIBRATION                               |    X    |    X    |         |         |
| A_TURN_PACK_OFF                                  |         |    X    |         |         |
| A_TURN_PACK_ON                                   |         |    X    |         |         |
| A_TURN_WAND_ON                                   |         |         |    X    |         |
| A_VENTING                                        |    X    |         |         |    X    |
| A_VENTING_FINISHED                               |    X    |         |    X    |         |
| A_VIDEO_GAME_MODE                                |         |         |         |    X    |
| A_VIDEO_GAME_MODE_COLOUR_TOGGLE                  |         |         |         |    X    |
| A_VIDEO_GAME_MODE_CYCLOTRON_ENABLED              |         |         |    X    |         |
| A_VIDEO_GAME_MODE_POWER_CELL_ENABLED             |         |         |    X    |         |
| A_VOLUME_DECREASE                                |         |    X    |         |    X    |
| A_VOLUME_DECREASE_EEPROM                         |         |         |         |    X    |
| A_VOLUME_INCREASE                                |         |    X    |         |    X    |
| A_VOLUME_INCREASE_EEPROM                         |         |         |         |    X    |
| A_VOLUME_MUSIC_DECREASE                          |         |    X    |         |    X    |
| A_VOLUME_MUSIC_INCREASE                          |         |    X    |         |    X    |
| A_VOLUME_SET                                     |         |    X    |         |         |
| A_VOLUME_SOUND_EFFECTS_DECREASE                  |         |    X    |    X    |    X    |
| A_VOLUME_SOUND_EFFECTS_INCREASE                  |         |    X    |    X    |    X    |
| A_VOLUME_SYNC                                    |    X    |         |         |         |
| A_WAND_AUDIO_VERSION                             |    X    |         |         |    X    |
| A_WAND_BARREL_RETRACT                            |         |         |         |    X    |
| A_WAND_BEEP                                      |         |         |         |    X    |
| A_WAND_BEEP_BARGRAPH                             |         |         |         |    X    |
| A_WAND_BEEP_SOUNDS                               |         |         |         |    X    |
| A_WAND_BEEP_START                                |         |         |         |    X    |
| A_WAND_BEEP_STOP                                 |         |         |         |    X    |
| A_WAND_BEEP_STOP_LOOP                            |         |         |         |    X    |
| A_WAND_BOOTUP_1989                               |         |         |         |    X    |
| A_WAND_BOOTUP_SHORT_SOUND                        |         |         |         |    X    |
| A_WAND_BOOTUP_SOUND                              |         |         |         |    X    |
| A_WAND_CONNECTED                                 |    X    |         |         |         |
| A_WAND_DISCONNECTED                              |    X    |         |         |         |
| A_WAND_MASH_ERROR_SOUND                          |         |         |         |    X    |
| A_WAND_OFF                                       |    X    |         |         |         |
| A_WAND_ON                                        |    X    |         |         |    X    |
| A_WAND_POWER_AMPS                                |    X    |         |         |         |
| A_WAND_SHUTDOWN_SOUND                            |         |         |         |    X    |
| A_WAND_WIFI_RESET                                |         |         |         |    X    |
| A_WARNING_CANCELLED                              |         |    X    |    X    |         |
| A_YEAR_1984                                      |    X    |    X    |    X    |         |
| A_YEAR_1989                                      |    X    |    X    |    X    |         |
| A_YEAR_AFTERLIFE                                 |    X    |    X    |    X    |         |
| A_YEAR_FROZEN_EMPIRE                             |    X    |    X    |    X    |         |
| A_YEAR_MODES_CYCLE                               |         |         |         |    X    |
| A_YEAR_MODES_CYCLE_EEPROM                        |         |         |         |    X    |
| A_YEAR_MODE_DEFAULT                              |         |         |    X    |         |
---
