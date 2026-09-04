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

// This board emulates EEPROM by writing into flash itself. Left to
// auto-detect, the wear-leveling driver's backward-from-end scan lands its
// reserved region at 0x08016000 (90112) -- NOT near the true end of this
// chip's flash -- silently overlapping application code/data once the
// firmware grows past that point. Every EEPROM write (which happens
// automatically at boot) then corrupts the overlapping code: the board works
// right after flashing and fails on the next boot.
//
// 256KB total flash and 2KB sectors are the actual, ROM-bootloader-reported
// geometry, not assumed from the "STM32L432xC" part number: `dfu-util -l`
// while the board is in its stm32-dfu bootloader (a factory-programmed ROM,
// entirely separate from and unaffected by this firmware) reports
// "Internal Flash /0x08000000/0128*0002Kg" -- 128 sectors of 2KB. Pinning
// this removes the wear_leveling_efl.c `chSysHalt` guard that would
// otherwise catch a wrong flash size loudly, so if that geometry is ever in
// doubt, re-verify via `dfu-util -l` before trusting these numbers, and
// confirm a changed, persisted setting (e.g. RM_NEXT) survives a couple of
// power cycles -- the 0x0803E800 target was confirmed empty (all 0xFF) via
// `dfu-util -U` before this was pinned, but an actual write there hasn't
// independently been verified since.
#define K8_PRO_FLASH_TOTAL_SIZE (256 * 1024)
#define K8_PRO_FLASH_SECTOR_SIZE (2 * 1024)
#define K8_PRO_FLASH_TOTAL_SECTORS (K8_PRO_FLASH_TOTAL_SIZE / K8_PRO_FLASH_SECTOR_SIZE)
#if (WEAR_LEVELING_BACKING_SIZE % K8_PRO_FLASH_SECTOR_SIZE) != 0
#    error WEAR_LEVELING_BACKING_SIZE must be a multiple of the flash sector size, or reservation would under-count sectors.
#endif
// WEAR_LEVELING_BACKING_SIZE comes from "eeprom.wear_leveling.backing_size"
// in keyboards/keychron/k8_pro/info.json (via the generated info_config.h).
#define K8_PRO_EEPROM_RESERVED_SECTORS (WEAR_LEVELING_BACKING_SIZE / K8_PRO_FLASH_SECTOR_SIZE)
#define WEAR_LEVELING_EFL_FLASH_SIZE K8_PRO_FLASH_TOTAL_SIZE
#define WEAR_LEVELING_EFL_FIRST_SECTOR (K8_PRO_FLASH_TOTAL_SECTORS - K8_PRO_EEPROM_RESERVED_SECTORS)

// Tap both shifts together to toggle Caps Word, no dedicated key needed.
// Safe here because this board doesn't enable COMMAND_ENABLE (same chord).
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

// Boot straight into NKRO instead of 6KRO. Without this, NKRO_ENABLE only
// makes NKRO available; USB still boots 6KRO until toggled at runtime, which
// is easy to forget and then silently drop a simultaneous direction+attack
// press mid-combo in a fighting game. FORCE_NKRO (the older equivalent) is
// deprecated as of this QMK version -- it also re-forces NKRO on every boot
// even after an explicit NK_OFF, whereas this only sets the EEPROM default.
#define NKRO_DEFAULT_ON true

// VIA defaults this to 4. Needs to cover the highest layer index actually
// used (WIN_FN = 5, see the `enum layers` comment in keymap.c for why the
// numbering has a gap), not just the count of real layers.
#define DYNAMIC_KEYMAP_LAYER_COUNT 6

// PaletteFx: all effects and palettes, cycled with RM_NEXT/RM_PREV and
// RM_HUEU/RM_HUED respectively.
#define PALETTEFX_ENABLE_ALL_EFFECTS
#define PALETTEFX_ENABLE_ALL_PALETTES

// Boot into our own PaletteFx Reactive variant (rgb_matrix_user.inc; same as
// upstream Reactive but fully off at idle instead of a ~25% background glow)
// with the "Notpink" palette, instead of OpenRGB direct mode. Only takes
// effect on a fresh EEPROM (first flash, or after an eeconfig reset) --
// RM_NEXT/RM_HUEU etc. override and persist from then on.
#define RGB_MATRIX_HUE_STEP 8
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CUSTOM_PALETTEFX_REACTIVE_DARK
// PALETTEFX_NOTPINK (palettefx.h) rather than a hardcoded index, so this
// can't silently drift if upstream reorders/adds palettes.
#define RGB_MATRIX_DEFAULT_HUE (RGB_MATRIX_HUE_STEP * PALETTEFX_NOTPINK)
