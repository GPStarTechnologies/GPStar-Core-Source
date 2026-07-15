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

void checkWandAction() {
  switch(WAND_ACTION_STATUS) {
    case ACTION_IDLE:
    default:
      // Do nothing.
    break;

    case ACTION_OFF:
      b_wand_mash_lockout = false;
      wandOff();
    break;

    case ACTION_FIRING:
      if(b_pack_on && !b_pack_alarm) {
        if(gpstarWand.inStreamMode(MESON)) {
          if(ms_meson_blast.justFinished()) {
            if(i_pack_audio_version < 109) {
              // Only retransmit if the pack is on an older audio board.
              packSerialSend(A_MESON_FIRE_PULSE);
            }

            if(i_audio_version < 109) {
              // Only manually repeat if our audio board is an older revision.
              playEffect(S_MESON_FIRE_PULSE, false, i_volume_effects, false, 0, false);
            }

            if(i_num_barrel_leds == 48) {
              // Reset the barrel before starting a new pulse on the high-density barrels.
              barrelLightsOff();
            }

            ms_firing_stream_effects.start(0); // Start new barrel animation.
            ms_meson_blast.repeat(); // Repeat the timer.
            firingCheckIR(); // Handle IR for meson pulse.
          }
        }

        if(!b_firing) {
          b_firing = true;
          modeFireStart();
        }

        if(ms_warning_blink.justFinished()) {
          ms_warning_blink.repeat();
        }

        // Overheating check, start vent sequence if expected for power level and timer delay is completed.
        if(ms_overheat_initiate.justFinished()) {
          startVentSequence();
        }
        else {
          modeFiring(); // Tell the pack whether firing has started/stopped.

          // Stop firing if any of the main switches are turned off or the barrel is retracted.
          if(!switch_vent.on() || !switch_wand.on() || gpstarWand.getBarrelState() != BARREL_EXTENDED) {
            modeFireStop();
            return;
          }
        }

        // Send infrared command using the IR manager when firing.
        if(!gpstarWand.inStreamMode(MESON)) {
          firingCheckIR();
        }
      }
      else if(b_pack_alarm && b_firing) {
        modeFireStop();
      }
    break;

    case ACTION_OVERHEATING:
      if(b_overheat_bargraph_blink) {
        settingsBlinkingLights();

        if(ms_blink_sound_timer_1.justFinished()) {
          if(b_extra_pack_sounds) {
            packSerialSend(A_WAND_BEEP_SOUNDS);
          }

          playEffect(S_BEEPS_LOW, false, i_volume_effects, false, 0, false);
          playEffect(S_BEEPS, false, i_volume_effects, false, 0, false);

          ms_blink_sound_timer_1.repeat();
        }

        if(ms_blink_sound_timer_2.justFinished()) {
          if(b_extra_pack_sounds) {
            packSerialSend(A_WAND_BEEP_BARGRAPH);
          }

          playEffect(S_BEEPS_BARGRAPH, false, i_volume_effects, false, 0, false);

          ms_blink_sound_timer_2.repeat();
        }
      }

      if(ms_overheating.justFinished()) {
        overheatingFinished();
      }
    break;

    case ACTION_VENTING:
      // Since the Proton Pack tells the Neutrona Wand when venting is finished, standalone wand needs its own timer.
      if(ms_overheating.justFinished()) {
        quickVentFinished();
      }
    break;

    case ACTION_ERROR:
      // nothing.
    break;

    case ACTION_ACTIVATE:
      modeActivate();
    break;

    case ACTION_LED_EEPROM_MENU:
      settingsBlinkingLights();

      switch(i_wand_menu) {
        // Level 1 Intensify: Clear the EEPROM settings and exit.
        // Level 1 Barrel Wing Button: Save the current settings to the EEPROM and exit.
        // Level 2 Intensify: Toggle 84/89 outer cyclotron fade effect.
        // Level 2 Barrel Wing Button: Toggle brightness for Proton Pack LEDs.
        // Level 3 Intensify: Toggle GPStar Audio LED state on Proton Pack.
        // Level 3 Barrel Wing Button: Toggle GPStar Audio LED state on Neutrona Wand.
        case 5:
          // Tell the Proton Pack to clear the EEPROM settings and exit.
          if(switch_intensify.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Toggle Proton Pack GPStar Audio LED status.
                packSerialSend(A_SET_PACK_GPSTAR_AUDIO_LED, 2);
              break;

              case MENU_LEVEL_2:
                // Toggle 84/89 outer cyclotron fade effect.
                packSerialSend(A_SET_CYCLOTRON_FADING, 2);
              break;

              case MENU_LEVEL_1:
              default:
                // Tell pack to clear the EEPROM and exit.
                packSerialSend(A_CLEAR_LED_EEPROM_SETTINGS);
                packSerialSend(A_SET_SPECTRAL_LIGHTS, 0);

                stopEffect(S_VOICE_EEPROM_ERASE);
                playEffect(S_VOICE_EEPROM_ERASE);

                clearLEDEEPROM();

                wandExitEEPROMMenu();
              break;
            }
          }
          else {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                if(switch_mode.pushed()) {
                  // Toggle Neutrona Wand GPStar Audio LED status.
                  stopEffect(S_VOICE_NEUTRONA_WAND_GPSTAR_AUDIO_LED_DISABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_GPSTAR_AUDIO_LED_ENABLED);

                  if(b_gpstar_audio_led_enabled) {
                    // Turn off GPStar Audio LED.
                    b_gpstar_audio_led_enabled = false;
                    playEffect(S_VOICE_NEUTRONA_WAND_GPSTAR_AUDIO_LED_DISABLED);
                    packSerialSend(A_SET_WAND_GPSTAR_AUDIO_LED, 0);
                  }
                  else {
                    // Turn on GPStar Audio LED.
                    b_gpstar_audio_led_enabled = true;
                    playEffect(S_VOICE_NEUTRONA_WAND_GPSTAR_AUDIO_LED_ENABLED);
                    packSerialSend(A_SET_WAND_GPSTAR_AUDIO_LED, 1);
                  }

                  setAudioLED(b_gpstar_audio_led_enabled);
                }
              break;

              case MENU_LEVEL_2:
                if(switch_mode.singleClick()) {
                  // Change which device we are currently dimming.
                  // Note that the actual dimming is handled by checkRotaryEncoder().
                  packSerialSend(A_DIMMING_TOGGLE);
                }
              break;

              case MENU_LEVEL_1:
              default:
                if(switch_mode.pushed()) {
                  // Tell the Proton Pack to save the current settings to the EEPROM and exit.
                  packSerialSend(A_SAVE_LED_EEPROM_SETTINGS);
                  packSerialSend(A_SET_SPECTRAL_LIGHTS, 0);

                  stopEffect(S_VOICE_EEPROM_SAVE);
                  playEffect(S_VOICE_EEPROM_SAVE);

                  saveLEDEEPROM();

                  wandExitEEPROMMenu();
                }
              break;
            }
          }
        break;

        // Level 1 Intensify: Cycle through the different Neutrona Wand barrel LED counts.
        // Level 1 Barrel Wing Button: Adjust the Neutrona Wand barrel colour hue. <- Controlled by checkRotaryEncoder()
        // Level 2 Intensify: Toggle between 28-segment and 30-segment bargraph LEDs.
        // Level 2 Barrel Wing Button: Enable/Disable the addressable RGB vent/top light board (non-ESP32 only).
        // Level 3 Intensify: Toggle between 1 or 3 LEDs for the Cyclotron (1984/1989 mode).
        case 4:
          if(switch_intensify.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Tell the Proton Pack to toggle the Single LED or 3 LEDs for 1984/1989 modes.
                packSerialSend(A_CYCLOTRON_LED_TOGGLE);
              break;

              case MENU_LEVEL_2:
                if(BARGRAPH_TYPE_EEPROM != SEGMENTS_30) {
                  // Switch to 30-segment bargraph.
                  BARGRAPH_TYPE_EEPROM = SEGMENTS_30;

                  stopEffect(S_VOICE_BARGRAPH_28_SEGMENTS);
                  stopEffect(S_VOICE_BARGRAPH_30_SEGMENTS);

                  playEffect(S_VOICE_BARGRAPH_30_SEGMENTS);

                  packSerialSend(A_BARGRAPH_30_SEGMENTS);
                }
                else {
                  // Switch to 28-segment bargraph.
                  BARGRAPH_TYPE_EEPROM = SEGMENTS_28;

                  stopEffect(S_VOICE_BARGRAPH_28_SEGMENTS);
                  stopEffect(S_VOICE_BARGRAPH_30_SEGMENTS);

                  playEffect(S_VOICE_BARGRAPH_28_SEGMENTS);

                  packSerialSend(A_BARGRAPH_28_SEGMENTS);
                }

                if(BARGRAPH_TYPE == SEGMENTS_28 || BARGRAPH_TYPE == SEGMENTS_30) {
                  // Only toggle between segment types if not on a stock Hasbro bargraph.
                  BARGRAPH_TYPE = BARGRAPH_TYPE_EEPROM;
                }
              break;

              case MENU_LEVEL_1:
              default:
                wandBarrelLightsOff();
                wandTipOff();

                switch(WAND_BARREL_LED) {
                  case HASBRO_BARREL:
                    WAND_BARREL_LED = FRUTTO_BARREL;
                    i_num_barrel_leds = 48;

                    stopEffect(S_VOICE_HASBRO_BARREL);
                    //stopEffect(S_VOICE_FRUTTO_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL_II);
                    //stopEffect(S_VOICE_BARREL_LED_MINI);

                    playEffect(S_VOICE_FRUTTO_BARREL);

                    packSerialSend(A_SET_BARREL_TYPE, 2);
                  break;

                  case FRUTTO_BARREL:
                    WAND_BARREL_LED = GPSTAR_BARREL;
                    i_num_barrel_leds = 48;

                    //stopEffect(S_VOICE_HASBRO_BARREL);
                    stopEffect(S_VOICE_FRUTTO_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL_II);
                    //stopEffect(S_VOICE_BARREL_LED_MINI);

                    playEffect(S_VOICE_GPSTAR_BARREL);

                    packSerialSend(A_SET_BARREL_TYPE, 3);
                  break;

                  case GPSTAR_BARREL:
                  default:
                    WAND_BARREL_LED = GPSTAR_BARREL_II;
                    i_num_barrel_leds = 48;

                    //stopEffect(S_VOICE_HASBRO_BARREL);
                    //stopEffect(S_VOICE_FRUTTO_BARREL);
                    stopEffect(S_VOICE_GPSTAR_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL_II);
                    //stopEffect(S_VOICE_BARREL_LED_MINI);

                    playEffect(S_VOICE_GPSTAR_BARREL_II);

                    packSerialSend(A_SET_BARREL_TYPE, 4);
                  break;

                  case GPSTAR_BARREL_II:
                    WAND_BARREL_LED = GPSTAR_BARREL_MINI;
                    i_num_barrel_leds = 2;

                    //stopEffect(S_VOICE_HASBRO_BARREL);
                    //stopEffect(S_VOICE_FRUTTO_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL);
                    stopEffect(S_VOICE_GPSTAR_BARREL_II);
                    //stopEffect(S_VOICE_BARREL_LED_MINI);

                    playEffect(S_VOICE_BARREL_LED_MINI);

                    packSerialSend(A_SET_BARREL_TYPE, 5);
                  break;

                  case GPSTAR_BARREL_MINI:
                    WAND_BARREL_LED = HASBRO_BARREL;
                    i_num_barrel_leds = 5;

                    //stopEffect(S_VOICE_HASBRO_BARREL);
                    //stopEffect(S_VOICE_FRUTTO_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL);
                    //stopEffect(S_VOICE_GPSTAR_BARREL_II);
                    stopEffect(S_VOICE_BARREL_LED_MINI);

                    playEffect(S_VOICE_HASBRO_BARREL);

                    packSerialSend(A_SET_BARREL_TYPE, 1);
                  break;
                }

                wandBarrelSpectralCustomConfigOn();
              break;
            }
          }
          else if(switch_mode.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
              #ifndef ESP32
                if(b_rgb_vent_light) {
                  // Disable the RGB vent light functionality.
                  b_rgb_vent_light = false;

                  stopEffect(S_VOICE_RGB_VENT_LIGHTS_ENABLED);
                  stopEffect(S_VOICE_RGB_VENT_LIGHTS_DISABLED);

                  playEffect(S_VOICE_RGB_VENT_LIGHTS_DISABLED);

                  packSerialSend(A_SET_RGB_VENT, 0);
                }
                else {
                  // Enable the RGB vent light functionality.
                  b_rgb_vent_light = true;

                  stopEffect(S_VOICE_RGB_VENT_LIGHTS_ENABLED);
                  stopEffect(S_VOICE_RGB_VENT_LIGHTS_DISABLED);

                  playEffect(S_VOICE_RGB_VENT_LIGHTS_ENABLED);

                  packSerialSend(A_SET_RGB_VENT, 1);
                }
              #endif
              break;

              case MENU_LEVEL_1:
              default:
                // Do nothing as this is controlled by checkRotaryEncoder().
              break;
            }
          }
        break;

        // Level 1 Intensify: Cycle through the different Power Cell LED counts.
        // Level 1 Barrel Wing Button: Adjust the Power Cell colour hue. <- Controlled by checkRotaryEncoder()
        // Level 2 Intensify: Toggle inverting of Power Cell LED direction (required for 1984 Power Cell).
        // Level 2 Barrel Wing Button: Toggle the auto vent light intensity control on/off.
        case 3:
          if(switch_intensify.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
                packSerialSend(A_TOGGLE_POWERCELL_DIRECTION);
              break;

              case MENU_LEVEL_1:
              default:
                packSerialSend(A_SET_POWERCELL_LED_COUNT, 2); // d1: 2 = toggle to next
              break;
            }
          }
          else if(switch_mode.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
                if(b_vent_light_control) {
                  // Disable the auto vent light intensity functionality.
                  b_vent_light_control = false;

                  stopEffect(S_VOICE_VENT_LIGHT_INTENSITY_ENABLED);
                  stopEffect(S_VOICE_VENT_LIGHT_INTENSITY_DISABLED);

                  playEffect(S_VOICE_VENT_LIGHT_INTENSITY_DISABLED);

                  packSerialSend(A_SET_AUTO_VENT_INTENSITY, 0);
                }
                else {
                  // Enable the auto vent light intensity functionality.
                  b_vent_light_control = true;

                  stopEffect(S_VOICE_VENT_LIGHT_INTENSITY_ENABLED);
                  stopEffect(S_VOICE_VENT_LIGHT_INTENSITY_DISABLED);

                  playEffect(S_VOICE_VENT_LIGHT_INTENSITY_ENABLED);

                  packSerialSend(A_SET_AUTO_VENT_INTENSITY, 1);
                }
              break;

              case MENU_LEVEL_1:
              default:
                // Do nothing as this is controlled by checkRotaryEncoder().
              break;
            }
          }
        break;

        // Level 1 Intensify: Cycle through the different Cyclotron LED counts.
        // Level 1 Barrel Wing Button: Adjust the Cyclotron colour hue. <- Controlled by checkRotaryEncoder()
        // Level 2 Intensify: Enable or disable the Inner Cyclotron LED Panel.
        // Level 2 Barrel Wing Button: Cycle through VG colour modes to disable them (see operational guide for more details on this).
        case 2:
          if(switch_intensify.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
                packSerialSend(A_TOGGLE_INNER_CYCLOTRON_PANEL);
              break;

              case MENU_LEVEL_1:
              default:
                packSerialSend(A_SET_CYCLOTRON_LED_COUNT, 2); // d1: 2 = toggle to next
              break;
            }
          }
          else if(switch_mode.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
                // Enable or disable video game colours for the Power Cell, Cyclotron, etc.
                packSerialSend(A_VIDEO_GAME_MODE_COLOUR_TOGGLE);
              break;

              case MENU_LEVEL_1:
              default:
                // Do nothing as this is controlled by checkRotaryEncoder().
              break;
            }
          }
        break;


        // Level 1 Intensify: Cycle through the different inner Cyclotron LED counts.
        // Level 1 Barrel Wing Button: Adjust the Inner Cyclotron colour hue. <- Controlled by checkRotaryEncoder()
        // Level 2 Intensify: Enable or disable GRB mode for the inner Cyclotron LEDs.
        // Level 2 Barrel Wing Button: Enable or disable having the addressable RGB vent/top light board change colours based on current stream
        case 1:
          if(switch_intensify.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
                packSerialSend(A_TOGGLE_RGB_INNER_CYCLOTRON_LEDS);
              break;

              case MENU_LEVEL_1:
              default:
                packSerialSend(A_SET_INNER_CYCLOTRON_LED_COUNT, 2); // d1: 2 = toggle to next
              break;
            }
          }
          else if(switch_mode.pushed()) {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_3:
                // Do nothing.
              break;

              case MENU_LEVEL_2:
                if(b_vent_light_stream_colours) {
                  b_vent_light_stream_colours = false;

                  stopEffect(S_VOICE_VENT_LIGHT_COLOURS_ENABLED);
                  stopEffect(S_VOICE_VENT_LIGHT_COLOURS_DISABLED);

                  playEffect(S_VOICE_VENT_LIGHT_COLOURS_DISABLED);

                  packSerialSend(A_SET_VENT_LIGHT_COLOURS, 0);
                }
                else {
                  b_vent_light_stream_colours = true;

                  stopEffect(S_VOICE_VENT_LIGHT_COLOURS_ENABLED);
                  stopEffect(S_VOICE_VENT_LIGHT_COLOURS_DISABLED);

                  playEffect(S_VOICE_VENT_LIGHT_COLOURS_ENABLED);

                  packSerialSend(A_SET_VENT_LIGHT_COLOURS, 1);
                }
              break;

              case MENU_LEVEL_1:
              default:
                // Do nothing; this is controlled in checkRotaryEncoder() instead.
              break;
            }
          }
        break;
      }
    break;

    case ACTION_CONFIG_EEPROM_MENU:
      settingsBlinkingLights();

      switch(i_wand_menu) {
        // Menu Level 1: Intensify: Clear the Neutrona Wand EEPROM settings and exit.
        // Menu Level 1: Barrel Wing Button: Save the current settings to the Neutrona Wand EEPROM and exit.
        // Menu Level 2: Intensify: Quick Vent.
        // Menu Level 2: Barrel Wing Button: Wand Boot Errors.
        // Menu Level 3: Intensify: Toggle between Neutrona Wand and Proton Pack default startup volume adjustment.
        // Menu Level 3: Barrel Wing Button: Set Neutrona Wand year mode (84/89/AL/FE/Match Proton Pack).
        // Menu Level 4: Intensify + top dial: Adjust overheat smoke duration by 1 second : Power Level 5
        // Menu Level 4: Barrel Wing Button + top dial: Adjust overheat start timer by 1 second : Power Level 5
        // Menu Level 5: Intensify: Enable/Disable overheat in power level #5
        // Menu Level 5: Barrel Wing Button: Enable/Disable sustained smoke in power level #5
        case 5:
          if(switch_mode.pushed()) {
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              // Tell the Proton Pack to save its current configuration to the EEPROM.
              packSerialSend(A_SAVE_CONFIG_EEPROM_SETTINGS);

              stopEffect(S_VOICE_EEPROM_SAVE);
              playEffect(S_VOICE_EEPROM_SAVE);

              // Save wand EEPROM. (CTS/VGA, Overheating)
              saveConfigEEPROM();

              wandExitEEPROMMenu();
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              if(b_wand_boot_errors) {
                b_wand_boot_errors = false;

                stopEffect(S_VOICE_BOOTUP_ERRORS_DISABLED);
                stopEffect(S_VOICE_BOOTUP_ERRORS_ENABLED);
                playEffect(S_VOICE_BOOTUP_ERRORS_DISABLED);

                packSerialSend(A_SET_BOOTUP_ERRORS, 0);
              }
              else {
                b_wand_boot_errors = true;

                stopEffect(S_VOICE_BOOTUP_ERRORS_ENABLED);
                stopEffect(S_VOICE_BOOTUP_ERRORS_DISABLED);
                playEffect(S_VOICE_BOOTUP_ERRORS_ENABLED);

                packSerialSend(A_SET_BOOTUP_ERRORS, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              switch(WAND_YEAR_MODE) {
                case YEAR_1984:
                  // 1984 -> 1989
                  WAND_YEAR_MODE = YEAR_1989;

                  stopEffect(S_VOICE_NEUTRONA_WAND_DEFAULT_MODE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_FROZEN_EMPIRE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_AFTERLIFE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1989);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1984);

                  playEffect(S_VOICE_NEUTRONA_WAND_1989);

                  packSerialSend(A_NEUTRONA_WAND_1989_MODE);
                break;

                case YEAR_1989:
                  // 1989 -> Afterlife
                  WAND_YEAR_MODE = YEAR_AFTERLIFE;

                  stopEffect(S_VOICE_NEUTRONA_WAND_DEFAULT_MODE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_FROZEN_EMPIRE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_AFTERLIFE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1989);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1984);

                  playEffect(S_VOICE_NEUTRONA_WAND_AFTERLIFE);

                  packSerialSend(A_NEUTRONA_WAND_AFTERLIFE_MODE);
                break;

                case YEAR_AFTERLIFE:
                  // Afterlife -> Frozen Empire
                  WAND_YEAR_MODE = YEAR_FROZEN_EMPIRE;

                  stopEffect(S_VOICE_NEUTRONA_WAND_DEFAULT_MODE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_FROZEN_EMPIRE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_AFTERLIFE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1989);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1984);

                  playEffect(S_VOICE_NEUTRONA_WAND_FROZEN_EMPIRE);

                  packSerialSend(A_NEUTRONA_WAND_FROZEN_EMPIRE_MODE);
                break;

                case YEAR_FROZEN_EMPIRE:
                  // Frozen Empire -> Default (Toggle)
                  WAND_YEAR_MODE = YEAR_DEFAULT;

                  stopEffect(S_VOICE_NEUTRONA_WAND_DEFAULT_MODE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_FROZEN_EMPIRE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_AFTERLIFE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1989);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1984);

                  playEffect(S_VOICE_NEUTRONA_WAND_DEFAULT_MODE);

                  packSerialSend(A_NEUTRONA_WAND_DEFAULT_MODE);
                break;

                case YEAR_DEFAULT:
                default:
                  // Default (Toggle) -> 1984
                  WAND_YEAR_MODE = YEAR_1984;

                  stopEffect(S_VOICE_NEUTRONA_WAND_DEFAULT_MODE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_FROZEN_EMPIRE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_AFTERLIFE);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1989);
                  stopEffect(S_VOICE_NEUTRONA_WAND_1984);

                  playEffect(S_VOICE_NEUTRONA_WAND_1984);

                  packSerialSend(A_NEUTRONA_WAND_1984_MODE);
                break;
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Handled in checkRotaryEncoder()
              // The time it takes to overheat in power level 5.
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_1);

              playEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_5);

              packSerialSend(A_SOUND_OVERHEAT_START_TIMER, 5);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              smokeConfig.overheatContinuous5 = !smokeConfig.overheatContinuous5;
              packSerialSend(A_SET_CONTINUOUS_SMOKE_5, smokeConfig.overheatContinuous5 ? 1 : 0);
            }
          }
          else {
            switch(WAND_MENU_LEVEL) {
              case MENU_LEVEL_5:
                if(switch_intensify.pushed()) {
                  if(b_overheat_level_5) {
                    b_overheat_level_5 = false;

                    smokeConfig.overheatContinuous5 = false;

                    stopEffect(S_VOICE_OVERHEAT_LEVEL_5_DISABLED);
                    stopEffect(S_VOICE_OVERHEAT_LEVEL_5_ENABLED);
                    playEffect(S_VOICE_OVERHEAT_LEVEL_5_DISABLED);

                    packSerialSend(A_SET_OVERHEAT_LEVEL_5, 0);
                  }
                  else {
                    b_overheat_level_5 = true;

                    smokeConfig.overheatContinuous5 = true;

                    stopEffect(S_VOICE_OVERHEAT_LEVEL_5_ENABLED);
                    stopEffect(S_VOICE_OVERHEAT_LEVEL_5_DISABLED);
                    playEffect(S_VOICE_OVERHEAT_LEVEL_5_ENABLED);

                    packSerialSend(A_SET_OVERHEAT_LEVEL_5, 1);
                  }

                  updateOverheatLevels();
                }
              break;

              case MENU_LEVEL_4:
                if(switch_intensify.pushed()) {
                  // Overheat smoke duration level 5.
                  // Adjustment is handled in checkRotaryEncoder()
                  stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_5);
                  stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_4);
                  stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_3);
                  stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_2);
                  stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_1);
                  playEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_5);

                  packSerialSend(A_SOUND_OVERHEAT_SMOKE_DURATION, 5);
                }
              break;

              case MENU_LEVEL_3:
                if(switch_intensify.singleClick() && !b_wand_standalone) {
                  // Toggle between Neutrona Wand and Proton Pack default volume adjustment.
                  if(VOLUME_ADJUST_DEVICE == VOLUME_PROTON_PACK) {
                    VOLUME_ADJUST_DEVICE = VOLUME_NEUTRONA_WAND;

                    stopEffect(S_VOICE_NEUTRONA_WAND_VOLUME_ADJUSTMENT);
                    stopEffect(S_VOICE_PROTON_PACK_VOLUME_ADJUSTMENT);
                    playEffect(S_VOICE_NEUTRONA_WAND_VOLUME_ADJUSTMENT);

                    packSerialSend(A_NEUTRONA_WAND_VOLUME_ADJUSTMENT);
                  }
                  else {
                    VOLUME_ADJUST_DEVICE = VOLUME_PROTON_PACK;

                    stopEffect(S_VOICE_NEUTRONA_WAND_VOLUME_ADJUSTMENT);
                    stopEffect(S_VOICE_PROTON_PACK_VOLUME_ADJUSTMENT);
                    playEffect(S_VOICE_PROTON_PACK_VOLUME_ADJUSTMENT);

                    packSerialSend(A_PROTON_PACK_VOLUME_ADJUSTMENT);
                  }
                }
              break;

              case MENU_LEVEL_2:
                if(switch_intensify.pushed()) {
                  if(b_quick_vent) {
                    b_quick_vent = false;

                    stopEffect(S_VOICE_QUICK_VENT_DISABLED);
                    stopEffect(S_VOICE_QUICK_VENT_ENABLED);
                    playEffect(S_VOICE_QUICK_VENT_DISABLED);

                    packSerialSend(A_SET_QUICK_VENT, 0);
                  }
                  else {
                    b_quick_vent = true;

                    stopEffect(S_VOICE_QUICK_VENT_DISABLED);
                    stopEffect(S_VOICE_QUICK_VENT_ENABLED);
                    playEffect(S_VOICE_QUICK_VENT_ENABLED);

                    packSerialSend(A_SET_QUICK_VENT, 1);
                  }
                }
              break;

              case MENU_LEVEL_1:
              default:
                if(switch_intensify.pushed()) {
                  // Tell the Proton Pack to clear its current configuration from the EEPROM.
                  packSerialSend(A_CLEAR_CONFIG_EEPROM_SETTINGS);

                  stopEffect(S_VOICE_EEPROM_ERASE);
                  playEffect(S_VOICE_EEPROM_ERASE);

                  // Clear wand EEPROM.
                  clearConfigEEPROM();

                  wandExitEEPROMMenu();
                }
              break;
            }
          }
        break;

        // Menu Level 1: Intensify: Cycle through the modes (Video Game, Cross The Streams, Cross The Streams Mix)
        // Menu Level 1: Barrel Wing Button: Enable/Disable Spectral and Holiday modes.
        // Menu Level 2: Intensify: Enable pack vibration, enable pack vibration while firing only, disable pack vibration, reset to defaults. *Note that the pack vibration switch will toggle both pack and wand vibration on or off*
        // Menu Level 2: Barrel Wing Button: Enable wand vibration, enable wand vibration while firing only, disable wand vibration, reset to defaults.
        // Menu Level 3: Intensify: Invert Bargraph
        // Menu Level 3: Barrel Wing Button: Toggle Bargraph Overheat Blinking enabled/disabled
        // Menu Level 4: Intensify + top dial: Adjust overheat smoke duration by 1 second : Power Level 4
        // Menu Level 4: Barrel Wing Button + top dial: Adjust overheat start timer by 1 second : Power Level 4
        // Menu Level 5: Intensify: Enable/Disable overheat in power level #4
        // Menu Level 5: Barrel Wing Button: Enable/Disable sustained smoke in power level #4
        case 4:
          if(switch_intensify.pushed()) {
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              toggleWandModes();
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              packSerialSend(A_SET_VIBRATION_MODE, 6); // 6 = CYCLE_TOGGLE_EEPROM
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              if(b_bargraph_invert) {
                b_bargraph_invert = false;

                stopEffect(S_VOICE_BARGRAPH_INVERTED);
                stopEffect(S_VOICE_BARGRAPH_NOT_INVERTED);
                playEffect(S_VOICE_BARGRAPH_NOT_INVERTED);

                packSerialSend(A_SET_BARGRAPH_INVERT, 0);
              }
              else {
                b_bargraph_invert = true;

                stopEffect(S_VOICE_BARGRAPH_INVERTED);
                stopEffect(S_VOICE_BARGRAPH_NOT_INVERTED);
                playEffect(S_VOICE_BARGRAPH_INVERTED);

                packSerialSend(A_SET_BARGRAPH_INVERT, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Overheat smoke duration level 4.
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_1);
              playEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_4);

              // Handled in checkRotaryEncoder()
              packSerialSend(A_SOUND_OVERHEAT_SMOKE_DURATION, 4);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              if(b_overheat_level_4) {
                b_overheat_level_4 = false;

                smokeConfig.overheatContinuous4 = false;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_4_DISABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_4_ENABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_4_DISABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_4, 0);
              }
              else {
                b_overheat_level_4 = true;

                smokeConfig.overheatContinuous4 = true;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_4_ENABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_4_DISABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_4_ENABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_4, 1);
              }

              updateOverheatLevels();
            }
          }
          else if(switch_mode.pushed()) {
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              // Due to limited wand menu space, this toggles ALL spectral/holiday stream modes on or off.
              // This should account for the user manually adjusting individual modes outside of the menu.
              if(gpstarWand.supportsAllSpectralStreams()) {
                // Disable all spectral/holiday stream modes if ANY are enabled.
                gpstarWand.removeAllSpectralStreams();

                stopEffect(S_VOICE_SPECTRAL_MODES_DISABLED);
                stopEffect(S_VOICE_SPECTRAL_MODES_ENABLED);
                playEffect(S_VOICE_SPECTRAL_MODES_DISABLED);

                packSerialSend(A_SET_SPECTRAL_MODES, 0);
              }
              else {
                // Enable all spectral/holiday stream modes if NONE are enabled.
                gpstarWand.enableAllSpectralStreams();

                stopEffect(S_VOICE_SPECTRAL_MODES_DISABLED);
                stopEffect(S_VOICE_SPECTRAL_MODES_ENABLED);
                playEffect(S_VOICE_SPECTRAL_MODES_ENABLED);

                packSerialSend(A_SET_SPECTRAL_MODES, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              stopEffect(S_BEEPS_ALT);

              playEffect(S_BEEPS_ALT);

              switch(VIBRATION_MODE_EEPROM) {
                case VIBRATION_DEFAULT:
                default:
                  VIBRATION_MODE_EEPROM = VIBRATION_ALWAYS;
                  gpstarWand.setVibrationMode(VIBRATION_MODE_EEPROM);
                  b_vibration_switch_on = true; // Override the Proton Pack vibration toggle switch.

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DEFAULT);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);

                  packSerialSend(A_SET_VIBRATION_MODE, 1); // 1 = ALWAYS

                  ms_menu_vibration.start(250); // Confirmation buzz for 250ms.
                break;
                case VIBRATION_ALWAYS:
                  VIBRATION_MODE_EEPROM = VIBRATION_FIRING_ONLY;
                  gpstarWand.setVibrationMode(VIBRATION_MODE_EEPROM);
                  b_vibration_switch_on = true; // Override the Proton Pack vibration toggle switch.

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DEFAULT);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);

                  packSerialSend(A_SET_VIBRATION_MODE, 2); // 2 = FIRING_ONLY

                  ms_menu_vibration.start(250); // Confirmation buzz for 250ms.
                break;
                case VIBRATION_FIRING_ONLY:
                  VIBRATION_MODE_EEPROM = VIBRATION_NEVER;
                  gpstarWand.setVibrationMode(VIBRATION_MODE_EEPROM);

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DEFAULT);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);

                  packSerialSend(A_SET_VIBRATION_MODE, 3); // 3 = NEVER
                break;
                case VIBRATION_NEVER:
                  VIBRATION_MODE_EEPROM = VIBRATION_DEFAULT;
                  gpstarWand.setVibrationMode(VIBRATION_FIRING_ONLY);

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DEFAULT);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DEFAULT);

                  packSerialSend(A_SET_VIBRATION_MODE, 4); // 4 = DEFAULT

                  ms_menu_vibration.start(250); // Confirmation buzz for 250ms.
                break;
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              // Toggle Bargraph Overheat Blinking enabled/disabled
              if(b_overheat_bargraph_blink) {
                b_overheat_bargraph_blink = false;

                stopEffect(S_VOICE_BARGRAPH_OVERHEAT_BLINK_DISABLED);
                stopEffect(S_VOICE_BARGRAPH_OVERHEAT_BLINK_ENABLED);
                playEffect(S_VOICE_BARGRAPH_OVERHEAT_BLINK_DISABLED);

                packSerialSend(A_SET_BARGRAPH_OVERHEAT_BLINK, 0);
              }
              else {
                b_overheat_bargraph_blink = true;

                stopEffect(S_VOICE_BARGRAPH_OVERHEAT_BLINK_DISABLED);
                stopEffect(S_VOICE_BARGRAPH_OVERHEAT_BLINK_ENABLED);
                playEffect(S_VOICE_BARGRAPH_OVERHEAT_BLINK_ENABLED);

                packSerialSend(A_SET_BARGRAPH_OVERHEAT_BLINK, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Handled in checkRotaryEncoder()
              // The time it takes to overheat in power level 4.
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_1);

              playEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_4);

              packSerialSend(A_SOUND_OVERHEAT_START_TIMER, 4);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              smokeConfig.overheatContinuous4 = !smokeConfig.overheatContinuous4;
              packSerialSend(A_SET_CONTINUOUS_SMOKE_4, smokeConfig.overheatContinuous4 ? 1 : 0);
            }
          }
        break;

        // Menu Level 1: Intensify: Enable or Disable overheating settings.
        // Menu Level 1: Barrel Wing Button: Enable or disable smoke.
        // Menu Level 2: Intensify: Enable/Disable Wand beeping in Afterlife / Frozen Empire modes.
        // Menu Level 2: Barrel Wing Button: Barrel Safety Switch Polarity Toggle setting: Default / Inverted / Disabled
        // Menu Level 3: Intensify: Bargraph Idle Animation Toggle setting: Super Hero / Bargraph Original / System Default
        // Menu Level 3: Barrel Wing Button: Bargraph Firing Animation Toggle setting: Super Hero / Bargraph Original / System Default
        // Menu Level 4: Intensify + top dial: Adjust overheat smoke duration by 1 second : Power Level 3
        // Menu Level 4: Barrel Wing Button + top dial: Adjust overheat start timer by 1 second : Power Level 3
        // Menu Level 5: Intensify: Enable/Disable overheat in power level #3
        // Menu Level 5: Barrel Wing Button: Enable/Disable sustained smoke in power level #3
        case 3:
          if(switch_intensify.pushed()) {
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              toggleOverheating();
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              if(b_beep_loop) {
                b_beep_loop = false;

                stopEffect(S_VOICE_NEUTRONA_WAND_BEEPING_DISABLED);
                stopEffect(S_VOICE_NEUTRONA_WAND_BEEPING_ENABLED);
                playEffect(S_VOICE_NEUTRONA_WAND_BEEPING_DISABLED);

                packSerialSend(A_SET_MODE_BEEP_LOOP, 0);
              }
              else {
                b_beep_loop = true;

                stopEffect(S_VOICE_NEUTRONA_WAND_BEEPING_DISABLED);
                stopEffect(S_VOICE_NEUTRONA_WAND_BEEPING_ENABLED);
                playEffect(S_VOICE_NEUTRONA_WAND_BEEPING_ENABLED);

                packSerialSend(A_SET_MODE_BEEP_LOOP, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              switch(BARGRAPH_MODE_EEPROM) {
                case BARGRAPH_EEPROM_ORIGINAL:
                  BARGRAPH_MODE_EEPROM = BARGRAPH_EEPROM_SUPER_HERO;

                  stopEffect(S_VOICE_DEFAULT_BARGRAPH);
                  stopEffect(S_VOICE_SUPER_HERO_BARGRAPH);
                  stopEffect(S_VOICE_MODE_ORIGINAL_BARGRAPH);
                  playEffect(S_VOICE_SUPER_HERO_BARGRAPH);

                  packSerialSend(A_SUPER_HERO_BARGRAPH);
                break;

                case BARGRAPH_EEPROM_SUPER_HERO:
                  BARGRAPH_MODE_EEPROM = BARGRAPH_EEPROM_DEFAULT;

                  stopEffect(S_VOICE_DEFAULT_BARGRAPH);
                  stopEffect(S_VOICE_MODE_ORIGINAL_BARGRAPH);
                  stopEffect(S_VOICE_SUPER_HERO_BARGRAPH);
                  playEffect(S_VOICE_DEFAULT_BARGRAPH);

                  packSerialSend(A_DEFAULT_BARGRAPH);
                break;

                case BARGRAPH_EEPROM_DEFAULT:
                default:
                  BARGRAPH_MODE_EEPROM = BARGRAPH_EEPROM_ORIGINAL;

                  stopEffect(S_VOICE_DEFAULT_BARGRAPH);
                  stopEffect(S_VOICE_MODE_ORIGINAL_BARGRAPH);
                  stopEffect(S_VOICE_SUPER_HERO_BARGRAPH);
                  playEffect(S_VOICE_MODE_ORIGINAL_BARGRAPH);

                  packSerialSend(A_MODE_ORIGINAL_BARGRAPH);
                break;
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Overheat smoke duration level .
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_1);
              playEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_3);

              // Handled in checkRotaryEncoder()
              packSerialSend(A_SOUND_OVERHEAT_SMOKE_DURATION, 3);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              if(b_overheat_level_3) {
                b_overheat_level_3 = false;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_3_DISABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_3_ENABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_3_DISABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_3, 0);
              }
              else {
                b_overheat_level_3 = true;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_3_ENABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_3_DISABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_3_ENABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_3, 1);
              }
            }
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              // Enable or disable smoke.
              packSerialSend(A_SMOKE_TOGGLE);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              if(BARREL_SWITCH_POLARITY == SWITCH_DEFAULT) {
                // Change barrel safety switch polarity to inverted.
                BARREL_SWITCH_POLARITY = SWITCH_INVERTED;

                // Reset the barrel state to prevent repeated sounds.
                gpstarWand.setBarrelState(BARREL_UNKNOWN);

                stopEffect(S_VOICE_BARREL_SWITCH_DEFAULT);
                stopEffect(S_VOICE_BARREL_SWITCH_INVERTED);
                stopEffect(S_VOICE_BARREL_SWITCH_DISABLED);

                playEffect(S_VOICE_BARREL_SWITCH_INVERTED);

                packSerialSend(A_SET_BARREL_SWITCH, 2);
              }
              else if(BARREL_SWITCH_POLARITY == SWITCH_INVERTED) {
                // Change barrel safety switch polarity to disabled.
                BARREL_SWITCH_POLARITY = SWITCH_DISABLED;

                // Reset the barrel state to prevent repeated sounds.
                gpstarWand.setBarrelState(BARREL_UNKNOWN);

                stopEffect(S_VOICE_BARREL_SWITCH_DEFAULT);
                stopEffect(S_VOICE_BARREL_SWITCH_INVERTED);
                stopEffect(S_VOICE_BARREL_SWITCH_DISABLED);

                playEffect(S_VOICE_BARREL_SWITCH_DISABLED);

                packSerialSend(A_SET_BARREL_SWITCH, 3);
              }
              else {
                // Change barrel safety switch polarity to default.
                BARREL_SWITCH_POLARITY = SWITCH_DEFAULT;

                // Reset the barrel state to prevent repeated sounds.
                gpstarWand.setBarrelState(BARREL_UNKNOWN);

                stopEffect(S_VOICE_BARREL_SWITCH_DEFAULT);
                stopEffect(S_VOICE_BARREL_SWITCH_INVERTED);
                stopEffect(S_VOICE_BARREL_SWITCH_DISABLED);

                playEffect(S_VOICE_BARREL_SWITCH_DEFAULT);

                packSerialSend(A_SET_BARREL_SWITCH, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              switch(BARGRAPH_EEPROM_FIRING_ANIMATION) {
                case BARGRAPH_EEPROM_ORIGINAL:
                  BARGRAPH_EEPROM_FIRING_ANIMATION = BARGRAPH_EEPROM_ANIMATION_SUPER_HERO;

                  stopEffect(S_VOICE_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH);
                  stopEffect(S_VOICE_DEFAULT_FIRING_ANIMATIONS_BARGRAPH);
                  stopEffect(S_VOICE_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH);
                  playEffect(S_VOICE_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH);

                  packSerialSend(A_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH);
                break;

                case BARGRAPH_EEPROM_SUPER_HERO:
                  BARGRAPH_EEPROM_FIRING_ANIMATION = BARGRAPH_EEPROM_ANIMATION_DEFAULT;

                  stopEffect(S_VOICE_DEFAULT_FIRING_ANIMATIONS_BARGRAPH);
                  stopEffect(S_VOICE_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH);
                  stopEffect(S_VOICE_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH);
                  playEffect(S_VOICE_DEFAULT_FIRING_ANIMATIONS_BARGRAPH);

                  packSerialSend(A_DEFAULT_FIRING_ANIMATIONS_BARGRAPH);
                break;

                case BARGRAPH_EEPROM_DEFAULT:
                default:
                  BARGRAPH_EEPROM_FIRING_ANIMATION = BARGRAPH_EEPROM_ANIMATION_ORIGINAL;

                  stopEffect(S_VOICE_DEFAULT_FIRING_ANIMATIONS_BARGRAPH);
                  stopEffect(S_VOICE_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH);
                  stopEffect(S_VOICE_SUPER_HERO_FIRING_ANIMATIONS_BARGRAPH);
                  playEffect(S_VOICE_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH);

                  packSerialSend(A_MODE_ORIGINAL_FIRING_ANIMATIONS_BARGRAPH);
                break;
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Handled in checkRotaryEncoder()
              // The time it takes to overheat in power level 3.
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_1);

              playEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_3);

              packSerialSend(A_SOUND_OVERHEAT_START_TIMER, 3);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              smokeConfig.overheatContinuous3 = !smokeConfig.overheatContinuous3;
              packSerialSend(A_SET_CONTINUOUS_SMOKE_3, smokeConfig.overheatContinuous3 ? 1 : 0);
            }
          }
        break;

        // Menu Level 1: Intensify: Change the Cyclotron direction.
        // Menu Level 1: Barrel Wing Button: Enable the simulation of a ring for the Cyclotron lid.
        // Menu Level 2: Intensify: Overheat strobe.
        // Menu Level 2: Barrel Wing Button: Overheat lights off.
        // Menu Level 3: Intensify: Startup (Demo) Light Mode Enabled
        // Menu Level 3: Barrel Wing Button: Toggle whether wand bootup is short or full (Afterlife/Frozen Empire).
        // Menu Level 4: Intensify + top dial: Adjust overheat smoke duration by 1 second : Power Level 2
        // Menu Level 4: Barrel Wing Button + top dial: Adjust overheat start timer by 1 second : Power Level 2
        // Menu Level 5: Intensify: Enable/Disable overheat in power level #2
        // Menu Level 5: Barrel Wing Button: Enable/Disable sustained smoke in power level #2
        case 2:
          if(switch_intensify.pushed()) {
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              // Tell the Proton Pack to change the Cyclotron rotation direction.
              packSerialSend(A_CYCLOTRON_DIRECTION_TOGGLE);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              // Sub menu.
              packSerialSend(A_SET_OVERHEAT_STROBE, 2); // 2 = toggle command
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              packSerialSend(A_SET_DEMO_LIGHT_MODE, 2); // 2 = toggle command
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Overheat smoke duration level 2.
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_1);
              playEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_2);

              // Handled in checkRotaryEncoder()
              packSerialSend(A_SOUND_OVERHEAT_SMOKE_DURATION, 2);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              if(b_overheat_level_2) {
                b_overheat_level_2 = false;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_2_DISABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_2_ENABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_2_DISABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_2, 0);
              }
              else {
                b_overheat_level_2 = true;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_2_ENABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_2_DISABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_2_ENABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_2, 1);
              }
            }
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              packSerialSend(A_SET_CYCLOTRON_SIMULATE_RING, 2); // 2 = toggle command
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              // Sub menu.
              packSerialSend(A_SET_OVERHEAT_LIGHTS_OFF, 2); // 2 = toggle command
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              // Toggle whether the wand bootup does the full pack startup or not in AL/FE.
              packSerialSend(A_SET_QUICK_BOOTUP, 2); // 2 = toggle command
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Handled in checkRotaryEncoder()
              // The time it takes to overheat in power level 2.
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_1);

              playEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_2);

              packSerialSend(A_SOUND_OVERHEAT_START_TIMER, 2);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              smokeConfig.overheatContinuous2 = !smokeConfig.overheatContinuous2;
              packSerialSend(A_SET_CONTINUOUS_SMOKE_2, smokeConfig.overheatContinuous2 ? 1 : 0);
            }
          }
        break;

        // Menu Level 1: Intensify: Enable or disable extra Neutrona Wand Sounds.
        // Menu Level 1: Barrel Wing Button: Enable or disable Proton Stream Impact Effects.
        // Menu Level 2: Intensify: 1984 / 1989 / Afterlife / Frozen Empire / Default (Proton Pack toggle switch) year mode selection.
        // Menu Level 2: Barrel Wing Button: Overheat sync to fan.
        // Menu Level 3: Intensify: Toggle between Super Hero and Original Mode.
        // Menu Level 3: Barrel Wing Button: Toggle CTS between: 1984 / Afterlife / Default (Based on the year you are in)
        // Menu Level 4: Intensify + top dial: Adjust overheat smoke duration by 1 second : Power Level 1
        // Menu Level 4: Barrel Wing Button + top dial: Adjust overheat start timer by 1 second : Power Level 1
        // Menu Level 5: Intensify: Enable/Disable overheat in power level #1
        // Menu Level 5: Barrel Wing Button: Enable/Disable sustained smoke in power level #1
        case 1:
          if(switch_intensify.pushed()) {
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              if(b_extra_pack_sounds) {
                b_extra_pack_sounds = false;

                playEffect(S_VOICE_NEUTRONA_WAND_SOUNDS_DISABLED);

                packSerialSend(A_SET_VOICE_NEUTRONA_WAND_SOUNDS, 0);
              }
              else {
                b_extra_pack_sounds = true;

                playEffect(S_VOICE_NEUTRONA_WAND_SOUNDS_ENABLED);

                packSerialSend(A_SET_VOICE_NEUTRONA_WAND_SOUNDS, 1);
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              // Sub menu.
              packSerialSend(A_YEAR_MODES_CYCLE_EEPROM);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              // Toggle between Super Hero and Mode Original.
              packSerialSend(A_MODE_TOGGLE);

              // If there is no Pack, we need to cycle modes manually.
              if(b_wand_standalone) {
                if(gpstarWand.getSystemMode() == MODE_SUPER_HERO) {
                  gpstarWand.setSystemMode(MODE_ORIGINAL);

                  if(gpstarWand.getPowerLevel() != MIN_POWER_LEVEL) {
                    // If not already in PL1, set to PL1 as this is idle in Mode Original.
                    gpstarWand.setPowerLevel(LEVEL_1);
                  }

                  stopEffect(S_VOICE_MODE_ORIGINAL);
                  stopEffect(S_VOICE_MODE_SUPER_HERO);
                  playEffect(S_VOICE_MODE_ORIGINAL);
                }
                else {
                  gpstarWand.setSystemMode(MODE_SUPER_HERO);
                  gpstarWand.setPowerLevel(LEVEL_5); // Restore PL5 as the default power level.

                  stopEffect(S_VOICE_MODE_SUPER_HERO);
                  stopEffect(S_VOICE_MODE_ORIGINAL);
                  playEffect(S_VOICE_MODE_SUPER_HERO);
                }

                vgModeCheck();
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Overheat smoke duration level 1.
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_1);
              playEffect(S_VOICE_OVERHEAT_SMOKE_DURATION_LEVEL_1);

              // Handled in checkRotaryEncoder()
              packSerialSend(A_SOUND_OVERHEAT_SMOKE_DURATION, 1);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              if(b_overheat_level_1) {
                b_overheat_level_1 = false;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_1_DISABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_1_ENABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_1_DISABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_1, 0);
              }
              else {
                b_overheat_level_1 = true;

                stopEffect(S_VOICE_OVERHEAT_LEVEL_1_ENABLED);
                stopEffect(S_VOICE_OVERHEAT_LEVEL_1_DISABLED);
                playEffect(S_VOICE_OVERHEAT_LEVEL_1_ENABLED);

                packSerialSend(A_SET_OVERHEAT_LEVEL_1, 1);
              }
            }
            if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
              b_stream_effects = !b_stream_effects;

              stopEffect(S_VOICE_PROTON_MIX_EFFECTS_ENABLED);
              stopEffect(S_VOICE_PROTON_MIX_EFFECTS_DISABLED);
              b_stream_effects ? playEffect(S_VOICE_PROTON_MIX_EFFECTS_ENABLED) : playEffect(S_VOICE_PROTON_MIX_EFFECTS_DISABLED);

              // Tell the Proton Pack to toggle the Proton Stream impact effects.
              packSerialSend(A_SET_PROTON_STREAM_IMPACT, b_stream_effects ? 2 : 1);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
              packSerialSend(A_SET_OVERHEAT_SYNC_FAN, 2); // 2 = toggle command
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
              switch(WAND_YEAR_CTS) {
                case CTS_1984:
                  WAND_YEAR_CTS = CTS_AFTERLIFE;

                  stopEffect(S_VOICE_CTS_1984);
                  stopEffect(S_VOICE_CTS_AFTERLIFE);
                  stopEffect(S_VOICE_CTS_DEFAULT);

                  playEffect(S_VOICE_CTS_AFTERLIFE);

                  packSerialSend(A_CTS_AFTERLIFE);
                break;

                case CTS_AFTERLIFE:
                  WAND_YEAR_CTS = CTS_DEFAULT;

                  stopEffect(S_VOICE_CTS_1984);
                  stopEffect(S_VOICE_CTS_AFTERLIFE);
                  stopEffect(S_VOICE_CTS_DEFAULT);

                  playEffect(S_VOICE_CTS_DEFAULT);

                  packSerialSend(A_CTS_DEFAULT);
                break;

                case CTS_DEFAULT:
                default:
                  WAND_YEAR_CTS = CTS_1984;

                  stopEffect(S_VOICE_CTS_1984);
                  stopEffect(S_VOICE_CTS_AFTERLIFE);
                  stopEffect(S_VOICE_CTS_DEFAULT);

                  playEffect(S_VOICE_CTS_1984);

                  packSerialSend(A_CTS_1984);
                break;
              }
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_4) {
              // Handled in checkRotaryEncoder()
              // The time it takes to overheat in power level 1.
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_5);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_4);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_3);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_2);
              stopEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_1);

              playEffect(S_VOICE_OVERHEAT_START_TIMER_LEVEL_1);

              packSerialSend(A_SOUND_OVERHEAT_START_TIMER, 1);
            }
            else if(WAND_MENU_LEVEL == MENU_LEVEL_5) {
              smokeConfig.overheatContinuous1 = !smokeConfig.overheatContinuous1;
              packSerialSend(A_SET_CONTINUOUS_SMOKE_1, smokeConfig.overheatContinuous1 ? 1 : 0);
            }
          }
        break;
      }
    break;

    case ACTION_SETTINGS:
      settingsBlinkingLights();

      switch(i_wand_menu) {
        // Menu Level 1: (Intensify) -> Music track loop setting.
        // Menu Level 1: (Barrel Wing Button) -> Exit menu. <--handled by altWingButtonCheck() if wand is on, or mainLoop() if wand is off
        // Menu Level 2: (Intensify) -> Enable or disable crossing the streams / video game modes.
        // Menu Level 2: (Barrel Wing Button) -> Enable/Disable Video Game Colour Modes for the Proton Pack LEDs (when video game mode is selected).
        // Menu Level 3: (Intensify) -> GPStar II: Toggle the Neutrona Wand WiFi.
        // Menu Level 3: (Barrel Wing Button) -> GPStar II: Toggle the Proton Pack WiFi.
        case 5:
          // Music track loop setting.
          if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
            if(switch_intensify.pushed()) {
              // Tell pack to loop the music track.
              packSerialSend(A_MUSIC_TRACK_LOOP_TOGGLE);

              // Standalone Neutrona Wand has to change this setting on its own.
              if(b_wand_standalone) {
                toggleMusicLoop(!b_repeat_track);
              }
            }
          }
          else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
            if(switch_intensify.pushed()) {
              toggleWandModes();
            }
            else if(switch_mode.pushed()) {
              if(gpstarWand.isFiringModeVG()) {
                // Tell the Proton Pack to cycle through the Video Game Colour toggles.
                packSerialSend(A_VIDEO_GAME_MODE_COLOUR_TOGGLE);
              }
            }
          }
          #ifdef ESP32
          else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
            if(switch_intensify.pushed()) {
              // Toggle the Neutrona Wand WiFi.
              if(WIFI_USER_MODE == WIFI_ENABLED) {
                WIFI_USER_MODE = WIFI_DISABLED;
                stopEffect(S_VOICE_WAND_WIFI_DISABLED);
                stopEffect(S_VOICE_WAND_WIFI_ENABLED);
                playEffect(S_VOICE_WAND_WIFI_DISABLED);
                packSerialSend(A_SET_WAND_WIFI, 0);
              }
              else {
                WIFI_USER_MODE = WIFI_ENABLED;
                stopEffect(S_VOICE_WAND_WIFI_DISABLED);
                stopEffect(S_VOICE_WAND_WIFI_ENABLED);
                playEffect(S_VOICE_WAND_WIFI_ENABLED);
                packSerialSend(A_SET_WAND_WIFI, 1);
              }
            }
            else if(switch_mode.pushed()) {
              // Toggle the Proton Pack WiFi (just send the command, let the pack sort it out).
              packSerialSend(A_TOGGLE_PACK_WIFI);
            }
          }
          #endif
        break;

        // Menu Level 1: (Intensify + Top dial) -> Adjust the LED dimming of the Power Cell, Cyclotron and Inner Cyclotron.
        // Menu Level 1: (Barrel Wing Button) -> Cycle through which dimming mode to adjust in the Proton Pack. Power Cell, Cyclotron, Inner Cyclotron.
        // Menu Level 2: (Intensify) -> Enable or disable overheating.
        // Menu Level 2: (Barrel Wing Button) -> Enable or disable smoke for the Proton Pack.
        // Menu Level 3: (Intensify) -> GPStar II: -AVAILABLE-
        // Menu Level 3: (Barrel Wing Button) -> GPStar II: -AVAILABLE-
        case 4:
          // Adjust the Proton Pack / Neutrona Wand sound effects volume.
          if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
            // Cycle through the dimming modes in the Proton Pack. (Power Cell, Cyclotron and Inner Cyclotron). Actual control of the dimming is handled in checkRotaryEncoder().
            if(switch_mode.pushed()) {
              // Tell the Proton Pack to change to the next dimming mode.
              packSerialSend(A_DIMMING_TOGGLE);
            }
          }
          else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
            // Enable or disable overheating.
            if(switch_intensify.pushed()) {
              toggleOverheating();
            }
            else if(switch_mode.pushed()) {
              // Tell the Proton Pack to toggle the smoke on or off.
              packSerialSend(A_SMOKE_TOGGLE);
            }
          }
        break;

        // Menu Level 1: (Intensify + Top dial) -> Adjust Proton Pack / Neutrona Wand sound effects.
        // Menu Level 1: (Barrel Wing Button + top dial) Adjust Proton Pack / Neutrona Wand music volume.
        // Menu Level 2: (Intensify) -> Toggle Cyclotron rotation direction.
        // Menu Level 2: (Barrel Wing Button) -> Toggle the Proton Pack Single LED or 3 LEDs for 1984/1989 modes.
        // Menu Level 3: (Intensify) -> GPStar II: -AVAILABLE-
        // Menu Level 3: (Barrel Wing Button) -> GPStar II: -AVAILABLE-
        case 3:
          // Top menu code is handled in checkRotaryEncoder()
          // Sub menu. Adjust Cyclotron settings.
          if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
            if(switch_intensify.pushed()) {
              // Tell the Proton Pack to change the Cyclotron rotation direction.
              packSerialSend(A_CYCLOTRON_DIRECTION_TOGGLE);
            }
            else if(switch_mode.pushed()) {
              // Tell the Proton Pack to toggle the Single LED or 3 LEDs for 1984/1989 modes.
              packSerialSend(A_CYCLOTRON_LED_TOGGLE);
            }
          }
        break;

        // Menu Level 1: (Intensify) -> Go to next music track.
        // Menu Level 1: (Barrel Wing Button) -> Go to previous music track.
        // Menu Level 2: (Intensify) -> Enable pack vibration, enable pack vibration while firing only, disable pack vibration. *Note that the pack vibration switch will toggle both pack and wand vibration on or off*
        // Menu Level 2: (Barrel Wing Button) -> Enable wand vibration, enable wand vibration while firing only, disable wand vibration.
        // Menu Level 3: (Intensify) -> GPStar II: -AVAILABLE-
        // Menu Level 3: (Barrel Wing Button) -> GPStar II: -AVAILABLE-
        case 2:
          // Change music tracks.
          if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
            if(switch_intensify.pushed()) {
              if(b_wand_standalone) {
                musicNextTrack();
              }
              else {
                // Tell the pack to play the next track.
                packSerialSend(A_MUSIC_NEXT_TRACK);
              }
            }
            else if(switch_mode.pushed()) {
              if(b_wand_standalone) {
                musicPrevTrack();
              }
              else {
                // Tell the pack to play the previous track.
                packSerialSend(A_MUSIC_PREV_TRACK);
              }
            }
          }
          else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
            if(switch_intensify.pushed()) {
              // Enable or disable vibration for the pack or during firing only.
              packSerialSend(A_SET_VIBRATION_MODE, 5); // 5 = CYCLE_TOGGLE
            }
            else if(switch_mode.pushed()) {
              // Enable or disable vibration or firing vibration only for the wand.
              stopEffect(S_BEEPS_ALT);
              playEffect(S_BEEPS_ALT);

              switch(gpstarWand.getVibrationMode()) {
                case VIBRATION_ALWAYS:
                  gpstarWand.setVibrationMode(VIBRATION_FIRING_ONLY);
                  b_vibration_switch_on = true; // Override the Proton Pack vibration toggle switch.

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);

                  packSerialSend(A_SET_VIBRATION_MODE, 2); // 2 = FIRING_ONLY

                  ms_menu_vibration.start(250); // Confirmation buzz for 250ms.
                break;
                case VIBRATION_FIRING_ONLY:
                default:
                  gpstarWand.setVibrationMode(VIBRATION_NEVER);

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);

                  packSerialSend(A_SET_VIBRATION_MODE, 3); // 3 = NEVER
                break;
                case VIBRATION_NEVER:
                  gpstarWand.setVibrationMode(VIBRATION_ALWAYS);
                  b_vibration_switch_on = true; // Override the Proton Pack vibration toggle switch.

                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_FIRING_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);
                  stopEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_DISABLED);

                  playEffect(S_VOICE_NEUTRONA_WAND_VIBRATION_ENABLED);

                  packSerialSend(A_SET_VIBRATION_MODE, 1); // 1 = ALWAYS

                  ms_menu_vibration.start(250); // Confirmation buzz for 250ms.
                break;
              }
            }
          }
        break;

        // Menu Level 1: (Intensify) -> Play music or stop music.
        // Menu Level 1: (Barrel Wing Button) -> Mute the Proton Pack and Neutrona Wand.
        // Menu Level 2: (Intensify) -> Switch between 1984/1989/Afterlife/Frozen Empire mode.
        // Menu Level 2: (Barrel Wing Button) -> Shuffle music tracks setting.
        // Menu Level 3: (Intensify) -> GPStar II: Reset the wand WiFi password to default.
        // Menu Level 3: (Barrel Wing Button) -> GPStar II: Reset the pack WiFi password to default.
        case 1:
          if(WAND_MENU_LEVEL == MENU_LEVEL_1) {
            if(switch_intensify.pushed()) {
              // Tell the pack to start or stop its music.
              packSerialSend(A_MUSIC_TOGGLE);

              // Handle standalone wand music playback.
              if(b_wand_standalone) {
                if(b_playing_music) {
                  stopMusic();
                }
                else {
                  playMusic();
                }
              }
            }
            else if(switch_mode.pushed()) {
              // Silence the Proton Pack and Neutrona Wand or revert back to previously-selected volume.
              packSerialSend(A_TOGGLE_MUTE);

              if(b_wand_standalone) {
                if(i_volume_master == i_volume_abs_min) {
                  i_volume_master = i_volume_revert;
                }
                else {
                  i_volume_revert = i_volume_master;

                  // Set the master volume to silent.
                  i_volume_master = i_volume_abs_min;
                }

                updateMasterVolume();
              }
            }
          }
          else if(WAND_MENU_LEVEL == MENU_LEVEL_2) {
            // Switch between 1984/1989/Afterlife/Frozen Empire mode.
            if(switch_intensify.pushed()) {
              // Tell the Proton Pack to cycle through year modes.
              packSerialSend(A_YEAR_MODES_CYCLE);

              // There is no pack connected; let's change the years.
              if(b_wand_standalone) {
                stopEffect(S_BEEPS_BARGRAPH);
                playEffect(S_BEEPS_BARGRAPH);

                switch(getNeutronaWandYearMode()) {
                  case SYSTEM_1984:
                    // 1984 -> 1989
                    WAND_YEAR_MODE = YEAR_1989;

                    stopEffect(S_VOICE_FROZEN_EMPIRE);
                    stopEffect(S_VOICE_AFTERLIFE);
                    stopEffect(S_VOICE_1989);
                    stopEffect(S_VOICE_1984);

                    playEffect(S_VOICE_1989);
                  break;

                  case SYSTEM_1989:
                    // 1989 -> Afterlife
                    WAND_YEAR_MODE = YEAR_AFTERLIFE;

                    stopEffect(S_VOICE_FROZEN_EMPIRE);
                    stopEffect(S_VOICE_AFTERLIFE);
                    stopEffect(S_VOICE_1989);
                    stopEffect(S_VOICE_1984);

                    playEffect(S_VOICE_AFTERLIFE);
                  break;

                  case SYSTEM_AFTERLIFE:
                  default:
                    // Afterlife -> Frozen Empire
                    WAND_YEAR_MODE = YEAR_FROZEN_EMPIRE;

                    stopEffect(S_VOICE_FROZEN_EMPIRE);
                    stopEffect(S_VOICE_AFTERLIFE);
                    stopEffect(S_VOICE_1989);
                    stopEffect(S_VOICE_1984);

                    playEffect(S_VOICE_FROZEN_EMPIRE);
                  break;

                  case SYSTEM_FROZEN_EMPIRE:
                    // Frozen Empire -> 1984
                    WAND_YEAR_MODE = YEAR_1984;

                    stopEffect(S_VOICE_FROZEN_EMPIRE);
                    stopEffect(S_VOICE_AFTERLIFE);
                    stopEffect(S_VOICE_1989);
                    stopEffect(S_VOICE_1984);

                    playEffect(S_VOICE_1984);
                  break;
                }
              }
            }
            else if(switch_mode.pushed()) {
              // Tell pack to shuffle the music tracks.
              packSerialSend(A_MUSIC_TRACK_SHUFFLE_TOGGLE);

              // Standalone Neutrona Wand has to change this setting on its own.
              if(b_wand_standalone) {
                toggleMusicShuffle(!b_shuffle_tracks);
              }
            }
          }
          #ifdef ESP32
          else if(WAND_MENU_LEVEL == MENU_LEVEL_3) {
            if(switch_intensify.longPress()) {
              // Reset the WiFi password to default.
              wirelessMgr->resetWifiPassword();

              // Turn off the WiFi until the user decides to manually enable and reconnect.
              WIFI_USER_MODE = WIFI_DISABLED;

              // Give some audio feedback as to what just happened.
              packSerialSend(A_WAND_WIFI_RESET);
              stopEffect(S_VOICE_PACK_WIFI_RESET);
              stopEffect(S_VOICE_WAND_WIFI_RESET);
              playEffect(S_VOICE_WAND_WIFI_RESET);
            }
            else if(switch_mode.longPress()) {
              // Tell the Proton Pack to reset its WiFi password.
              packSerialSend(A_RESET_WIFI_PASSWORD);
            }
          }
          #endif
        break;
      }
    break;
  }
}
