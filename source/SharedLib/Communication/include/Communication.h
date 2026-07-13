/**
 *   Communications - Serial communication packet definitions for GPStar devices.
 *   Provides common objects and enums for serial data exchange.
 *   Copyright (C) 2023-2026 Michael Rajotte, Dustin Grau, Nomake Wan
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
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * Devices send serial (UART) data using SerialTransfer and packed structs.
 *
 * SerialTransfer always adds 6 bytes around each packet:
 *   - 4 bytes at the start
 *   - 2 bytes at the end
 *
 * Common packet data sizes for commands and messages:
 *   - CommandPacket = 6 packet bytes (s, c, d1, e)
 *   - DataPacket = 7 packet bytes (s, c, d[3], e)
 *   - Total bytes sent for these are 12 & 13 (packet bytes + 6 overhead).
 *
 * Special packet data sizes for preferences and synchronization:
 *   - PackPrefs = 26 packet bytes, 32 total bytes sent
 *   - WandPrefs = 16 packet bytes, 22 total bytes sent
 *   - SmokePrefs = 12 packet bytes, 18 total bytes sent
 *   - WandSyncData = 9 packet bytes, 15 total bytes sent
 *   - AttenuatorSyncData = 24 packet bytes, 30 total bytes sent
 *   - These all fit within one SerialTransfer packet.
 *
 * Byte order notes:
 *   - Supported serial devices here are little-endian (ATMega + ESP32).
 *   - Multi-byte values (for example uint16_t) are copied as-is.
 *
 * Timing and packet timeout:
 *   - Device-to-device serial (UART) links run at 9600 baud.
 *   - At 9600 baud, one byte takes about 1.04 ms to transmit.
 *   - UART frame format: 8 data bits, no parity, 1 stop bit (8N1).
 *   - SerialTransfer default packet timeout is 50 ms.
 *   - Proton Pack <-> Attenuator uses 100 ms timeout.
 *   - One sendData() call sends one SerialTransfer packet.
 *   - Time to finish sending depends on the total packet size and baud rate.
 *
 * Example device-to-device transmit times at 9600 baud:
 *   - Common command/message packets:
 *       CommandPacket: 12 total bytes, about 12.5 ms
 *       DataPacket: 13 total bytes, about 13.5 ms
 *   - Special preferences/sync packets:
 *       PackPrefs: 32 total bytes, about 33.3 ms (Pack <-> Attenuator)
 *       WandPrefs: 22 total bytes, about 22.9 ms (Pack <-> Attenuator, Pack <-> Wand)
 *       SmokePrefs: 18 total bytes, about 18.8 ms (Pack <-> Attenuator, Pack <-> Wand)
 *       WandSyncData: 15 total bytes, about 15.6 ms (Pack <-> Wand)
 *       AttenuatorSyncData: 30 total bytes, about 31.3 ms (Pack <-> Attenuator)
 *
 * Send timing:
 *   - SerialTransfer does not add a pause between packets.
 *   - Packets are queued to the UART transmit buffer (TX buffer) right away.
 *   - The actual gap depends on serial speed and how often code calls send.
 *
 * Receive timing and buffers:
 *   - SerialTransfer does not run its own receive timer.
 *   - Data is parsed when firmware calls available() from its loop/task checks.
 *   - On Pack and Wand builds, these checks happen once per main loop pass on both
 *     ESP32 and ATMega targets. On ESP32, the loop also yields briefly (~1 ms).
 *   - On the Attenuator, serial checks run in a dedicated task with a 2 ms delay
 *     between passes.
 *   - SerialTransfer packet buffers are fixed at 254 bytes each:
 *       txBuff[254] for payload staging and rxBuff[254] for parsed payload data.
 *   - SerialTransfer rxBuff is a packet buffer, not a circular (ring) buffer.
 *   - Pending packets are kept in the UART receive buffer and are handled in the
 *     order they arrive.
 *   - Each poll normally parses and exposes one completed packet, then the next
 *     packet is handled on a later loop/task pass.
 *   - UART driver RX/TX buffer sizes are not set in this file and may vary by board/core defaults.
 */

// Types of packets to be sent via serial communication.
enum PACKET_TYPE : uint8_t {
  PACKET_UNKNOWN = 0, // Unknown packet type (error)
  PACKET_COMMAND = 1, // Command with optional data value
  PACKET_DATA  = 2,   // Command with up to 3 data values
  PACKET_PACK  = 3,   // Proton Pack preferences/settings
  PACKET_WAND  = 4,   // Neutrona Wand preferences/settings
  PACKET_SMOKE = 5,   // Smoke/Overheat preferences/settings
  PACKET_SYNC  = 6    // Synchronization data
};

// For command signals (2 byte ID, 2 byte optional data).
struct __attribute__((packed)) CommandPacket {
  uint8_t s;
  uint16_t c; // Command enum (uint16_t supports up to 65,535 commands)
  uint16_t d1; // Reserved for values over 255 (eg. current music track)
  uint8_t e;
};

// For custom data communication (1 byte ID, 3 byte array).
struct __attribute__((packed)) DataPacket {
  uint8_t s;
  uint16_t c; // Command enum (uint16_t supports up to 65,535 commands)
  uint8_t d[3]; // Reserved for multiple, arbitrary byte values.
  uint8_t e;
};

/*
 * These enum definitions must be kept in sync across the devices they communicate with, using the same dataype and ordering.
 * Enum values are internally considered integer values and here they are being given a distinct underlying datatype of uint16_t.
 * It is therefore important that the total number of elements per enum must remain below 65535 to not overflow that (word) type.
 */

 // Specifically for device synchronization.
enum DEVICE_ID : uint8_t {
  A_COM_START = 0,
  A_COM_END = 1
};

/**
 * API_COMMAND: Command/status enums for PACKET_COMMAND type messages.
 * These enums represent discrete state changes and actions, some of
 * which may pass a single uint16_t value (or ENUM) as a parameter.
 */
enum API_COMMAND : uint16_t {
  A_CMD_NULL, // Special case for index 0 (avoiding an implicit or erroneous command).
  // Device initialization and synchronization commands.
  A_HANDSHAKE, // d1: PROTOCOL_SIGNATURE
  A_SYNC_NOW, // d1: PROTOCOL_SIGNATURE
  A_SYNC_START, // d1: PROTOCOL_SIGNATURE or post-finish state (1/2)
  A_SYNC_DATA, // Sends PACKET_SYNC data struct
  A_SPECTRAL_COLOUR_DATA, // Sends PACKET_DATA with d1: colour, d2: saturation
  A_VOLUME_SYNC, // Sends PACKET_DATA with d1: master, d2: effects, d3: music
  A_SYNC_END,
  A_SYNCHRONIZED,
  A_POST_FINISH,
  // Device State Change Commands
  A_PACK_ON,
  A_PACK_OFF, // d1: shutdown state (0=NORMAL_OFF, 1=SHUTTING_DOWN)
  A_TURN_PACK_ON,
  A_TURN_PACK_OFF,
  A_TURN_WAND_ON,
  A_WAND_CONNECTED,
  A_WAND_DISCONNECTED,
  A_WAND_ON, // d1: wand-on mode state (0=SUPER_HERO_SPECIAL, 1=NORMAL)
  A_WAND_OFF,
  // Firing States
  A_STREAM_FLAGS, // d1: stream option bitfield
  A_FIRING, // d1: firing mode/state (1/2)
  A_FIRING_STOPPED,
  A_FIRING_CTS,
  A_FIRING_CTS_STOPPED,
  // Overheat and Venting Commands
  A_CYCLOTRON_NORMAL_SPEED,
  A_CYCLOTRON_INCREASE_SPEED,
  A_OVERHEATING,
  A_OVERHEATING_FINISHED,
  A_VENTING,
  A_VENTING_FINISHED,
  A_WARNING_CANCELLED,
  A_MANUAL_OVERHEAT,
  A_MANUAL_QUICK_VENT,
  A_BUTTON_MASHING, // d1: timeout value
  A_MASH_ERROR_LOOP,
  A_MASH_ERROR_RESTART,
  A_SYSTEM_LOCKOUT, // d1: timeout value
  A_CANCEL_LOCKOUT,
  // Device Error/Warning States
  A_CYCLOTRON_LID_ON,
  A_CYCLOTRON_LID_OFF,
  A_ALARM_ON, // d1: ribbon cable state (0/1)
  A_ALARM_OFF, // d1: ribbon cable state (0/1)
  // Music and Volume Controls
  A_MUSIC_START_STOP,
  A_MUSIC_PAUSE_RESUME,
  A_MUSIC_NEXT_TRACK,
  A_MUSIC_PREV_TRACK,
  A_MUSIC_TOGGLE,
  A_MUSIC_PLAY_TRACK, // d1: track number
  A_MUSIC_TRACK_LOOP_STATUS, // d1: loop state (1=OFF, 2=ON)
  A_MUSIC_TRACK_LOOP_TOGGLE, // d1: loop toggle request (1=OFF, 2=ON)
  A_MUSIC_TRACK_SHUFFLE_STATUS, // d1: shuffle state (1=OFF, 2=ON)
  A_MUSIC_TRACK_SHUFFLE_TOGGLE, // d1: shuffle toggle request (1=OFF, 2=ON)
  A_MUSIC_IS_PLAYING, // d1: current track number
  A_MUSIC_IS_NOT_PLAYING, // d1: current track number
  A_MUSIC_IS_PAUSED,
  A_MUSIC_IS_NOT_PAUSED,
  A_MUSIC_STATUS, // d1: music playback state (1/2/3/4)
  A_TOGGLE_MUTE, // d1: mute toggle state (1=OFF, 2=ON)
  A_VOLUME_SET, // d1: volume value (0-255)
  A_VOLUME_INCREASE,
  A_VOLUME_INCREASE_EEPROM,
  A_VOLUME_DECREASE,
  A_VOLUME_DECREASE_EEPROM,
  A_VOLUME_MUSIC_INCREASE,
  A_VOLUME_MUSIC_DECREASE,
  A_VOLUME_SOUND_EFFECTS_INCREASE,
  A_VOLUME_SOUND_EFFECTS_DECREASE,
  A_PROTON_PACK_VOLUME_ADJUSTMENT,
  A_NEUTRONA_WAND_VOLUME_ADJUSTMENT,
  A_WAND_AUDIO_VERSION, // d1: wand audio version number
  // EEPROM Management
  A_SAVE_EEPROM_SETTINGS_PACK,
  A_SAVE_EEPROM_SETTINGS_WAND,
  A_RESET_EEPROM_SETTINGS_PACK,
  A_RESET_EEPROM_SETTINGS_WAND,
  A_CLEAR_CONFIG_EEPROM_SETTINGS,
  A_SAVE_CONFIG_EEPROM_SETTINGS,
  A_CLEAR_LED_EEPROM_SETTINGS,
  A_SAVE_LED_EEPROM_SETTINGS,
  // Theme Selections
  A_MODE_1984,
  A_MODE_1989,
  A_MODE_AFTERLIFE,
  A_MODE_FROZEN_EMPIRE,
  A_YEAR_1984,
  A_YEAR_1989,
  A_YEAR_AFTERLIFE,
  A_YEAR_FROZEN_EMPIRE,
  A_YEAR_MODES_CYCLE,
  A_YEAR_MODES_CYCLE_EEPROM,
  // Device Behaviors
  A_BARGRAPH_28_SEGMENTS,
  A_BARGRAPH_30_SEGMENTS,
  A_BARREL_EXTENDED,
  A_BARREL_RETRACTED,
  A_BATTERY_VOLTAGE_PACK, // d1: battery voltage x100
  A_CROSS_THE_STREAMS,
  A_CROSS_THE_STREAMS_MIX,
  A_CTS_1984,
  A_CTS_AFTERLIFE,
  A_CTS_DEFAULT,
  A_CYCLOTRON_CLOCKWISE,
  A_CYCLOTRON_COUNTER_CLOCKWISE,
  A_CYCLOTRON_DIMMING,
  A_CYCLOTRON_DIRECTION_TOGGLE, // d1: direction toggle state (1=COUNTER_CLOCKWISE, 2=CLOCKWISE)
  A_CYCLOTRON_LED_TOGGLE,
  A_CYCLOTRON_PANEL_DIMMING,
  A_CYCLOTRON_SINGLE_LED,
  A_CYCLOTRON_THREE_LED,
  A_DEFAULT_BARGRAPH,
  A_DEFAULT_FIRING_ANIMATIONS_BARGRAPH,
  A_DIMMING,
  A_DIMMING_DECREASE,
  A_DIMMING_INCREASE,
  A_DIMMING_TOGGLE,
  A_FIRING_ALT_MIX,
  A_FIRING_ALT_STOPPED_MIX,
  A_FIRING_CROSSING_THE_STREAMS_1984,
  A_FIRING_CROSSING_THE_STREAMS_2021,
  A_FIRING_CROSSING_THE_STREAMS_MIX_1984,
  A_FIRING_CROSSING_THE_STREAMS_MIX_2021,
  A_FIRING_CROSSING_THE_STREAMS_STOPPED_MIX_1984,
  A_FIRING_CROSSING_THE_STREAMS_STOPPED_MIX_2021,
  A_FIRING_INTENSIFY_MIX,
  A_FIRING_INTENSIFY_STOPPED_MIX,
  A_GB1_WAND_BARREL_EXTEND,
  A_GRB_INNER_CYCLOTRON_LEDS,
  A_INNER_CYCLOTRON_DIMMING,
  A_INNER_CYCLOTRON_PANEL_DISABLED,
  A_INNER_CYCLOTRON_PANEL_DYNAMIC,
  A_INNER_CYCLOTRON_PANEL_STATIC,
  A_ION_ARM_SWITCH_ON,
  A_ION_ARM_SWITCH_OFF,
  A_MASTER_AUDIO_STATUS, // d1: master mute state (1=OFF, 2=ON)
  A_MESON_FIRE_PULSE,
  A_MODE_ORIGINAL,
  A_MODE_ORIGINAL_BARGRAPH,
  A_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH,
  A_MODE_ORIGINAL_HEATDOWN,
  A_MODE_ORIGINAL_HEATDOWN_STOP,
  A_MODE_ORIGINAL_HEATUP,
  A_MODE_ORIGINAL_HEATUP_STOP,
  A_MODE_SUPER_HERO,
  A_MODE_TOGGLE,
  A_NEUTRONA_WAND_1984_MODE,
  A_NEUTRONA_WAND_1989_MODE,
  A_NEUTRONA_WAND_AFTERLIFE_MODE,
  A_NEUTRONA_WAND_DEFAULT_MODE,
  A_NEUTRONA_WAND_FROZEN_EMPIRE_MODE,
  A_OVERHEAT_DECREASE_LEVEL_1,
  A_OVERHEAT_DECREASE_LEVEL_2,
  A_OVERHEAT_DECREASE_LEVEL_3,
  A_OVERHEAT_DECREASE_LEVEL_4,
  A_OVERHEAT_DECREASE_LEVEL_5,
  A_OVERHEAT_INCREASE_LEVEL_1,
  A_OVERHEAT_INCREASE_LEVEL_2,
  A_OVERHEAT_INCREASE_LEVEL_3,
  A_OVERHEAT_INCREASE_LEVEL_4,
  A_OVERHEAT_INCREASE_LEVEL_5,
  A_POWERCELL_DIMMING,
  A_RGB_INNER_CYCLOTRON_LEDS,
  A_SMOKE_TOGGLE,
  A_SPECTRAL_CYCLOTRON_CUSTOM_DECREASE,
  A_SPECTRAL_CYCLOTRON_CUSTOM_INCREASE,
  A_SPECTRAL_INNER_CYCLOTRON_CUSTOM_DECREASE,
  A_SPECTRAL_INNER_CYCLOTRON_CUSTOM_INCREASE,
  A_SPECTRAL_POWERCELL_CUSTOM_DECREASE,
  A_SPECTRAL_POWERCELL_CUSTOM_INCREASE,
  A_SUPER_HERO_BARGRAPH,
  A_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH,
  A_TEMPERATURE_PACK, // d1: temperature celsius x100
  A_TOGGLE_INNER_CYCLOTRON_PANEL,
  A_TOGGLE_POWERCELL_DIRECTION,
  A_TOGGLE_RGB_INNER_CYCLOTRON_LEDS,
  A_TOGGLE_SMOKE, // d1: smoke toggle state (1=OFF, 2=ON)
  A_TOGGLE_VIBRATION, // d1: vibration toggle state (1=OFF, 2=ON)
  A_VIDEO_GAME_MODE,
  A_VIDEO_GAME_MODE_COLOUR_TOGGLE,
  A_VIDEO_GAME_MODE_CYCLOTRON_ENABLED,
  A_VIDEO_GAME_MODE_POWER_CELL_ENABLED,
  A_WAND_BARREL_RETRACT,
  A_WAND_BEEP_BARGRAPH,
  A_WAND_BOOTUP_1989,
  A_WAND_POWER_AMPS, // d1: wand power draw value
  A_YEAR_MODE_DEFAULT,
  // WIFI Management
  A_RESET_WIFI_PASSWORD,
  A_TOGGLE_PACK_WIFI,
  A_WAND_WIFI_RESET,
  // Device Preferences
  A_REQUEST_PREFERENCES_PACK,
  A_REQUEST_PREFERENCES_WAND,
  A_REQUEST_PREFERENCES_SMOKE,
  A_SEND_PREFERENCES_PACK, // PACKET_PACK data struct
  A_SEND_PREFERENCES_WAND, // PACKET_WAND data struct
  A_SEND_PREFERENCES_SMOKE, // PACKET_SMOKE data struct
  A_SAVE_PREFERENCES_PACK,
  A_SAVE_PREFERENCES_WAND,
  A_SAVE_PREFERENCES_SMOKE,
  // Sound Effects/Voices
  A_BEEPS_ALT,
  A_BEEP_START,
  A_BOSON_DART_SOUND,
  A_MESON_COLLIDER_SOUND,
  A_SHOCK_BLAST_SOUND,
  A_SLIME_TETHER_SOUND,
  A_WAND_BOOTUP_SHORT_SOUND,
  A_WAND_BOOTUP_SOUND,
  A_WAND_MASH_ERROR_SOUND,
  A_WAND_SHUTDOWN_SOUND,
  A_AFTERLIFE_GUN_LOOP_1,
  A_AFTERLIFE_GUN_LOOP_2,
  A_AFTERLIFE_GUN_RAMP_1,
  A_AFTERLIFE_GUN_RAMP_2,
  A_AFTERLIFE_GUN_RAMP_2_FADE_IN,
  A_AFTERLIFE_GUN_RAMP_DOWN_1,
  A_AFTERLIFE_GUN_RAMP_DOWN_2,
  A_AFTERLIFE_GUN_RAMP_DOWN_2_FADE_OUT,
  A_AFTERLIFE_RAMP_LOOP_2_STOP,
  A_AFTERLIFE_WAND_BARREL_EXTEND,
  A_BARREL_ERROR_SOUND,
  A_EXTRA_WAND_SOUNDS_STOP,
  A_IMPACT_SOUND, // d1: impact sound effect ID
  A_COM_SOUND_NUMBER, // d1: sound effect ID
  A_REQUEST_BEEP_SYNC,
  A_SAY_EEPROM_CONFIG_MENU,
  A_SAY_EEPROM_LED_MENU,
  A_SOUND_SUPER_HERO,
  A_SOUND_MODE_ORIGINAL,
  A_SOUND_OVERHEAT_SMOKE_DURATION, // d1: POWER_LEVELS (ENUM)
  A_SOUND_OVERHEAT_START_TIMER, // d1: POWER_LEVELS (ENUM)
  A_WAND_BEEP,
  A_WAND_BEEP_START,
  A_WAND_BEEP_STOP,
  A_WAND_BEEP_STOP_LOOP,
  A_WAND_BEEP_SOUNDS,
  // Setter Commands
  A_SAY_MENU_LEVEL, // d1: 1-5 (Menu Levels)
  A_SET_AUTO_VENT_INTENSITY, // d1: 0=DISABLED, 1=ENABLED
  A_SET_BARGRAPH_INVERT, // d1: 0=NOT_INVERTED, 1=INVERTED
  A_SET_BARGRAPH_OVERHEAT_BLINK, // d1: 0=DISABLED, 1=ENABLED
  A_SET_BARREL_SWITCH, // d1: 1=DEFAULT, 2=INVERTED, 3=DISABLED
  A_SET_BARREL_TYPE, // d1: 1=HASBRO, 2=FRUTTO, 3=GPSTAR, 4=GPSTAR_II, 5=GPSTAR_MINI
  A_SET_BOOTUP_ERRORS, // d1: 0=DISABLED, 1=ENABLED
  A_SET_CONTINUOUS_SMOKE_1, // d1: 0=DISABLED, 1=ENABLED
  A_SET_CONTINUOUS_SMOKE_2, // d1: 0=DISABLED, 1=ENABLED
  A_SET_CONTINUOUS_SMOKE_3, // d1: 0=DISABLED, 1=ENABLED
  A_SET_CONTINUOUS_SMOKE_4, // d1: 0=DISABLED, 1=ENABLED
  A_SET_CONTINUOUS_SMOKE_5, // d1: 0=DISABLED, 1=ENABLED
  A_SET_CYCLOTRON_FADING, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_CYCLOTRON_LED_COUNT, // d1: 12, 20, 36, 40 or 2=TOGGLE
  A_SET_CYCLOTRON_SIMULATE_RING, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_DEMO_LIGHT_MODE, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_FIRING_MODE, // d1: FLAG_VG_MODE, FLAG_CTS_MODE, FLAG_CTS_MIX_MODE
  A_SET_INNER_CYCLOTRON_LED_COUNT, // d1: 12, 23, 24, 26, 35, 36 or 2=TOGGLE
  A_SET_MODE_BEEP_LOOP, // d1: 0=DISABLED, 1=ENABLED
  A_SET_OVERHEAT_LEVEL_1, // d1: 0=DISABLED, 1=ENABLED
  A_SET_OVERHEAT_LEVEL_2, // d1: 0=DISABLED, 1=ENABLED
  A_SET_OVERHEAT_LEVEL_3, // d1: 0=DISABLED, 1=ENABLED
  A_SET_OVERHEAT_LEVEL_4, // d1: 0=DISABLED, 1=ENABLED
  A_SET_OVERHEAT_LEVEL_5, // d1: 0=DISABLED, 1=ENABLED
  A_SET_OVERHEAT_LIGHTS_OFF, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_OVERHEAT_STROBE, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_OVERHEAT_SYNC_FAN, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_OVERHEATING, // d1: 0=DISABLED, 1=ENABLED
  A_SET_PACK_GPSTAR_AUDIO_LED, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_PACK_VIBRATION_MODE, // d1: VIBRATION_MODES (ENUM)
  A_SET_POWER_LEVEL, // d1: POWER_LEVELS (ENUM)
  A_SET_POWERCELL_INVERT, // d1: 0=NOT_INVERTED, 1=INVERTED
  A_SET_POWERCELL_LED_COUNT, // d1: 13, 15 or 2=TOGGLE
  A_SET_PROTON_STREAM_IMPACT, // d1: 1=DISABLE_REQ, 2=ENABLE_REQ, 3=DISABLED_STATUS, 4=ENABLED_STATUS
  A_SET_QUICK_BOOTUP, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_QUICK_VENT, // d1: 0=DISABLED, 1=ENABLED
  A_SET_RGB_VENT, // d1: 0=DISABLED, 1=ENABLED
  A_SET_SMOKE, // d1: 0=DISABLED, 1=ENABLED
  A_SET_SPECTRAL_LIGHTS, // d1: 0=OFF, 1=ON
  A_SET_SPECTRAL_MODES, // d1: 0=DISABLED, 1=ENABLED
  A_SET_STREAM_MODE, // d1: STREAM_MODES (ENUM)
  A_SET_VENT_LIGHT_COLOURS, // d1: 0=DISABLED, 1=ENABLED
  A_SET_VIDEO_GAME_MODE_COLOURS, // d1: 0=DISABLED, 1=ENABLED
  A_SET_VIBRATION_MODE, // d1: VIBRATION_MODES (ENUM)
  A_SET_VOICE_NEUTRONA_WAND_SOUNDS, // d1: 0=DISABLED, 1=ENABLED
  A_SET_WAND_GPSTAR_AUDIO_LED, // d1: 0=DISABLED, 1=ENABLED, 2=TOGGLE
  A_SET_WAND_VIBRATION_MODE, // d1: VIBRATION_MODES (ENUM)
  A_SET_WAND_WIFI, // d1: 0=DISABLED, 1=ENABLED
  // End of List
  A_CMD_MAX // Sentinel value to represent the maximum command value.
};

/**
 * Generates a CRC16 value for a given input.
 *
 * Returns a 16-bit CRC hash.
 *
 * Code derived from torusle2 on Reddit:
 * https://www.reddit.com/r/embedded/comments/1acoobg/crc16_again_with_a_little_gift_for_you_all/
 */
uint16_t crc16(const uint8_t *pData, size_t numBytes)
{
	uint32_t crc = 0;

	for (size_t i=0; i<numBytes; i++)
	{
		uint8_t  d = *(pData++);
		uint32_t x = ((crc ^ d) & 0xff) << 8;
		uint32_t y = x;

		x ^= x << 1;
		x ^= x << 2;
		x ^= x << 4;

		x  = (x & 0x8000) | (y >> 1);

		crc = (crc >> 8) ^ (x >> 15) ^ (x >> 1) ^ x;
	}
	return crc;
}

/**
 * Generate a signature value that changes when packet sizes or message types change.
 * This helps detect incompatible firmware versions during device synchronization.
 *
 * This function places the individual sizes of each serial data struct into a
 * byte array, then performs a CRC16 on the array to return a unique hash.
 *
 * Returns a 16-bit signature value.
 *
 * When devices sync, they compare these calculated signature values:
 *   - Matching signatures: firmware is compatible, sync proceeds like normal
 *   - Different signatures: incompatible firmware, block sync and set error flag
 */
const uint16_t calculateProtocolSignature(
  uint8_t cmd_packet_size,
  uint8_t data_packet_size,
  uint8_t pack_prefs_size,
  uint8_t wand_prefs_size,
  uint8_t smoke_prefs_size,
  uint8_t wand_sync_size,
  uint8_t atten_sync_size,
  uint16_t api_cmd_max)
{
  uint8_t data[10] = {
    cmd_packet_size,
    data_packet_size,
    pack_prefs_size,
    wand_prefs_size,
    smoke_prefs_size,
    wand_sync_size,
    atten_sync_size,
    (uint8_t)(api_cmd_max >> 8), // high byte
    (uint8_t)(api_cmd_max & 0xFF)}; // low byte
  return crc16(data, sizeof(data));
}
