/**
 *   GPStar Neutrona Wand - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023-2026 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
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

// Forward function declaration.
void packSerialSend(uint16_t i_command, uint16_t i_value); // From Serial.h
void notifyWSClients(); // From Webhandler.h

/**
 * Centralized handler for commands, allowing the Pack and Wand to both perform the same action.
 * This approach is applying the Command Pattern to decouple the sender from the receiver.
 * In order for this to work, the command value must come from a unique source: PACK_MESSAGE
 *
 * Inputs:
 *   - i_command: Command identifier (PACK_MESSAGE enum)
 *   - i_value: Optional value for the command (default 0)
 */
void executeCommand(uint16_t i_command, uint16_t i_value = 0) {
  switch(i_command) {
    case A_PACK_ON:
      // Pack is on.
      b_pack_on = true;
      b_pack_shutting_down = false;
    break;

    case A_PACK_OFF:
      // Pack is off.
      if(b_pack_on) {
        // Turn wand off.
        if(WAND_STATUS != MODE_OFF) {
          if(WAND_STATUS == MODE_ERROR) {
            wandOff();
            b_wand_mash_lockout = false;
          }
          else {
            b_wand_mash_lockout = false;
            WAND_ACTION_STATUS = ACTION_OFF;
          }
        }
      }

      // Pack is off.
      b_pack_on = false;
      b_pack_shutting_down = (i_value == 1);
    break;

    case A_SOUND_SUPER_HERO:
      stopEffect(S_VOICE_MODE_SUPER_HERO);
      stopEffect(S_VOICE_MODE_ORIGINAL);
      playEffect(S_VOICE_MODE_SUPER_HERO);
    break;

    case A_SOUND_MODE_ORIGINAL:
      stopEffect(S_VOICE_MODE_SUPER_HERO);
      stopEffect(S_VOICE_MODE_ORIGINAL);
      playEffect(S_VOICE_MODE_ORIGINAL);
    break;

    case A_MODE_SUPER_HERO:
      gpstarWand.setSystemMode(MODE_SUPER_HERO);
      gpstarWand.setPowerLevel(LEVEL_5); // Restore PL5 as the default power level.
      updatePackPowerLevel();
      vgModeCheck(); // Re-check VG/CTS mode.
      packSerialSend(A_STREAM_FLAGS, gpstarWand.getStreamModeOpts()); // Send the latest flags upstream.
    break;

    case A_MODE_ORIGINAL:
      gpstarWand.setSystemMode(MODE_ORIGINAL);
      if(gpstarWand.getPowerLevel() != MIN_POWER_LEVEL) {
        // If not already in PL1, set to PL1 as this is idle in Mode Original.
        gpstarWand.setPowerLevel(LEVEL_1);
        updatePackPowerLevel();
      }
      vgModeCheck(); // Assert CTS mode.
      packSerialSend(A_STREAM_FLAGS, gpstarWand.getStreamModeOpts()); // Send the latest flags upstream.
    break;

    case A_OVERHEATING_FINISHED:
      if(WAND_STATUS != MODE_OFF) {
        overheatingFinished();
      }
    break;

    case A_VENTING_FINISHED:
      if(WAND_STATUS != MODE_OFF) {
        quickVentFinished();
      }
    break;

    case A_INNER_CYCLOTRON_PANEL_DISABLED:
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_STATIC_COLORS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DYNAMIC_COLORS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DISABLED);
      playEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DISABLED);
    break;

    case A_INNER_CYCLOTRON_PANEL_STATIC:
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_STATIC_COLORS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DYNAMIC_COLORS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DISABLED);
      playEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_STATIC_COLORS);
    break;

    case A_INNER_CYCLOTRON_PANEL_DYNAMIC:
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_STATIC_COLORS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DYNAMIC_COLORS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DISABLED);
      playEffect(S_VOICE_INNER_CYCLOTRON_LED_PANEL_DYNAMIC_COLORS);
    break;

    case A_ION_ARM_SWITCH_ON:
      changeIonArmSwitchState(true);
    break;

    case A_ION_ARM_SWITCH_OFF:
      changeIonArmSwitchState(false);
    break;

    case A_CYCLOTRON_LID_ON:
      b_pack_cyclotron_lid_on = true;
    break;

    case A_CYCLOTRON_LID_OFF:
      b_pack_cyclotron_lid_on = false;
    break;

    case A_MANUAL_OVERHEAT:
      if(WAND_STATUS == MODE_ON && WAND_ACTION_STATUS != ACTION_SETTINGS && WAND_ACTION_STATUS != ACTION_OVERHEATING && WAND_ACTION_STATUS != ACTION_VENTING) {
        if(b_pack_on && !b_pack_alarm && b_overheat_enabled) {
          if(b_extra_pack_sounds) {
            packSerialSend(A_EXTRA_WAND_SOUNDS_STOP);
          }

          switch(getNeutronaWandYearMode()) {
            case SYSTEM_1984:
            case SYSTEM_1989:
            break;

            case SYSTEM_AFTERLIFE:
            case SYSTEM_FROZEN_EMPIRE:
            default:
              if(!b_sound_idle) {
                stopAfterlifeSounds();
                playEffect(S_AFTERLIFE_WAND_RAMP_DOWN_1);

                if(b_extra_pack_sounds) {
                  packSerialSend(A_AFTERLIFE_GUN_RAMP_DOWN_1);
                }
              }
            break;
          }

          startVentSequence();
        }
      }
      else if(WAND_STATUS == MODE_OFF) {
        packSerialSend(A_OVERHEATING);
      }
    break;

    case A_MANUAL_QUICK_VENT:
      if(WAND_STATUS == MODE_ON && WAND_ACTION_STATUS != ACTION_SETTINGS && WAND_ACTION_STATUS != ACTION_OVERHEATING && WAND_ACTION_STATUS != ACTION_VENTING) {
        if(b_pack_on && !b_pack_alarm && b_overheat_enabled) {
          startQuickVent();
        }
      }
      else if(WAND_STATUS == MODE_OFF) {
        packSerialSend(A_VENTING);
      }
    break;

    case A_MUSIC_STATUS:
      // Received music status update, so set playing music variables accordingly.
      switch(i_value) {
        case 1:
        default:
          // Music stopped.
          b_playing_music = false;
          b_music_paused = false;
        break;
        case 2:
          // Music started.
          b_playing_music = true;
          b_music_paused = false;
        break;
        case 3:
          // Music resumed.
          b_playing_music = true;
          b_music_paused = false;
        break;
        case 4:
          // Music paused.
          b_playing_music = true;
          b_music_paused = true;
        break;
      }

      // If we are fully off we must also make sure to start/stop the power reminder.
      if(b_playing_music && !b_music_paused) {
        setPowerOnReminder(false);
      }
      else if(WAND_STATUS == MODE_OFF && WAND_ACTION_STATUS == ACTION_IDLE && ((!b_pack_on && gpstarWand.getSystemMode() == MODE_SUPER_HERO) || gpstarWand.isPackInactiveModeOriginal())) {
        setPowerOnReminder(true);
      }
    break;

    case A_MUSIC_TRACK_LOOP_STATUS:
      // The pack is telling us if the current music track is looped or not.
      toggleMusicLoop(i_value == 2);
    break;

    case A_MUSIC_TRACK_SHUFFLE_STATUS:
      // The pack is telling us if "shuffle all music tracks" is enabled or not.
      toggleMusicShuffle(i_value == 2);
    break;

    case A_MASTER_AUDIO_STATUS:
      // The pack is telling us whether the master mute is enabled or not.
      toggleMute(i_value == 2);
    break;

    case A_ALARM_ON:
      // Set ribbon cable status.
      b_ribbon_cable_attached = i_value == 1;

      // Alarm is on.
      b_pack_alarm = true;

      if(WAND_STATUS != MODE_ERROR && WAND_ACTION_STATUS != ACTION_OVERHEATING) {
        if(WAND_STATUS == MODE_ON) {
          if(b_extra_pack_sounds) {
            packSerialSend(A_WAND_SHUTDOWN_SOUND);
            packSerialSend(A_EXTRA_WAND_SOUNDS_STOP);
          }

          stopEffect(S_WAND_SHUTDOWN);
          playEffect(S_WAND_SHUTDOWN);

          switch(getNeutronaWandYearMode()) {
            case SYSTEM_1984:
            case SYSTEM_1989:
              // Do nothing.
            break;

            case SYSTEM_AFTERLIFE:
            case SYSTEM_FROZEN_EMPIRE:
            default:
              if(!b_sound_idle) {
                stopAfterlifeSounds();
                playEffect(S_AFTERLIFE_WAND_RAMP_DOWN_1);

                if(b_extra_pack_sounds) {
                  packSerialSend(A_AFTERLIFE_GUN_RAMP_DOWN_1);
                }
              }
            break;
          }

          // Prepare to ramp the bargraph down.
          prepBargraphRampDown();

          if(WAND_ACTION_STATUS == ACTION_SETTINGS) {
            // If the wand is in settings mode while the alarm is activated, exit the settings mode.
            packSerialSend(A_SET_STREAM_MODE, gpstarWand.getStreamModeByte());
            WAND_ACTION_STATUS = ACTION_IDLE;
          }

          // Update sounds as necessary.
          soundBeepLoopStop();
          soundIdleStop();
          soundIdleLoopStop(true);
        }

        ms_error_blink.start(i_error_blink_delay); // Start the error blink timer.
      }
    break;

    case A_ALARM_OFF:
      // Set ribbon cable status.
      b_ribbon_cable_attached = i_value == 1;

      if(WAND_STATUS != MODE_ERROR && b_pack_alarm) {
        resetHatLights(); // Reset the hat light states.

        if(WAND_STATUS == MODE_ON) {
          switch(gpstarWand.getSystemMode()) {
            case MODE_ORIGINAL:
              if(switch_vent.on() && switch_wand.on() && switch_activate.on()) {
                prepBargraphRampUp();
              }
            break;

            case MODE_SUPER_HERO:
            default:
              prepBargraphRampUp();
            break;
          }
        }

        if(WAND_STATUS == MODE_ON && WAND_ACTION_STATUS != ACTION_OVERHEATING && b_pack_on) {
          soundIdleLoop(true);

          if(getNeutronaWandYearMode() == SYSTEM_AFTERLIFE || getNeutronaWandYearMode() == SYSTEM_FROZEN_EMPIRE) {
            stopEffect(S_WAND_BOOTUP);
            playEffect(S_WAND_BOOTUP);

            if(!switch_vent.on()) {
              afterlifeRampSound1();
            }
          }
        }
      }

      // Alarm is off.
      b_pack_alarm = false;
    break;

    case A_WARNING_CANCELLED:
      // Pack is telling wand to cancel any overheat warnings.
      // First, stop the timer which triggers the overheat.
      ms_overheat_initiate.stop();

      // Then reset the hat light states.
      resetHatLights();

      // Next, reset the cyclotron speed on all devices.
      packSerialSend(A_CYCLOTRON_NORMAL_SPEED);
      cyclotronSpeedRevert();
    break;

    case A_VOLUME_SOUND_EFFECTS_INCREASE:
      // Increase effects volume.
      increaseVolumeEffects();
    break;

    case A_VOLUME_SOUND_EFFECTS_DECREASE:
      // Decrease effects volume.
      decreaseVolumeEffects();
    break;

    case A_SET_WAND_VIBRATION_MODE:
      // Wand vibration mode from Pack vibration toggle switch.
      // i_value: 1=ALWAYS, 2=FIRING_ONLY, 3=NEVER, 4=DEFAULT
      if(i_value == 1 || i_value == 2 || i_value == 4) {
        // ALWAYS, FIRING_ONLY, or DEFAULT (treat as enabled)
        b_vibration_switch_on = true;

        if(WAND_ACTION_STATUS != ACTION_CONFIG_EEPROM_MENU) {
          stopEffect(S_BEEPS_ALT);
          playEffect(S_BEEPS_ALT);

          stopEffect(S_VOICE_VIBRATION_ENABLED);
          stopEffect(S_VOICE_VIBRATION_DISABLED);
          playEffect(S_VOICE_VIBRATION_ENABLED);
        }
      }
      else if(i_value == 3) {
        // NEVER (disabled)
        b_vibration_switch_on = false;

        stopEffect(S_BEEPS_ALT);
        playEffect(S_BEEPS_ALT);

        stopEffect(S_VOICE_VIBRATION_DISABLED);
        stopEffect(S_VOICE_VIBRATION_ENABLED);
        playEffect(S_VOICE_VIBRATION_DISABLED);

        vibrationOff();
      }
    break;

    case A_SET_PACK_VIBRATION_MODE:
      // Pack vibration mode setting.
      // i_value: 1=ALWAYS, 2=FIRING_ONLY, 3=NEVER, 4=DEFAULT, 5=CYCLOTRON_MOTOR
      stopEffect(S_BEEPS_ALT);
      playEffect(S_BEEPS_ALT);

      stopEffect(S_VOICE_PROTON_PACK_VIBRATION_FIRING_ENABLED);
      stopEffect(S_VOICE_PROTON_PACK_VIBRATION_ENABLED);
      stopEffect(S_VOICE_PROTON_PACK_VIBRATION_DISABLED);
      stopEffect(S_VOICE_PROTON_PACK_VIBRATION_DEFAULT);
      stopEffect(S_VOICE_MOTORIZED_CYCLOTRON_ENABLED);

      switch(i_value) {
        case 1:
          // ALWAYS
          playEffect(S_VOICE_PROTON_PACK_VIBRATION_ENABLED);
        break;
        case 2:
          // FIRING_ONLY
          playEffect(S_VOICE_PROTON_PACK_VIBRATION_FIRING_ENABLED);
        break;
        case 3:
          // NEVER
          playEffect(S_VOICE_PROTON_PACK_VIBRATION_DISABLED);
        break;
        case 4:
          // DEFAULT
          playEffect(S_VOICE_PROTON_PACK_VIBRATION_DEFAULT);
        break;
        case 5:
          // CYCLOTRON_MOTOR
          playEffect(S_VOICE_MOTORIZED_CYCLOTRON_ENABLED);
        break;
      }
    break;

    case A_YEAR_1984:
      // Indicates system (pack) year is 1984 mode
      gpstarWand.setSystemTheme(SYSTEM_1984);
      bargraphYearModeUpdate();
      resetWhiteLEDBlinkRate();
    break;

    case A_YEAR_1989:
      // Indicates system (pack) year is 1984 mode
      gpstarWand.setSystemTheme(SYSTEM_1989);
      bargraphYearModeUpdate();
      resetWhiteLEDBlinkRate();
    break;

    case A_YEAR_AFTERLIFE:
      // Indicates system (pack) year is Afterlife mode
      gpstarWand.setSystemTheme(SYSTEM_AFTERLIFE);
      bargraphYearModeUpdate();
      resetWhiteLEDBlinkRate();
    break;

    case A_YEAR_FROZEN_EMPIRE:
      // Indicates system (pack) year is Frozen Empire mode
      gpstarWand.setSystemTheme(SYSTEM_FROZEN_EMPIRE);
      bargraphYearModeUpdate();
      resetWhiteLEDBlinkRate();
    break;

    case A_MODE_FROZEN_EMPIRE:
      // Play only the Frozen Empire voice
      stopEffect(S_BEEPS_BARGRAPH);
      stopEffect(S_VOICE_FROZEN_EMPIRE);
      stopEffect(S_VOICE_AFTERLIFE);
      stopEffect(S_VOICE_1989);
      stopEffect(S_VOICE_1984);

      playEffect(S_BEEPS_BARGRAPH);
      playEffect(S_VOICE_FROZEN_EMPIRE);
    break;

    case A_MODE_AFTERLIFE:
      // Play only the Afterlife voice
      stopEffect(S_BEEPS_BARGRAPH);
      stopEffect(S_VOICE_FROZEN_EMPIRE);
      stopEffect(S_VOICE_AFTERLIFE);
      stopEffect(S_VOICE_1989);
      stopEffect(S_VOICE_1984);

      playEffect(S_BEEPS_BARGRAPH);
      playEffect(S_VOICE_AFTERLIFE);
    break;

    case A_MODE_1989:
      // Play only the 1989 voice
      stopEffect(S_BEEPS_BARGRAPH);
      stopEffect(S_VOICE_FROZEN_EMPIRE);
      stopEffect(S_VOICE_AFTERLIFE);
      stopEffect(S_VOICE_1989);
      stopEffect(S_VOICE_1984);

      playEffect(S_BEEPS_BARGRAPH);
      playEffect(S_VOICE_1989);
    break;

    case A_MODE_1984:
      // Play only the 1984 voice
      stopEffect(S_BEEPS_BARGRAPH);
      stopEffect(S_VOICE_FROZEN_EMPIRE);
      stopEffect(S_VOICE_AFTERLIFE);
      stopEffect(S_VOICE_1989);
      stopEffect(S_VOICE_1984);

      playEffect(S_BEEPS_BARGRAPH);
      playEffect(S_VOICE_1984);
    break;

    case A_YEAR_MODE_DEFAULT:
      // Play only the default year voice
      stopEffect(S_VOICE_YEAR_MODE_DEFAULT);
      stopEffect(S_VOICE_FROZEN_EMPIRE);
      stopEffect(S_VOICE_AFTERLIFE);
      stopEffect(S_VOICE_1984);
      stopEffect(S_VOICE_1989);

      playEffect(S_VOICE_YEAR_MODE_DEFAULT);
    break;

    case A_SET_STREAM_MODE:
      if(vgModeCheck()) {
        // Only change our stream mode if VG mode is actually enabled.
        gpstarWand.setStreamMode((STREAM_MODES)i_value);

        // Apply the change immediately.
        streamModeCheck();
      }
    break;

    case A_SET_SMOKE:
      stopEffect(S_VOICE_SMOKE_DISABLED);
      stopEffect(S_VOICE_SMOKE_ENABLED);

      if(i_value == 0) {
        playEffect(S_VOICE_SMOKE_DISABLED);
      }
      else {
        playEffect(S_VOICE_SMOKE_ENABLED);
      }
    break;

    case A_SET_POWERCELL_INVERT:
      stopEffect(S_VOICE_POWERCELL_NOT_INVERTED);
      stopEffect(S_VOICE_POWERCELL_INVERTED);

      if(i_value == 0) {
        playEffect(S_VOICE_POWERCELL_NOT_INVERTED);
      }
      else {
        playEffect(S_VOICE_POWERCELL_INVERTED);
      }
    break;

    case A_SET_INNER_CYCLOTRON_INVERT:
      stopEffect(S_VOICE_INNER_CYCLOTRON_NOT_INVERTED);
      stopEffect(S_VOICE_INNER_CYCLOTRON_INVERTED);

      if(i_value == 0) {
        playEffect(S_VOICE_INNER_CYCLOTRON_NOT_INVERTED);
      }
      else {
        playEffect(S_VOICE_INNER_CYCLOTRON_INVERTED);
      }
    break;

    case A_CYCLOTRON_COUNTER_CLOCKWISE:
      // Play Cyclotron counter clockwise voice.
      stopEffect(S_VOICE_CYCLOTRON_CLOCKWISE);
      stopEffect(S_VOICE_CYCLOTRON_COUNTER_CLOCKWISE);

      playEffect(S_VOICE_CYCLOTRON_COUNTER_CLOCKWISE);
    break;

    case A_CYCLOTRON_CLOCKWISE:
      // Play Cyclotron clockwise voice.
      stopEffect(S_VOICE_CYCLOTRON_CLOCKWISE);
      stopEffect(S_VOICE_CYCLOTRON_COUNTER_CLOCKWISE);

      playEffect(S_VOICE_CYCLOTRON_CLOCKWISE);
    break;

    case A_CYCLOTRON_SINGLE_LED:
      // Play Single LED voice.
      stopEffect(S_VOICE_THREE_LED);
      stopEffect(S_VOICE_SINGLE_LED);

      playEffect(S_VOICE_SINGLE_LED);
    break;

    case A_CYCLOTRON_THREE_LED:
      // Play 3 LED voice.
      stopEffect(S_VOICE_THREE_LED);
      stopEffect(S_VOICE_SINGLE_LED);

      playEffect(S_VOICE_THREE_LED);
    break;

    case A_SET_VIDEO_GAME_MODE_COLOURS:
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_DISABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_ENABLED);
      if(i_value == 0) {
        playEffect(S_VOICE_VIDEO_GAME_COLOURS_DISABLED);
      }
      else {
        playEffect(S_VOICE_VIDEO_GAME_COLOURS_ENABLED);
      }
    break;

    case A_VIDEO_GAME_MODE_POWER_CELL_ENABLED:
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_DISABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_ENABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_POWERCELL_ENABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_CYCLOTRON_ENABLED);

      playEffect(S_VOICE_VIDEO_GAME_COLOURS_POWERCELL_ENABLED);
    break;

    case A_VIDEO_GAME_MODE_CYCLOTRON_ENABLED:
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_DISABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_ENABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_POWERCELL_ENABLED);
      stopEffect(S_VOICE_VIDEO_GAME_COLOURS_CYCLOTRON_ENABLED);

      playEffect(S_VOICE_VIDEO_GAME_COLOURS_CYCLOTRON_ENABLED);
    break;

    case A_DIMMING:
      stopEffect(S_BEEPS);
      playEffect(S_BEEPS);
    break;

    case A_SET_CONTINUOUS_SMOKE_5:
      if(i_value) {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_5_ENABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_5_DISABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_5_ENABLED);
      }
      else {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_5_DISABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_5_ENABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_5_DISABLED);
      }
    break;

    case A_SET_CONTINUOUS_SMOKE_4:
      if(i_value) {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_4_ENABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_4_DISABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_4_ENABLED);
      }
      else {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_4_DISABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_4_ENABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_4_DISABLED);
      }
    break;

    case A_SET_CONTINUOUS_SMOKE_3:
      if(i_value) {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_3_ENABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_3_DISABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_3_ENABLED);
      }
      else {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_3_DISABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_3_ENABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_3_DISABLED);
      }
    break;

    case A_SET_CONTINUOUS_SMOKE_2:
      if(i_value) {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_2_ENABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_2_DISABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_2_ENABLED);
      }
      else {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_2_DISABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_2_ENABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_2_DISABLED);
      }
    break;

    case A_SET_CONTINUOUS_SMOKE_1:
      if(i_value) {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_1_ENABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_1_DISABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_1_ENABLED);
      }
      else {
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_1_DISABLED);
        stopEffect(S_VOICE_CONTINUOUS_SMOKE_1_ENABLED);
        playEffect(S_VOICE_CONTINUOUS_SMOKE_1_DISABLED);
      }
    break;

    case A_SET_OVERHEAT_STROBE:
      // Overheat strobe state from Pack (d1: 0=DISABLED, 1=ENABLED)
      stopEffect(S_VOICE_OVERHEAT_STROBE_DISABLED);
      stopEffect(S_VOICE_OVERHEAT_STROBE_ENABLED);
      if(i_value == 1) {
        playEffect(S_VOICE_OVERHEAT_STROBE_ENABLED);
      }
      else {
        playEffect(S_VOICE_OVERHEAT_STROBE_DISABLED);
      }
    break;

    case A_SET_OVERHEAT_LIGHTS_OFF:
      // Overheat lights off state from Pack (d1: 0=DISABLED, 1=ENABLED)
      stopEffect(S_VOICE_OVERHEAT_LIGHTS_OFF_DISABLED);
      stopEffect(S_VOICE_OVERHEAT_LIGHTS_OFF_ENABLED);
      if(i_value == 1) {
        playEffect(S_VOICE_OVERHEAT_LIGHTS_OFF_ENABLED);
      }
      else {
        playEffect(S_VOICE_OVERHEAT_LIGHTS_OFF_DISABLED);
      }
    break;

    case A_SET_OVERHEAT_SYNC_FAN:
      // Overheat sync fan state from Pack (d1: 0=DISABLED, 1=ENABLED)
      stopEffect(S_VOICE_OVERHEAT_FAN_SYNC_DISABLED);
      stopEffect(S_VOICE_OVERHEAT_FAN_SYNC_ENABLED);
      if(i_value == 1) {
        playEffect(S_VOICE_OVERHEAT_FAN_SYNC_ENABLED);
      }
      else {
        playEffect(S_VOICE_OVERHEAT_FAN_SYNC_DISABLED);
      }
    break;

    case A_POWERCELL_DIMMING:
      stopEffect(S_VOICE_POWERCELL_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_INNER_BRIGHTNESS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_PANEL_BRIGHTNESS);

      playEffect(S_VOICE_POWERCELL_BRIGHTNESS);
    break;

    case A_CYCLOTRON_DIMMING:
      stopEffect(S_VOICE_POWERCELL_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_INNER_BRIGHTNESS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_PANEL_BRIGHTNESS);

      playEffect(S_VOICE_CYCLOTRON_BRIGHTNESS);
    break;

    case A_INNER_CYCLOTRON_DIMMING:
      stopEffect(S_VOICE_POWERCELL_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_INNER_BRIGHTNESS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_PANEL_BRIGHTNESS);

      playEffect(S_VOICE_CYCLOTRON_INNER_BRIGHTNESS);
    break;

    case A_CYCLOTRON_PANEL_DIMMING:
      stopEffect(S_VOICE_POWERCELL_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_BRIGHTNESS);
      stopEffect(S_VOICE_CYCLOTRON_INNER_BRIGHTNESS);
      stopEffect(S_VOICE_INNER_CYCLOTRON_PANEL_BRIGHTNESS);

      playEffect(S_VOICE_INNER_CYCLOTRON_PANEL_BRIGHTNESS);
    break;

    case A_SET_CYCLOTRON_FADING:
      stopEffect(S_VOICE_CYCLOTRON_FADING_DISABLED);
      stopEffect(S_VOICE_CYCLOTRON_FADING_ENABLED);

      if(i_value == 0) {
        playEffect(S_VOICE_CYCLOTRON_FADING_DISABLED);
      }
      else {
        playEffect(S_VOICE_CYCLOTRON_FADING_ENABLED);
      }
    break;

    case A_SET_CYCLOTRON_SIMULATE_RING:
      // Cyclotron simulate ring state from Pack (d1: 0=DISABLED, 1=ENABLED)
      stopEffect(S_VOICE_CYCLOTRON_SIMULATE_RING_DISABLED);
      stopEffect(S_VOICE_CYCLOTRON_SIMULATE_RING_ENABLED);
      if(i_value == 1) {
        playEffect(S_VOICE_CYCLOTRON_SIMULATE_RING_ENABLED);
      }
      else {
        playEffect(S_VOICE_CYCLOTRON_SIMULATE_RING_DISABLED);
      }
    break;

    case A_SET_DEMO_LIGHT_MODE:
      // Demo light mode state from Pack.
      // i_value: 0=DISABLED, 1=ENABLED
      stopEffect(S_VOICE_DEMO_LIGHT_MODE_DISABLED);
      stopEffect(S_VOICE_DEMO_LIGHT_MODE_ENABLED);

      if(i_value == 1) {
        playEffect(S_VOICE_DEMO_LIGHT_MODE_ENABLED);
      }
      else {
        playEffect(S_VOICE_DEMO_LIGHT_MODE_DISABLED);
      }
    break;

    case A_RGB_INNER_CYCLOTRON_LEDS:
      stopEffect(S_VOICE_RGB_INNER_CYCLOTRON);
      stopEffect(S_VOICE_GRB_INNER_CYCLOTRON);

      playEffect(S_VOICE_RGB_INNER_CYCLOTRON);
    break;

    case A_GRB_INNER_CYCLOTRON_LEDS:
      stopEffect(S_VOICE_GRB_INNER_CYCLOTRON);
      stopEffect(S_VOICE_RGB_INNER_CYCLOTRON);

      playEffect(S_VOICE_GRB_INNER_CYCLOTRON);
    break;

    case A_SET_CYCLOTRON_LED_COUNT:
      // Set cyclotron LED count (d1: 12, 20, 36, 40)
      stopEffect(S_VOICE_CYCLOTRON_40);
      stopEffect(S_VOICE_CYCLOTRON_36);
      stopEffect(S_VOICE_CYCLOTRON_20);
      stopEffect(S_VOICE_CYCLOTRON_12);

      switch(i_value) {
        case 40:
          playEffect(S_VOICE_CYCLOTRON_40);
        break;

        case 36:
          playEffect(S_VOICE_CYCLOTRON_36);
        break;

        case 20:
          playEffect(S_VOICE_CYCLOTRON_20);
        break;

        case 12:
        default:
          playEffect(S_VOICE_CYCLOTRON_12);
        break;
      }
    break;

    case A_SET_POWERCELL_LED_COUNT:
      // Set powercell LED count (d1: 13, 15)
      stopEffect(S_VOICE_POWERCELL_15);
      stopEffect(S_VOICE_POWERCELL_13);

      switch(i_value) {
        case 15:
          playEffect(S_VOICE_POWERCELL_15);
        break;

        case 13:
        default:
          playEffect(S_VOICE_POWERCELL_13);
        break;
      }
    break;

    case A_SET_INNER_CYCLOTRON_LED_COUNT:
      // Set inner cyclotron LED count (d1: 12, 23, 24, 26, 35, 36)
      stopEffect(S_VOICE_INNER_CYCLOTRON_36);
      stopEffect(S_VOICE_INNER_CYCLOTRON_35);
      stopEffect(S_VOICE_INNER_CYCLOTRON_26);
      stopEffect(S_VOICE_INNER_CYCLOTRON_24);
      stopEffect(S_VOICE_INNER_CYCLOTRON_23);
      stopEffect(S_VOICE_INNER_CYCLOTRON_12);

      switch(i_value) {
        case 36:
          playEffect(S_VOICE_INNER_CYCLOTRON_36);
        break;

        case 35:
          playEffect(S_VOICE_INNER_CYCLOTRON_35);
        break;

        case 26:
          playEffect(S_VOICE_INNER_CYCLOTRON_26);
        break;

        case 24:
          playEffect(S_VOICE_INNER_CYCLOTRON_24);
        break;

        case 23:
          playEffect(S_VOICE_INNER_CYCLOTRON_23);
        break;

        case 12:
        default:
          playEffect(S_VOICE_INNER_CYCLOTRON_12);
        break;
      }
    break;

    case A_SET_PACK_GPSTAR_AUDIO_LED:
      // Pack GPStar Audio LED state from Pack (d1: 0=DISABLED, 1=ENABLED)
      stopEffect(S_VOICE_PROTON_PACK_GPSTAR_AUDIO_LED_DISABLED);
      stopEffect(S_VOICE_PROTON_PACK_GPSTAR_AUDIO_LED_ENABLED);
      if(i_value == 1) {
        playEffect(S_VOICE_PROTON_PACK_GPSTAR_AUDIO_LED_ENABLED);
      }
      else {
        playEffect(S_VOICE_PROTON_PACK_GPSTAR_AUDIO_LED_DISABLED);
      }
    break;

    case A_SET_QUICK_BOOTUP:
      // Quick bootup state from Pack (d1: 0=DISABLED, 1=ENABLED)
      stopEffect(S_VOICE_QUICK_BOOTUP_ENABLED);
      stopEffect(S_VOICE_QUICK_BOOTUP_DISABLED);
      if(i_value == 1) {
        playEffect(S_VOICE_QUICK_BOOTUP_ENABLED);
      }
      else {
        playEffect(S_VOICE_QUICK_BOOTUP_DISABLED);
      }
    break;

    case A_TURN_WAND_ON:
      if(WAND_STATUS == MODE_OFF && gpstarWand.getSystemMode() == MODE_SUPER_HERO) {
        if(switch_activate.on() && WAND_ACTION_STATUS == ACTION_IDLE) {
          // Turn wand and pack on.
          WAND_ACTION_STATUS = ACTION_ACTIVATE;
        }
      }
    break;

    case A_SAVE_EEPROM_SETTINGS_WAND:
      // Commit changes to the EEPROM in the wand controller
      saveLEDEEPROM();
      saveConfigEEPROM();
      stopEffect(S_VOICE_EEPROM_SAVE);
      playEffect(S_VOICE_EEPROM_SAVE);
    break;

    case A_RESET_EEPROM_SETTINGS_WAND:
      // Reset the EEPROM on the wand controller
      clearLEDEEPROM();
      clearConfigEEPROM();
      stopEffect(S_VOICE_EEPROM_ERASE);
      playEffect(S_VOICE_EEPROM_ERASE);
    break;

    default:
      // No-op for anything else.
    break;
  }
}
