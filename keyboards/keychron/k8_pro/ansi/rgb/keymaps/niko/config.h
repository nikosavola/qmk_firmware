/* Copyright 2026 Niko Savola
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// Tap both shifts together to toggle Caps Word, no dedicated key needed.
// Safe here because this board doesn't enable COMMAND_ENABLE (same chord).
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

// PaletteFx: only the keypress-reactive effect (the other 5 are more of a
// static/ambient look), but keep every palette since RM_HUEU/RM_HUED cycles
// through them and there's no clear "best" one to pick in advance.
#define PALETTEFX_REACTIVE_ENABLE
#define PALETTEFX_ENABLE_ALL_PALETTES

// Boot into PaletteFx Reactive with the "Notpink" palette (8th palette,
// 0-indexed, in the alphabetical order palettefx.c defines them in) instead
// of OpenRGB direct mode. Only takes effect on a fresh EEPROM (first flash,
// or after an eeconfig reset) -- RM_NEXT/RM_HUEU etc. override and persist
// from then on.
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_COMMUNITY_MODULE_PALETTEFX_REACTIVE
#define RGB_MATRIX_DEFAULT_HUE 56
