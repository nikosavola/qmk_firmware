/* Copyright 2021 ~ 2025 @ Keychron (https://www.keychron.com)
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

#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "backlit_indicator.h"
#include "xcase.h"
#include "motion_macros.h"

// Explicit values, NOT sequential auto-increment -- two constraints fight
// over the numbering and both are load-bearing:
//
// 1. keyboards/keychron/k8_pro/k8_pro.c's dip_switch_update_kb() hardcodes
//    `default_layer_set(1UL << (active ? 2 : 0))` for the physical Mac/Win
//    switch. That's keyboard-level code we can't override (not weak), so
//    MAC_BASE must be exactly 0 and WIN_BASE must be exactly 2, always.
//    Sequentially renumbering this enum (as an earlier version of this file
//    did, to fix constraint 2 below) silently broke constraint 1 instead:
//    flipping the dip switch to Windows started setting layer 2 as default,
//    which was GAME under that renumbering -- toggling GAME_TOG then just
//    flipped a layer_state bit that default_layer_state already forced on,
//    so the board could never actually leave GAME once the dip switch had
//    ever landed on Windows. Only caught after flashing to real hardware.
// 2. GAME must outrank whichever base layer is currently default (so its
//    overlay wins while toggled on), but must NOT outrank either FN layer
//    -- otherwise holding FN while GAME is on would let GAME's motion-macro
//    keycodes shadow FN's RGB/system shortcuts at the same physical
//    positions (Tab/F/R/C) instead of falling through to them.
//
// Index 1 is an intentionally unused gap (wastes one layer's worth of
// flash, a few hundred bytes -- negligible here) since no value satisfies
// both constraints without it: WIN_BASE is pinned to 2, so GAME must be >2,
// which leaves nothing between MAC_BASE(0) and WIN_BASE(2) for an FN layer
// to occupy without also being numerically below GAME.
enum layers {
    MAC_BASE = 0,
    WIN_BASE = 2,
    GAME     = 3, // Toggled overlay (FN+Enter).
    MAC_FN   = 4,
    WIN_FN   = 5,
};

enum custom_keycodes {
    SOCD_TOG_FB = QK_USER_0, // SOCD toggle with an RGB flash to show the new state.
    // xcase's own XCASE_SNAKE/XCASE_KEBAB hardcode KC_UNDS/KC_MINS as the
    // delimiter: the *physical* QWERTY minus-key position. Under the Dvorak
    // layout applied in software, that physical key types "["/"{", not
    // "-"/"_" -- Dvorak moved "-"/"_" to the physical apostrophe key instead.
    // These call enable_xcase_with() ourselves with KC_QUOT so the delimiter
    // Dvorak actually produces is right.
    XCASE_SNAKE_DVORAK = QK_USER_1,
    XCASE_KEBAB_DVORAK = QK_USER_2,
    GAME_TOG           = QK_USER_3, // Toggle the GAME layer, with an RGB flash.
    MACRO_FACE_TOG     = QK_USER_4, // Flip which side the motion macros treat as "forward".
    MACRO_HALF_CIRCLE  = QK_USER_5,
    MACRO_DP           = QK_USER_6,
    MACRO_360          = QK_USER_7,
};

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)
#define SWITCH_MODE 0x1688

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_tkl_ansi(
        KC_ESC,   KC_BRID,  KC_BRIU,  KC_MCTL,  KC_LPAD,  UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,             KC_SNAP,  KC_SIRI,  UG_NEXT,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,   KC_INS,   KC_HOME,  KC_PGUP,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,   KC_DEL,   KC_END,   KC_PGDN,
        KC_ESC,   KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,             KC_UP,
        KC_LCTL,  KC_LOPTN, KC_LCMMD,                               KC_SPC,                                 KC_RCMMD, KC_ROPTN, FN_MAC,   KC_RCTL,   KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT_tkl_ansi(
        _______,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,              _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,   _______,  _______,  _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  XCASE_SNAKE_DVORAK, XCASE_KEBAB_DVORAK, XCASE_CAMEL, _______,  TURBO,    AC_TOGG,  SENTENCE_CASE_TOGGLE,  DM_RSTP,   DM_REC1,  DM_REC2,  SOCD_TOG_FB,
        KC_CAPS,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  QK_REP,   XCASE_OFF, QK_AREP,  LEADER,   DM_PLY1,  DM_PLY2,            GAME_TOG,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,             _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,   _______,  _______,  _______),

    [WIN_BASE] = LAYOUT_tkl_ansi(
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,              KC_PSCR,  KC_CTANA, UG_NEXT,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,   KC_INS,   KC_HOME,  KC_PGUP,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,   KC_DEL,   KC_END,   KC_PGDN,
        KC_ESC,   KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,             KC_UP,
        KC_LCTL,  KC_LGUI,  KC_LALT,                                KC_SPC,                                 KC_RALT,  KC_RGUI,  FN_WIN,   KC_RCTL,   KC_LEFT,  KC_DOWN,  KC_RGHT),

    [WIN_FN] = LAYOUT_tkl_ansi(
        _______,  KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,             _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,   _______,  _______,  _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  XCASE_SNAKE_DVORAK, XCASE_KEBAB_DVORAK, XCASE_CAMEL, SWITCH_MODE, TURBO,  AC_TOGG,  SENTENCE_CASE_TOGGLE,  DM_RSTP,   DM_REC1,  DM_REC2,  SOCD_TOG_FB,
        KC_CAPS,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  QK_REP,   XCASE_OFF, QK_AREP,  LEADER,   DM_PLY1,  DM_PLY2,            GAME_TOG,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,             _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,   _______,  _______,  _______),

    // Motion-macro overlay (see motion_macros.h). Everything else stays
    // transparent -- Ins/Del/Home/End/PgUp/PgDn (attack buttons) and Ctrl
    // (grab) already send themselves on the base layer, no remap needed.
    // Enter/`/Backspace/LShift are earmarked for future macros, left
    // transparent (still type normally) until those are defined.
    [GAME] = LAYOUT_tkl_ansi(
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,             _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,   _______,  _______,  _______,
        MACRO_FACE_TOG, _______, _______, _______, MACRO_DP, _______, _______, _______, _______, _______, _______,  _______,  _______,  _______,   _______,  _______,  _______,
        _______,  _______,  _______,  _______,  MACRO_HALF_CIRCLE, _______, _______, _______, _______, _______, _______,  _______,            _______,
        _______,            _______,  _______,  MACRO_360, _______, _______, _______, _______, _______,  _______,  _______,            _______,             _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,   _______,  _______,  _______)
};
// clang-format on

// SOCD Cleaner: WASD opposing-direction filtering for gaming, enabled by
// default. Toggle globally with FN+PgDn.
socd_cleaner_t socd_opposing_pairs[] = {
    {{KC_W, KC_S}, SOCD_CLEANER_LAST},
    {{KC_A, KC_D}, SOCD_CLEANER_LAST},
};

extern uint8_t is_orgb_mode;

// Whatever RGB Matrix mode was active before GAME_TOG switched to
// GAME_MODE_HIGHLIGHT, so it can be restored on toggle-off instead of
// always dropping back to the compiled default.
static uint8_t game_saved_rgb_mode;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case SWITCH_MODE:
#ifdef OPENRGB_ENABLE
            if (record->event.pressed) {
                is_orgb_mode = !is_orgb_mode;
            }
#endif
            return false;
        case SOCD_TOG_FB:
            if (record->event.pressed) {
                socd_cleaner_enabled = !socd_cleaner_enabled;
                // Flash the board green when turning on, red when turning off,
                // since the module itself has no on-keyboard indicator.
                RGB color = socd_cleaner_enabled ? (RGB){0, 255, 0} : (RGB){255, 0, 0};
                backlight_indicator_start(250, 250, 3, color);
            }
            return false;
        case XCASE_SNAKE_DVORAK:
            if (record->event.pressed) {
                enable_xcase_with(LSFT(KC_QUOT));
                // enable_xcase_with() only exempts the exact delimiter
                // keycode (LSFT(KC_QUOT)) from ending xcase, but a real
                // keypress always arrives as the bare base keycode (shift is
                // a separate event) -- so manually typing a literal "_" or
                // "'" here would otherwise prematurely end snake_case mode.
                add_xcase_exclusion_keycode(KC_QUOT);
            }
            return false;
        case XCASE_KEBAB_DVORAK:
            if (record->event.pressed) {
                enable_xcase_with(KC_QUOT);
            }
            return false;
        case GAME_TOG:
            if (record->event.pressed) {
                layer_invert(GAME);
                if (layer_state_is(GAME)) {
                    game_saved_rgb_mode = rgb_matrix_get_mode();
                    rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_GAME_MODE_HIGHLIGHT);
                } else {
                    // _noeeprom on both sides: this is a temporary mode
                    // swap, not a real setting change -- shouldn't wear
                    // EEPROM, and shouldn't overwrite the mode you'd
                    // actually saved via RM_NEXT/VIA.
                    rgb_matrix_mode_noeeprom(game_saved_rgb_mode);
                }
                RGB color = layer_state_is(GAME) ? (RGB){0, 200, 255} : (RGB){128, 128, 128};
                backlight_indicator_start(250, 250, 3, color);
            }
            return false;
        case MACRO_FACE_TOG:
            if (record->event.pressed) {
                motion_toggle_facing();
                RGB color = motion_facing_right ? (RGB){255, 180, 0} : (RGB){180, 0, 255};
                backlight_indicator_start(250, 250, 2, color);
            }
            return false;
        case MACRO_HALF_CIRCLE:
            if (record->event.pressed) {
                motion_play(MOTION_HALF_CIRCLE, ARRAY_SIZE(MOTION_HALF_CIRCLE));
            }
            return false;
        case MACRO_DP:
            if (record->event.pressed) {
                motion_play(MOTION_DP, ARRAY_SIZE(MOTION_DP));
            }
            return false;
        case MACRO_360:
            if (record->event.pressed) {
                motion_play(MOTION_360, ARRAY_SIZE(MOTION_360));
            }
            return false;
    }

    return true;
}
