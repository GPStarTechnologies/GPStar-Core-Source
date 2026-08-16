/**
 *   GPStar Stream Effects - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2024-2026 Dustin Grau <dustin.grau@gmail.com>
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

void sendDebug(const String& message);

// Clear any prior information from the WebSocket client.
void resetWebSocketData() {
  wsData.mode = "";
  wsData.theme = "";
  wsData.switchState = "";
  wsData.pack = "";
  wsData.safety = "";
  wsData.wandPower = 5; // Default to max power.
  wsData.wandMode = "";
  wsData.firing = "";
  wsData.cable = "";
  wsData.cyclotron = "";
  wsData.temperature = "";
}

// Obtain a list of partitions for this device.
void printPartitions() {
  const esp_partition_t *partition;
  esp_partition_iterator_t iterator = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if(iterator == nullptr) {
    debugln(F("No partitions found."));
    return;
  }

  debugln(F("Partitions:"));
  while(iterator != nullptr) {
    partition = esp_partition_get(iterator);
    debugf("Label: %s, Size: %lu bytes, Address: 0x%08lx\n",
           partition->label,
           partition->size,
           partition->address);
    iterator = esp_partition_next(iterator);
  }

  esp_partition_iterator_release(iterator);  // Release the iterator once done
}

// Function to update the current palette based on stream mode.
void updateStreamPalette() {
  switch(gpstarSystem.getStreamMode()) {
    case PROTON:
      if(gpstarSystem.getSystemTheme() == SYSTEM_FROZEN_EMPIRE && !wsData.cyclotronLid && wsData.ctsActive) {
        cp_StreamPalette = getPaletteBrass();
      }
      else {
        cp_StreamPalette = getPaletteProton();
      }
    break;
    case SLIME:
      cp_StreamPalette = getPaletteSlime();
    break;
    case STASIS:
      cp_StreamPalette = getPaletteStasis();
    break;
    case MESON:
      cp_StreamPalette = getPaletteMeson();
    break;
    case SPECTRAL:
      cp_StreamPalette = getPaletteSpectral();
    break;
    case HOLIDAY_HALLOWEEN:
      cp_StreamPalette = getPaletteHalloween();
    break;
    case HOLIDAY_CHRISTMAS:
      cp_StreamPalette = getPaletteChristmas();
    break;
    case SELFTEST:
      // Initialize timer on first entry to self-test mode
      if(!ms_selftest_cycle.isRunning()) {
        ms_selftest_cycle.start(i_selftest_interval);
        i_selftest_palette = 0; // Reset to first palette
      }

      // Cycle through all available palettes every 2 seconds during self-test
      if(ms_selftest_cycle.justFinished()) {
        sendDebug(String(F("Self-Test: Switching to Palette #")) + String(i_selftest_palette) + String(F(" w/ Power Level ")) + String(wsData.wandPower));

        // Set current palette based on count of palettes available
        switch(i_selftest_palette % i_palette_count) {
          case 0: cp_StreamPalette = getPaletteWhite(); break;
          case 1: cp_StreamPalette = getPaletteProton(); break;
          case 2: cp_StreamPalette = getPaletteSlime(); break;
          case 3: cp_StreamPalette = getPaletteStasis(); break;
          case 4: cp_StreamPalette = getPaletteMeson(); break;
          case 5: cp_StreamPalette = getPaletteSpectral(); break;
          case 6: cp_StreamPalette = getPaletteHalloween(); break;
          case 7: cp_StreamPalette = getPaletteChristmas(); break;
          case 8: cp_StreamPalette = getPaletteBrass(); break;
        }

        // Advance to next palette for the next cycle
        i_selftest_palette = (i_selftest_palette + 1) % i_palette_count;

        // Restart timer for next cycle.
        ms_selftest_cycle.restart();
      }
    break;
    default:
      cp_StreamPalette = getPaletteWhite();
    break;
  }
}
