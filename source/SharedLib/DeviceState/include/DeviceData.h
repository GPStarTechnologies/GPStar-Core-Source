/**
 *   DevicePrefs - Defines device preference structs.
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

// Includes for all ENUM declarations.
#include "Streams.h"
#include "Themes.h"
#include "Vibration.h"

// Preferences for the Proton Pack device.
// All bool values are treated as bits and purposefully placed at the end of the struct to ensure proper and efficient packing.
struct __attribute__((packed)) PackPrefs {
  SYSTEM_MODES defaultSystemModePack = MODE_SUPER_HERO; // [1=SuperHero,2=ModeOriginal]
  SYSTEM_THEMES defaultYearThemePack = SYSTEM_TOGGLE_SWITCH; // [1=TOGGLE,2=1984,3=1989,4=AFTERLIFE,5=FROZEN_EMPIRE]
  SYSTEM_THEMES currentYearThemePack = SYSTEM_AFTERLIFE; // [2=1984,3=1989,4=AFTERLIFE,5=FROZEN_EMPIRE]
  VIBRATION_MODES packVibration = VIBRATION_DEFAULT; // [1=ALWAYS,2=FIRING_ONLY,3=NEVER,4=DEFAULT,5=CYCLOTRON_MOTOR]
  uint8_t defaultPackVolume = 50; // 5-100
  uint8_t fadeoutIdleDelay = 30; // 20-60
  uint8_t ledCycLidCount = 12; // Cyclotron Lid LED Count [12,20,36,40]
  uint8_t ledCycLidHue = 2; // Spectral custom colour/hue 2-254
  uint8_t ledCycLidSat = 2; // Spectral custom saturation 2-254
  uint8_t ledCycLidLum = 50; // Brightness 20-100
  uint8_t ledCycLidCenter = 0; // [0=3 LED, 1=1 LED]
  uint8_t ledCycInnerPanel = 1; // [1=Individual,2=RGB-Static,3=RGB-Dynamic]
  uint8_t ledCycPanLum = 50; // Brightness 0-100
  uint8_t ledCycCakeCount = 12; // [12,23,24,26,35,36]
  uint8_t ledCycCakeHue = 2; // Spectral custom colour/hue 2-254
  uint8_t ledCycCakeSat = 2; // Spectral custom saturation 2-254
  uint8_t ledCycCakeLum = 50; // Brightness 20-100
  uint8_t ledCycCavCount = 0; // Cyclotron cavity LEDs (0-20)
  uint8_t ledCycCavType = 0; // Cyclotron cavity LED Type
  uint8_t ledPowercellCount = 13; //[13,15]
  uint8_t ledPowercellHue = 2; // Spectral custom colour/hue 2-254
  uint8_t ledPowercellSat = 2; // Spectral custom saturation 2-254
  uint8_t ledPowercellLum = 50; // Brightness 20-100
  bool isESP32 : 1;
  bool fadeoutIdleSounds : 1;
  bool ribbonCableAlarm : 1; // Ignore ribbon cable switch state
  bool wandQuickBootup : 1;
  bool cyclotronDirection : 1;
  bool demoLightMode : 1;
  bool brassStartupLoop : 1;
  bool overheatStrobeNF : 1;
  bool overheatSyncToFan : 1;
  bool overheatLightsOff : 1;
  bool ledCycLidFade : 1;
  bool ledCycLidSimRing : 1;
  bool disableLidDetection : 1; // Ignore cyclotron lid switch state
  bool ledCycCakeGRB : 1;
  bool ledCycCakeInvert : 1; // Inner Cyclotron cake animation inverted
  bool ledCycCavInvert : 1; // Cyclotron cavity animation inverted
  bool ledVGCyclotron : 1;
  bool ledInvertPowercell : 1;
  bool ledVGPowercell : 1;
  bool audioVolumeBoosted : 1; // Whether audio is boosted by +10dB or not
  bool gpstarAudioLed : 1;
  bool isWiFiEnabled : 1; // WiFi Enabled (true) or Disabled (false)
  bool resetWifiPassword : 1;
};

// Output a compiler message if the final struct exceeds a specific size needed for SerialTransfer.
static_assert(sizeof(PackPrefs) < 85, "WARNING: PackPrefs has grown too large (>84 bytes)");

// Preferences for the Neutrona Wand device.
// All bool values are treated as bits and purposefully placed at the end of the struct to ensure proper and efficient packing.
struct __attribute__((packed)) WandPrefs {
  uint8_t ledWandCount = 3; // [1=Hasbro,2=Frutto,3=GPStar Barrel,4=GPStar Barrel II,5=GPStar Barrel Mini]
  uint8_t ledWandHue = 2; // Spectral custom colour/hue 2-254
  uint8_t ledWandSat = 2; // Spectral custom saturation 2-254
  uint8_t streamFlags = FLAG_PROTON; // Represents STREAM_MODE_FLAGS (managed by a DeviceState)
  uint8_t defaultStreamMode = PROTON;
  uint8_t defaultFiringMode = 0; // [0=VG,1=CTS,3=CTS_MIX]
  VIBRATION_MODES wandVibration = VIBRATION_DEFAULT; // [1=ALWAYS,2=FIRING_ONLY,3=NEVER,4=DEFAULT]
  uint8_t barrelSwitchPolarity = 1; // [1=DEFAULT,2=INVERTED,3=DISABLED]
  SYSTEM_THEMES defaultYearModeWand = SYSTEM_TOGGLE_SWITCH; // [1=TOGGLE,2=1984,3=1989,4=AFTERLIFE,5=FROZEN_EMPIRE]
  uint8_t defaultYearModeCTS = 1; // [1=TOGGLE,2=1984,4=2021]
  uint8_t defaultWandVolume = 50; // 5-100
  uint8_t numBargraphSegments = 28; // [28=28-segment,30=30-segment]
  uint8_t bargraphIdleAnimation = 1; // [1=System,2=SuperHero,3=ModeOriginal]
  uint8_t bargraphFireAnimation = 1; // [1=System,2=SuperHero,3=ModeOriginal]
  bool isESP32 : 1;
  bool wandSoundsToPack : 1;
  bool rgbVentEnabled : 1;
  bool overheatEnabled : 1;
  bool extraProtonSounds : 1;
  bool quickVenting : 1;
  bool rgbVentColours : 1;
  bool autoVentLight : 1;
  bool wandBeepLoop : 1;
  bool wandBootError : 1;
  bool invertWandBargraph : 1;
  bool bargraphOverheatBlink : 1;
  bool audioVolumeBoosted : 1; // Whether audio is boosted by +10dB or not
  bool gpstarAudioLed : 1;
  bool isWiFiEnabled : 1; // WiFi Enabled (true) or Disabled (false)
  bool resetWifiPassword : 1;
};

// Output a compiler message if the final struct exceeds a specific size needed for SerialTransfer.
static_assert(sizeof(WandPrefs) < 35, "WARNING: WandPrefs has grown too large (>34 bytes)");

// Preferences for smoke/overheat behavior.
// All bool values are treated as bits and purposefully placed at the end of the struct to ensure proper and efficient packing.
struct __attribute__((packed)) SmokePrefs {
  uint8_t overheatDuration5 = 2; // 2-60 Seconds
  uint8_t overheatDuration4 = 2; // 2-60 Seconds
  uint8_t overheatDuration3 = 2; // 2-60 Seconds
  uint8_t overheatDuration2 = 2; // 2-60 Seconds
  uint8_t overheatDuration1 = 2; // 2-60 Seconds
  uint8_t overheatDelay5 = 2; // 2-60 Seconds
  uint8_t overheatDelay4 = 2; // 2-60 Seconds
  uint8_t overheatDelay3 = 2; // 2-60 Seconds
  uint8_t overheatDelay2 = 2; // 2-60 Seconds
  uint8_t overheatDelay1 = 2; // 2-60 Seconds
  bool smokeEnabled : 1;
  bool overheatContinuous5 : 1;
  bool overheatContinuous4 : 1;
  bool overheatContinuous3 : 1;
  bool overheatContinuous2 : 1;
  bool overheatContinuous1 : 1;
  bool overheatLevel5 : 1;
  bool overheatLevel4 : 1;
  bool overheatLevel3 : 1;
  bool overheatLevel2 : 1;
  bool overheatLevel1 : 1;
};

// Output a compiler message if the final struct exceeds a specific size needed for SerialTransfer.
static_assert(sizeof(SmokePrefs) < 35, "WARNING: SmokePrefs has grown too large (>34 bytes)");

// Data for synchronizing the Neutrona Wand.
// All bool values treated as bits and are purposefully placed at the end of the struct to ensure proper and efficient packing.
struct __attribute__((packed)) WandSyncData {
  SYSTEM_MODES systemMode = MODE_SUPER_HERO;
  SYSTEM_THEMES systemTheme = SYSTEM_AFTERLIFE;
  uint8_t streamFlags = FLAG_PROTON;
  STREAM_MODES streamMode = PROTON;
  POWER_LEVELS powerLevel = LEVEL_5;
  uint8_t packAudioVersion = 0;
  uint8_t effectsVolume = 50;
  uint8_t musicStatus = 0;
  bool ionArmSwitch : 1; // Limited to a binary state for this purpose.
  bool cyclotronLidState : 1;
  bool packOn : 1;
  bool vibrationToggle : 1; // Only tracks the pack's physical toggle state.
  bool masterMuted : 1;
  bool repeatMusicTrack : 1;
  bool shuffleMusicTracks : 1;
};

// Output a compiler message if the final struct exceeds a specific size needed for SerialTransfer.
static_assert(sizeof(WandSyncData) < 35, "WARNING: WandSyncData has grown too large (>34 bytes)");

// Data for synchronizing the Attenuator.
// All bool values are treated as bits and purposefully placed at the end of the struct to ensure proper and efficient packing.
struct __attribute__((packed)) AttenuatorSyncData {
  SYSTEM_MODES systemMode = MODE_SUPER_HERO;
  SYSTEM_THEMES systemTheme = SYSTEM_AFTERLIFE;
  uint8_t streamFlags = FLAG_PROTON;
  STREAM_MODES streamMode = PROTON;
  POWER_LEVELS powerLevel = LEVEL_5;
  uint8_t speedMultiplier = 1;
  uint8_t spectralColour = 0;
  uint8_t spectralSaturation = 0;
  uint8_t masterVolume = 50;
  uint8_t effectsVolume = 50;
  uint8_t musicVolume = 50;
  uint16_t currentTrack = 0;
  uint16_t musicCount = 0;
  uint16_t packAudioVersion = 0;
  uint16_t wandAudioVersion = 0;
  uint16_t packVoltage = 0;
  bool ionArmSwitch : 1; // Limited to a binary state for this purpose.
  bool cyclotronLidState : 1;
  bool packOn : 1;
  bool smokeOn : 1;
  bool vibrationOn : 1;
  bool cyclotronClockwise : 1;
  bool wandPresent : 1;
  bool wandMismatch : 1;
  bool barrelExtended : 1;
  bool wandFiring : 1;
  bool overheatingNow : 1;
  bool masterMuted : 1;
  bool musicPlaying : 1;
  bool musicPaused : 1;
  bool trackLooped : 1;
  bool shuffleTracks : 1;
  bool audioCorrupt : 1;
  bool audioOutdated : 1;
  bool esp32Pack : 1;
};

// Output a compiler message if the final struct exceeds a specific size needed for SerialTransfer.
static_assert(sizeof(AttenuatorSyncData) < 85, "WARNING: AttenuatorSyncData has grown too large (>84 bytes)");
