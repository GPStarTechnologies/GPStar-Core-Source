/**
 *   LightingPalettes - Global palette definitions for GPStar LED effects.
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

#include "Lighting.h"

/**
 * PALETTE CREATION FUNCTIONS
 * 
 * These functions build LED_RGB_Palette16 palettes using consistent
 * color definitions from the Lighting library. Each palette maps to
 * a thematic set of colors for different stream modes.
 * 
 * All colors are derived from ColorID enum values, ensuring consistency
 * across the application. ColorIDs will be mapped to HSV triplets.
 */

inline LED_Palette16 getPaletteProton() {
  return {{
    C_RED2, C_RED2, C_RED2, C_NAVY_BLUE, C_RED5, C_RED5, C_RED5, C_BLACK,
    C_NAVY_BLUE, C_RED2, C_RED2, C_RED4, C_RED4, C_RED5, C_RED5, C_BLACK
  }};
}

inline LED_Palette16 getPaletteSlime() {
  return {{
    C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_CHARTREUSE, C_CHARTREUSE, C_DARK_GREEN, C_BLACK,
    C_GREEN, C_GREEN, C_GREEN, C_GREEN, C_CHARTREUSE, C_CHARTREUSE, C_DARK_GREEN, C_BLACK
  }};
}

inline LED_Palette16 getPaletteStasis() {
  return {{
    C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_MID_BLUE, C_NAVY_BLUE, C_NAVY_BLUE, C_BLACK,
    C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_MID_BLUE, C_NAVY_BLUE, C_NAVY_BLUE, C_BLACK
  }};
}

inline LED_Palette16 getPaletteMeson() {
  return {{
    C_YELLOW, C_YELLOW, C_YELLOW, C_YELLOW, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK,
    C_YELLOW, C_YELLOW, C_YELLOW, C_YELLOW, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK
  }};
}

inline LED_Palette16 getPaletteSpectral() {
  return {{
    C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_BLUE, C_NAVY_BLUE, C_PURPLE, C_BLACK,
    C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_BLUE, C_NAVY_BLUE, C_PURPLE, C_BLACK
  }};
}

inline LED_Palette16 getPaletteHalloween() {
  return {{
    C_ORANGE, C_ORANGE, C_ORANGE, C_ORANGE, C_ORANGE, C_ORANGE, C_BLACK, C_BLACK,
    C_PURPLE, C_PURPLE, C_PURPLE, C_PURPLE, C_PURPLE, C_PURPLE, C_BLACK, C_BLACK
  }};
}

inline LED_Palette16 getPaletteChristmas() {
  return {{
    C_RED, C_RED, C_RED, C_RED, C_RED, C_RED, C_BLACK, C_BLACK,
    C_DARK_GREEN, C_DARK_GREEN, C_DARK_GREEN, C_DARK_GREEN, C_DARK_GREEN, C_DARK_GREEN, C_BLACK, C_BLACK
  }};
}

inline LED_Palette16 getPaletteBrass() {
  return {{
    C_DARK_GREEN, C_CHARTREUSE, C_CHARTREUSE, C_CHARTREUSE, C_ORANGE, C_ORANGE, C_ORANGE, C_BLACK,
    C_DARK_GREEN, C_CHARTREUSE, C_CHARTREUSE, C_CHARTREUSE, C_ORANGE, C_ORANGE, C_ORANGE, C_BLACK
  }};
}

inline LED_Palette16 getPaletteWhite() {
  return {{
    C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_BLACK, C_BLACK, C_BLACK, C_BLACK,
    C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_BLACK, C_BLACK, C_BLACK, C_BLACK
  }};
}
