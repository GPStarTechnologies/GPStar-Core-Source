/**
 *   GPStar Toaster - Ghostbusters Proton Pack & Neutrona Wand.
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

/*
 * Micro SD Card sound files in order. If you have no sound, your SD card might be too slow, try a faster one.
 * File naming 000_ is important. For music, it is 500_ and higher.
 */

enum sound_fx {
  S_EMPTY,
  S_PING1,
  S_PING2,
  S_PING3,
  S_PING4
};

/*
 * Need to keep track which is the last sound effect, so we can iterate over the effects to adjust the volume gain on them.
 */
const uint16_t i_last_effects_track = S_PING4;
