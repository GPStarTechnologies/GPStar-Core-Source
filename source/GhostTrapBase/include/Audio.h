/**
 *   GPStar Ghost Trap - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2025 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                    & Nomake Wan <nomake_wan@yahoo.co.jp>
 *                    & Dustin Grau <dustin.grau@gmail.com>
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
#include <ShuffleMusic.h>

#include <GPStarAudio.h>
gpstarAudio audio;

/*
 * Audio Devices
 */
enum AUDIO_DEVICES { A_NONE, A_GPSTAR_AUDIO, A_GPSTAR_AUDIO_ADV };
enum AUDIO_DEVICES AUDIO_DEVICE;

/*
 * Serial device
 */
#define AUDIO_RX_PIN 15 // Pin to receive serial data from the GPStar Audio
#define AUDIO_TX_PIN 16 // Pin to transmit serial data to the GPStar Audio
HardwareSerial AudioSerial(2);

/*
 * Audio Variables
 */
// Lookup tables to convert perceived loudness in 5% steps to amplifier gain. https://sengpielaudio.com/calculator-levelchange.htm
const int8_t i_volume_master_lookup_table[21] PROGMEM = { MINIMUM_VOLUME < -43 ? MINIMUM_VOLUME : -50, -43, -33, -27, -23, -20, -17, -15, -13, -12, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0 };
const int8_t i_boosted_volume_master_lookup_table[21] PROGMEM = { MINIMUM_VOLUME < -33 ? MINIMUM_VOLUME : -50, -33, -23, -17, -13, -10, -7, -5, -3, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

// Lookup tables to convert dB to %.
const uint8_t i_volume_percentage_lookup_table[44] PROGMEM = { 100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 0, 45, 40, 0, 35, 0, 30, 0, 0, 25, 0, 0, 20, 0, 0, 0, 15, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 };
const uint8_t i_pos_boosted_volume_percentage_lookup_table[11] PROGMEM = { 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100 };
const uint8_t i_neg_boosted_volume_percentage_lookup_table[34] PROGMEM = { 50, 0, 45, 40, 0, 35, 0, 30, 0, 0, 25, 0, 0, 20, 0, 0, 0, 15, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 };

uint16_t i_max_track_count = 4096; // Contains the maximum allowable tracks on a microSD card.
int16_t i_music_track_count = 0; // Contains the total number of detected music tracks on the SD card.
uint16_t i_current_music_track = 0; // Sets the ID number for the music track to be played.
uint16_t i_audio_version = 0; // Contains the firmware version for GPStar Audio (if applicable).
const uint16_t i_music_track_start = 500; // Music tracks start on file named 500_ and higher.
const int8_t i_volume_abs_min = -70; // System (absolute) minimum volume possible.
int8_t i_volume_abs_max = 0; // System (absolute) maximum volume possible.
const int8_t i_track_volume_abs_max = 0; // Maximum gain for effects/music is 0 dB (unity gain).
bool b_playing_music = false; // Sets whether a music track is currently playing or not.
bool b_music_paused = false; // Sets whether a music track is currently paused or not.
bool b_repeat_track = false; // Sets whether to repeat one music track or loop through all music tracks.
bool b_shuffle_tracks = false; // Sets whether to shuffle all music tracks or not.
bool b_preload_tracks = false; // Sets whether to add a 50ms delay before playing any file to allow slower SD cards more time to fill the buffer.
bool b_audio_boost = false; // Sets whether or not to use the +10dB boosted audio range or not.
bool b_microsd_outdated = false; // Sets whether the microSD card sound effect contents are out of date for the current firmware version.
bool b_microsd_corrupt = false; // Sets whether the microSD card appears to be corrupt.
String s_track_listing = ""; // Utilized only for the web UI to display the music track listing.

#ifndef VERSION_STRING_LEN
// Necessary for newer GPStar Audio library.
#define VERSION_STRING_LEN 21
#endif

/*
 * Music Control/Checking
 */
StatelessShuffle shuffleSystem;
const uint16_t i_music_check_delay = 2000;
const uint16_t i_music_next_track_delay = 500;
millisDelay ms_check_music;
millisDelay ms_music_next_track;
millisDelay ms_music_status_check;
uint16_t i_current_shuffle_index = 0;
uint16_t i_first_shuffle_index = 0;
uint16_t i_last_shuffle_index = 0;

/*
 * Volume percentage values (0 to 100)
 */
uint8_t i_volume_master_percentage = STARTUP_VOLUME; // Master overall volume.
uint8_t i_volume_effects_percentage = STARTUP_VOLUME_EFFECTS; // Sound effects.
uint8_t i_volume_music_percentage = STARTUP_VOLUME_MUSIC; // Music volume.

/*
 * General Volume
 * MINIMUM_VOLUME = Quietest, i_volume_abs_max = Loudest
 *
 * Note that these are set up properly in setupAudioDevice() below.
 */
int8_t i_volume_master = i_volume_abs_min; // Master overall volume.
int8_t i_volume_master_eeprom = i_volume_master; // Master overall volume that is saved into the eeprom menu and loaded during bootup.
int8_t i_volume_revert = i_volume_master; // Used to restore volume level from a muted state.
int8_t i_volume_effects = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_effects_percentage / 100); // Sound effects.
int8_t i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100); // Music volume.

/*
 * Function Prototypes
 */
void playEffect(uint16_t i_track_id, bool b_track_loop = false, int8_t i_track_volume = i_volume_effects, bool b_fade_in = false, uint16_t i_fade_time = 0, bool b_lock = true);
void stopEffect(uint16_t i_track_id);
void stopEffectLoop(uint16_t i_track_id);
void playTransitionEffect(uint16_t i_track_id, uint16_t i_track_id2, bool b_track2_loop = false, uint16_t i_track2_offset = 0, int8_t i_track_volume = i_volume_effects, bool b_fade_in = false, uint16_t i_fade_time = 0, bool b_lock = true);
void playRapidEffect(uint16_t i_track_id, uint16_t i_cycle_rate, int8_t i_track_volume = i_volume_effects);
void rapidEffectDelay(uint16_t i_track_id, uint16_t i_cycle_rate);
void adjustGainEffect(uint16_t i_track_id, int8_t i_track_volume = i_volume_effects, bool b_fade = false, uint16_t i_fade_time = 0);
void fadeoutEffect(uint16_t i_track_id, uint16_t i_fade_time = 50);
int8_t getGainValue(uint8_t percentage);
void updateMasterVolume(bool startup = false);
uint8_t getVolumePercentage(int8_t gain);
bool setMasterVolumePercentage(uint8_t percentage);
void toggleMute(bool enable);
void toggleAudioBoost(bool enable);
void toggleMusicLoop(bool enable);
void toggleMusicShuffle(bool enable);

/*
 * Audio playback functions.
 */

// Play a sound effect using certain defaults.
void playEffect(uint16_t i_track_id, bool b_track_loop, int8_t i_track_volume, bool b_fade_in, uint16_t i_fade_time, bool b_lock) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_track_volume_abs_max) {
    i_track_volume = i_track_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
      if(b_fade_in) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackPlayPoly(i_track_id, b_lock);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock);
      }

      if(b_track_loop) {
        audio.trackLoop(i_track_id, true);
      }
      else {
        audio.trackLoop(i_track_id, false);
      }
    break;

    case A_GPSTAR_AUDIO_ADV:
      if(b_fade_in) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0);
      }

      if(b_track_loop) {
        audio.trackLoop(i_track_id, true);
      }
      else {
        audio.trackLoop(i_track_id, false);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

void stopEffect(uint16_t i_track_id) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.trackStop(i_track_id);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

// Tell a looping track to stop looping and finish playing the current iteration.
void stopEffectLoop(uint16_t i_track_id) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.trackLoop(i_track_id, false);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

// Play a sound effect that plays a second sound effect once complete.
void playTransitionEffect(uint16_t i_track_id, uint16_t i_track_id2, bool b_track2_loop, uint16_t i_track2_offset, int8_t i_track_volume, bool b_fade_in, uint16_t i_fade_time, bool b_lock) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_track_volume_abs_max) {
    i_track_volume = i_track_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO_ADV:
      if(b_fade_in) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackGain(i_track_id2, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0, i_track_id2, b_track2_loop, i_track2_offset);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackGain(i_track_id2, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock, b_preload_tracks ? 50 : 0, i_track_id2, b_track2_loop, i_track2_offset);
      }
    break;

    default:
      // No valid audio device connected.
    break;
  }
}

// Play a sound effect rapid-fire using two channels for polyphony.
void playRapidEffect(uint16_t i_track_id, uint16_t i_cycle_rate, int8_t i_track_volume) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_track_volume_abs_max) {
    i_track_volume = i_track_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO_ADV:
      if(i_audio_version >= 109) {
        // This feature is only supported by GPStar Audio v1.09 or later.
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackRapidPlay(i_track_id, i_cycle_rate);
      }
    break;

    default:
      // No valid audio device connected.
    break;
  }
}

// Update the delay that the current rapid-fire track is using.
void rapidEffectDelay(uint16_t i_track_id, uint16_t i_cycle_rate) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO_ADV:
      if(i_audio_version >= 109) {
        // This feature is only supported by GPStar Audio v1.09 or later.
        audio.trackRapidDelay(i_track_id, i_cycle_rate);
      }
    break;

    default:
      // No valid audio device connected.
    break;
  }
}

// Play a music track using certain defaults.
void playMusic() {
  if(i_music_track_count > 0 && i_current_music_track >= i_music_track_start) {
    b_playing_music = true;

    switch(AUDIO_DEVICE) {
      case A_GPSTAR_AUDIO:
        // Loop the music track.
        if(b_repeat_track) {
          audio.trackLoop(i_current_music_track, true);
        }
        else {
          audio.trackLoop(i_current_music_track, false);
        }

        audio.trackGain(i_current_music_track, i_volume_music);
        audio.trackPlayPoly(i_current_music_track, true);
        audio.update();

        audio.resetTrackCounter();
      break;

      case A_GPSTAR_AUDIO_ADV:
        // Loop the music track.
        if(b_repeat_track) {
          audio.trackLoop(i_current_music_track, true);
        }
        else {
          audio.trackLoop(i_current_music_track, false);
        }

        audio.trackGain(i_current_music_track, i_volume_music);
        audio.trackPlayPoly(i_current_music_track, true, b_preload_tracks ? 50 : 0);
        audio.update();

        audio.resetTrackCounter();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    // Manage track navigation.
    ms_music_status_check.start(i_music_check_delay * 10);
  }
}

void stopMusic() {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      if(i_music_track_count > 0 && i_current_music_track >= i_music_track_start) {
        audio.trackStop(i_current_music_track);
      }

      audio.update();
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }

  b_music_paused = false;
  b_playing_music = false;
}

void pauseMusic() {
  if(b_playing_music && !b_music_paused) {
    // Stop the music check timer.
    ms_music_status_check.stop();

    // Pause music playback on the Proton Pack
    switch(AUDIO_DEVICE) {
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        audio.trackPause(i_current_music_track);
        audio.update();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    b_music_paused = true;
  }
}

void resumeMusic() {
  if(b_music_paused) {
    // Reset the music check timer.
    ms_music_status_check.start(i_music_check_delay * 4);

    // Resume music playback on the Proton Pack
    switch(AUDIO_DEVICE) {
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        audio.resetTrackCounter();
        audio.trackResume(i_current_music_track);
        audio.update();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    b_music_paused = false;
  }
}

void musicNextTrack() {
  int16_t i_temp_track = i_current_music_track; // Used for music navigation.

  if(b_shuffle_tracks) {
    // Reset the shuffle index just in case the user manually selected a track before this.
    i_current_shuffle_index = shuffleSystem.ShuffledIndexToIndex(i_temp_track - i_music_track_start);

    if(i_current_shuffle_index == i_last_shuffle_index) {
      // If we reached the end of the shuffle, restart the shuffle.
      i_current_shuffle_index = 0;
    }
    else {
      // Advance the shuffle index.
      i_current_shuffle_index++;
    }

    // Get the shuffled track number.
    i_temp_track = shuffleSystem.IndexToShuffledIndex(i_current_shuffle_index);

    while(i_temp_track >= i_music_track_count) {
      // Shuffle could include invalid values, so skip forward until we have a valid value.
      i_current_shuffle_index++;
      i_temp_track = shuffleSystem.IndexToShuffledIndex(i_current_shuffle_index);
    }

    // Finally, convert to a proper track number.
    i_temp_track = i_temp_track + i_music_track_start;
  }
  else {
    // Determine the next track.
    if(i_current_music_track + 1 > i_music_track_start + i_music_track_count - 1) {
      // Start at the first track if already on the last.
      i_temp_track = i_music_track_start;
    }
    else {
      i_temp_track++;
    }
  }

  // Switch to the next track.
  if(b_playing_music) {
    // Stops music using the current track number as the identifier.
    stopMusic();

    i_current_music_track = i_temp_track; // Change only AFTER stopping music playback.

    // Play the appropriate track on pack and wand, and notify the AudioSerial device.
    playMusic();
  }
  else {
    // Set the new track.
    i_current_music_track = i_temp_track;
  }
}

void musicPrevTrack() {
  int16_t i_temp_track = i_current_music_track; // Used for music navigation.

  if(b_shuffle_tracks) {
    // Reset the shuffle index just in case the user manually selected a track before this.
    i_current_shuffle_index = shuffleSystem.ShuffledIndexToIndex(i_temp_track - i_music_track_start);

    if(i_current_shuffle_index == i_first_shuffle_index) {
      // If we reached the end of the shuffle, restart the shuffle.
      i_current_shuffle_index = i_last_shuffle_index;
    }
    else {
      // Rewind the shuffle index.
      i_current_shuffle_index--;
    }

    // Get the shuffled track number.
    i_temp_track = shuffleSystem.IndexToShuffledIndex(i_current_shuffle_index);

    while(i_temp_track >= i_music_track_count) {
      // Shuffle could include invalid values, so skip backward until we have a valid value.
      i_current_shuffle_index--;
      i_temp_track = shuffleSystem.IndexToShuffledIndex(i_current_shuffle_index);
    }

    // Finally, convert to a proper track number.
    i_temp_track = i_temp_track + i_music_track_start;
  }
  else {
    // Determine the previous track.
    if(i_current_music_track - 1 < i_music_track_start) {
      // Start at the last track if already on the first.
      i_temp_track = i_music_track_start + (i_music_track_count - 1);
    }
    else {
      i_temp_track--;
    }
  }

  // Switch to the previous track.
  if(b_playing_music) {
    // Stops music using the current track number as the identifier.
    stopMusic();

    i_current_music_track = i_temp_track; // Change only AFTER stopping music playback.

    // Play the appropriate track on pack and wand, and notify the AudioSerial device.
    playMusic();
  }
  else {
    // Set the new track.
    i_current_music_track = i_temp_track;
  }
}

// Adjust the gain of a single track.
void adjustGainEffect(uint16_t i_track_id, int8_t i_track_volume, bool b_fade, uint16_t i_fade_time) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_track_volume_abs_max) {
    i_track_volume = i_track_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      if(b_fade) {
        audio.trackFade(i_track_id, i_track_volume, i_fade_time);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

// Fades out a single track.
void fadeoutEffect(uint16_t i_track_id, uint16_t i_fade_time) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.trackFade(i_track_id, i_volume_abs_min, i_fade_time, true);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

// Returns the correct audio gain value for a given percentage.
int8_t getGainValue(uint8_t percentage) {
  // Range validation: clamp to 0-100
  if(percentage > 100) {
    percentage = 100;
  }

  // Round to nearest VOLUME_MULTIPLIER step (5% increments)
  if(percentage % VOLUME_MULTIPLIER != 0) {
    uint8_t step_size = VOLUME_MULTIPLIER;
    percentage = ((percentage + step_size / 2) / step_size) * step_size;
  }

  // Ensure we don't exceed 100% after stepping
  if(percentage > 100) {
    percentage = 100;
  }

  // Determine if our gain range is boosted.
  if(b_audio_boost) {
    // Return the boosted range value.
    return PROGMEM_READI8(i_boosted_volume_master_lookup_table[percentage / 5]);
  }
  else {
    // Return the non-boosted range value.
    return PROGMEM_READI8(i_volume_master_lookup_table[percentage / 5]);
  }
}

void updateMasterVolume(bool startup) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.masterGain(i_volume_master);
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }

  if(!startup) {
    // If this isn't being called at boot, provide audio feedback and report the change.
    if((TRAP_STATE == TRAP_IDLE || TRAP_STATE == TRAP_SERVICE || TRAP_STATE == TRAP_NO_CARTRIDGE) && !(b_playing_music && !b_music_paused)) {
      if(i_volume_master_percentage == 50) {
        // Provide a distinct sound when set to 50%.
        stopEffect(S_BEEPS);
        playEffect(S_BEEPS, false, 0, false, 0, false);
      }
      else {
        // Provide feedback when the Proton Pack is not running.
        stopEffect(S_BEEPS_ALT);
        playEffect(S_BEEPS_ALT, false, 0, false, 0, false);
      }
    }
  }
}

void increaseVolumeEEPROM() {
  if(i_volume_master == i_volume_abs_max) {
    // Cannot go any higher.
  }
  else {
    if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
      i_volume_master_percentage = 100;
    }
    else {
      i_volume_master_percentage += VOLUME_MULTIPLIER;
    }

    //trapConfig.defaultSystemVolume = i_volume_master_percentage;
    i_volume_master = getGainValue(i_volume_master_percentage);
    i_volume_revert = i_volume_master;

    updateMasterVolume();
  }
}

void decreaseVolumeEEPROM() {
  if(i_volume_master == MINIMUM_VOLUME) {
    // Cannot go any lower.
  }
  else {
    if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
      i_volume_master_percentage = 0;
    }
    else {
      i_volume_master_percentage -= VOLUME_MULTIPLIER;
    }

    //trapConfig.defaultSystemVolume = i_volume_master_percentage;
    i_volume_master = getGainValue(i_volume_master_percentage);
    i_volume_revert = i_volume_master;

    updateMasterVolume();
  }
}

void increaseVolume() {
  if(i_volume_master == i_volume_abs_max) {
    // Cannot go any higher.
  }
  else {
    if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
      i_volume_master_percentage = 100;
    }
    else {
      i_volume_master_percentage += VOLUME_MULTIPLIER;
    }

    i_volume_master = getGainValue(i_volume_master_percentage);
    i_volume_revert = i_volume_master;

    updateMasterVolume();
  }
}

void decreaseVolume() {
  if(i_volume_master == MINIMUM_VOLUME) {
    // Cannot go any lower.
  }
  else {
    if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
      i_volume_master_percentage = 0;
    }
    else {
      i_volume_master_percentage -= VOLUME_MULTIPLIER;
    }

    i_volume_master = getGainValue(i_volume_master_percentage);
    i_volume_revert = i_volume_master;

    updateMasterVolume();
  }
}

// Return the percentage value for a particular dB gain
uint8_t getVolumePercentage(int8_t gain) {
  if(b_audio_boost) {
    // We are using the boosted curve so we need special handling.
    if(gain > 0) {
      return PROGMEM_READU8(i_pos_boosted_volume_percentage_lookup_table[gain]);
    }
    else if(gain < -33) {
      // Minimum is 0%.
      return 0;
    }
    else {
      return PROGMEM_READU8(i_neg_boosted_volume_percentage_lookup_table[abs(gain)]);
    }
  }
  else {
    // Use the standard curve.
    if(gain < -43) {
      // Minimum is 0%.
      return 0;
    }
    else if(gain > 0) {
      // Maximum is 100%.
      return 100;
    }
    else {
      return PROGMEM_READU8(i_volume_percentage_lookup_table[abs(gain)]);
    }
  }
}

// Set master volume to a specific percentage with range checking and stepping
bool setMasterVolumePercentage(uint8_t percentage) {
  // Range validation: clamp to 0-100
  if(percentage > 100) {
    percentage = 100;
  }

  // Round to nearest VOLUME_MULTIPLIER step (5% increments)
  if(percentage % VOLUME_MULTIPLIER != 0) {
    uint8_t step_size = VOLUME_MULTIPLIER;
    percentage = ((percentage + step_size / 2) / step_size) * step_size;
  }

  // Ensure we don't exceed 100% after stepping
  if(percentage > 100) {
    percentage = 100;
  }

  // Update percentage value
  i_volume_master_percentage = percentage;

  // Convert to decibel value using lookup table
  i_volume_master = getGainValue(i_volume_master_percentage);

  // Check against system min/max bounds
  if(i_volume_master > i_volume_abs_max) {
    i_volume_master = i_volume_abs_max;
    i_volume_master_percentage = 100;
  }
  else if(i_volume_master < MINIMUM_VOLUME) {
    i_volume_master = MINIMUM_VOLUME;
    i_volume_master_percentage = 0;
  }

  // Update revert volume for mute functionality
  i_volume_revert = i_volume_master;

  // Apply the change to audio system
  updateMasterVolume();

  return true;
}

void toggleMute(bool enable) {
  if(enable) {
    i_volume_revert = i_volume_master;

    // Set the master volume to minimum.
    i_volume_master = i_volume_abs_min;

    updateMasterVolume();
  }
  else {
    i_volume_master = i_volume_revert;

    updateMasterVolume();
  }
}

// Toggles doubling the volume output on or off.
void toggleAudioBoost(bool enable) {
  // If enabled, max gain is +10dB, otherwise unity gain.
  i_volume_abs_max = enable ? 10 : 0;

  // Finally, reset out current volume to the new paradigm.
  i_volume_master = getGainValue(i_volume_master_percentage);
  i_volume_revert = i_volume_master;
  updateMasterVolume(true); // set to true to stop sound playback
}

void updateEffectsVolume() {
  // Currently non-op as we do not have sound effects yet.
}

void increaseVolumeEffects() {
  if(i_volume_effects_percentage + VOLUME_EFFECTS_MULTIPLIER > 100) {
    i_volume_effects_percentage = 100;

    // Provide feedback at maximum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, 0, false, 0, false);
  }
  else {
    i_volume_effects_percentage += VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume_effects = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_effects_percentage / 100);
}

void decreaseVolumeEffects() {
  if(i_volume_effects_percentage - VOLUME_EFFECTS_MULTIPLIER < 0) {
    i_volume_effects_percentage = 0;;

    // Provide feedback at minimum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, 0, false, 0, false);
  }
  else {
    i_volume_effects_percentage -= VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume_effects = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_effects_percentage / 100);
}

void updateMusicVolume() {
  if(i_music_track_count > 0) {
    switch(AUDIO_DEVICE) {
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        audio.trackGain(i_current_music_track, i_volume_music);
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }
  }
}

void increaseVolumeMusic() {
  if(i_volume_music_percentage + VOLUME_MUSIC_MULTIPLIER > 100) {
    i_volume_music_percentage = 100;

    // Provide feedback at maximum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, 0, false, 0, false);
  }
  else {
    i_volume_music_percentage += VOLUME_MUSIC_MULTIPLIER;
  }

  i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

  updateMusicVolume();
}

void decreaseVolumeMusic() {
  if(i_volume_music_percentage - VOLUME_MUSIC_MULTIPLIER < 0) {
    i_volume_music_percentage = 0;

    // Provide feedback at minimum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, 0, false, 0, false);
  }
  else {
    i_volume_music_percentage -= VOLUME_MUSIC_MULTIPLIER;
  }

  i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

  updateMusicVolume();
}

void buildMusicCount(uint16_t i_num_tracks) {
  // Build the music track count.
  i_music_track_count = i_num_tracks - i_last_effects_track;
  int16_t i_max_music_tracks = i_max_track_count - i_last_effects_track;

  if(i_music_track_count == 0) {
    // Do nothing, we have no music.
  }
  else if(i_music_track_count < 0) {
    // If we have a negative music track count, this means the user needs to update their microSD card.
    i_music_track_count = 0; // If the music count is negative, make it 0
    b_microsd_outdated = true; // Set the flag indicating our microSD card contents are outdated.
    sendDebug(F("Warning: Track count does not match firmware! Please update the microSD card sound effects."));
  }
  else if(i_music_track_count < i_max_music_tracks && i_num_tracks <= i_max_track_count) {
    i_current_music_track = i_music_track_start; // Set the first track of music as file 500_

    // Build the shuffled music track system.
    uint32_t currentTime = millis() + random(256);
    uint32_t seed = MurmurHash2A(&currentTime, sizeof(currentTime), 0x1337beef);
    shuffleSystem.SetItemCount(i_music_track_count);
    shuffleSystem.SetSeed(seed);
    i_first_shuffle_index = 0;
    i_last_shuffle_index = 0;

    // Need to identify the index of the first and last items in the shuffle so we know when to loop back around.
    bool firstLoop = true;
    for(int16_t i = 0; i < i_music_track_count; i++) {
      int16_t i_last_shuffled_track = shuffleSystem.IndexToShuffledIndex(i_last_shuffle_index);
      while(i_last_shuffled_track >= i_music_track_count) {
        i_last_shuffle_index++;
        i_last_shuffled_track = shuffleSystem.IndexToShuffledIndex(i_last_shuffle_index);
      }
      if(firstLoop) {
        i_first_shuffle_index = i_last_shuffle_index;
        firstLoop = false;
      }
      if(i != i_music_track_count - 1) {
        // Only advance if we aren't on the final iteration.
        i_last_shuffle_index++;
      }
    }

    // Set the shuffle to start at the first track by default.
    i_current_shuffle_index = shuffleSystem.ShuffledIndexToIndex(i_current_music_track - i_last_effects_track);
  }
  else {
    // If we somehow underflowed, treat the microSD card as corrupt.
    i_music_track_count = 0; // If the music count is corrupt, make it 0
    b_microsd_corrupt = true; // Set the flag indicating our microSD card is corrupt.
    sendDebug(F("Warning: Calculated music count exceeds maximum possible; SD card corruption likely!"));
  }
}

bool musicIsTrackCounterReset() {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      return audio.isTrackCounterReset();
    break;

    case A_NONE:
    default:
      return false;
    break;
  }
}

void musicTrackPlayingStatus() {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.trackPlayingStatus(i_current_music_track);
    break;

    case A_NONE:
    default:
      // Do nothing.
    break;
  }
}

bool musicTrackStatus() {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      return audio.currentTrackStatus(i_current_music_track);
    break;

    case A_NONE:
    default:
      return false;
    break;
  }
}

void checkMusic() {
  if(ms_check_music.justFinished() && !ms_music_next_track.isRunning()) {
    switch(AUDIO_DEVICE) {
      case A_GPSTAR_AUDIO:
      case A_GPSTAR_AUDIO_ADV:
        ms_check_music.start(i_music_check_delay);

        musicTrackPlayingStatus();

        // Loop through all the tracks if the music is not set to repeat a track.
        if(b_playing_music && !b_repeat_track && !b_music_paused) {
          if(!musicTrackStatus() && ms_music_status_check.justFinished() && !musicIsTrackCounterReset()) {
            ms_check_music.stop();
            ms_music_status_check.stop();

            stopMusic();

            // Switch to the next track.
            if(i_current_music_track + 1 > i_music_track_start + i_music_track_count - 1) {
              i_current_music_track = i_music_track_start;
            }
            else {
              i_current_music_track++;
            }

            // Start timer to prepare to play music again.
            ms_music_next_track.start(i_music_next_track_delay);
          }
          else {
            if(ms_music_status_check.justFinished()) {
              ms_music_status_check.start(i_music_check_delay * 4);
            }
          }
        }
      break;

      case A_NONE:
      default:
        // None
      break;
    }
  }

  // Start playing music again.
  if(ms_music_next_track.justFinished()) {
    ms_music_next_track.stop();
    ms_check_music.start(i_music_check_delay);

    // Play the appropriate track on the pack and wand, and notify the AudioSerial device.
    playMusic();
  }
}

void toggleMusicLoop(bool enable) {
  b_repeat_track = enable;

  if(i_music_track_count > 0) {
    // Loop the current music track.
    audio.trackLoop(i_current_music_track, b_repeat_track);
  }
}

void toggleMusicShuffle(bool enable) {
  b_shuffle_tracks = enable;
}

void setAudioLED(bool on) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      // Set GPStar Audio LED state immediately.
      audio.gpstarLEDStatus(on);
    break;

    default:
      // Do nothing if not GPStar Audio.
    break;
  }
}

/*
 * Enabled by default, GPStar Audio will detect multiple versions of the same sound playing
 * in succession and prevent it from overloading and taking too many audio channels, instead
 * replaying the file to save system resources.
 */
void useShortTrackOverload(bool enabled) {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      // Enable or disable short track overload.
      audio.gpstarShortTrackOverload(enabled);
    break;

    default:
      // Do nothing if not GPStar Audio.
    break;
  }
}

/*
 * Audio Setup Routines
 * Used to detect, update, and reset the available audio devices.
 */
bool setupAudioDevice() {
  // Short delay to allow the audio boards to boot up.
  delay(1000);

  AudioSerial.begin(57600, SERIAL_8N1, AUDIO_RX_PIN, AUDIO_TX_PIN);

  audio.start(AudioSerial);

  uint16_t i_timeout = millis() + 1000;
  uint16_t i_num_tracks = 0;

  while(!audio.gpstarAudioHello() && millis() < i_timeout) {
    audio.hello();
    delay(10);
  }

  if(audio.gpstarAudioHello()) {
    i_audio_version = audio.getVersionNumber();

    if(i_audio_version != 0) {
      AUDIO_DEVICE = A_GPSTAR_AUDIO_ADV;

      if(i_audio_version >= 109) {
        // Version 1.09 does not require short track overload.
        useShortTrackOverload(false);
      }
    }
    else {
      AUDIO_DEVICE = A_GPSTAR_AUDIO;
      i_audio_version = 100; // Set to 100 to indicate old version.
    }

    i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100); // Master overall volume.
    i_volume_master_eeprom = i_volume_master; // Master overall volume that is saved into the eeprom menu and loaded during bootup.
    i_volume_revert = i_volume_master; // Used to restore volume level from a muted state.

    sendDebug(String(F("Using GPStar Audio Version: ")) + String(audio.getVersionNumber()));

    i_num_tracks = audio.getNumTracks();
    buildMusicCount(i_num_tracks);
    setAudioLED(b_gpstar_audio_led_enabled);

    // Reset the master gain db. Range is -70 to 0. Bootup the system muted, then we reset it after the system is loaded.
    audio.masterGain(i_volume_abs_min);

    // Stop all tracks.
    audio.stopAllTracks();

    if(b_microsd_corrupt || b_microsd_outdated) {
      // If we ran into an error, attempt to play an alarm sound and exit.
      if(i_num_tracks >= S_BEEP_8) {
        playEffect(S_BEEP_8, false, 0, false, 0, false);
      }

      return false;
    }

    return true;
  }
  else {
    // No audio devices connected.
    AUDIO_DEVICE = A_NONE;
    AudioSerial.end();

    sendDebug(F("No Audio Device"));

    return false;
  }
}

void updateAudio() {
  switch(AUDIO_DEVICE) {
    case A_GPSTAR_AUDIO:
    case A_GPSTAR_AUDIO_ADV:
      audio.update();
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }
}
