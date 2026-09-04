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

// Offline-only fighting-game motion macros (half-circle, DP/"Z", 360), gated
// behind the GAME layer toggle. Offline only: most games' rulesets ban
// macro'd special-move motions, unlike SOCD.
//
// Direction-only on purpose -- you press the attack button yourself after,
// so combo-confirm timing stays real skill.
//
// Driven by defer_exec(), not wait_ms(): wait_ms() blocks the whole matrix
// scan for the ~130-230ms a motion takes, dropping any key pressed meanwhile
// (including your attack press right after).

#include "quantum.h"
#include "deferred_exec.h"

// Hint only, not a measured optimization -- at ~30ms/step this is unmeasurable either way.
#define motion_unlikely(x) __builtin_expect(!!(x), 0)

typedef enum {
    DIR_NEUTRAL = 0,
    DIR_UP      = 1 << 0,
    DIR_DOWN    = 1 << 1,
    DIR_FORWARD = 1 << 2, // Resolved to left/right at runtime -- see motion_facing_right.
    DIR_BACK    = 1 << 3,
} motion_dir_t;

// Forward/back are relative to facing; default is facing left (forward = KC_D). Flip with
// MACRO_FACE_TOG (Tab on GAME) after a cross-up -- can't be detected automatically.
static bool motion_facing_right = true;

static inline uint16_t motion_kc_forward(void) {
    return motion_facing_right ? KC_D : KC_A;
}
static inline uint16_t motion_kc_back(void) {
    return motion_facing_right ? KC_A : KC_D;
}
static inline void motion_toggle_facing(void) {
    motion_facing_right = !motion_facing_right;
}

// Half-circle forward: back, down-back, down, down-forward, forward.
static const uint8_t MOTION_HALF_CIRCLE[] = {
    DIR_BACK, DIR_BACK | DIR_DOWN, DIR_DOWN, DIR_DOWN | DIR_FORWARD, DIR_FORWARD,
};

// Dragon punch / "Z" motion: forward, (brief neutral), down, down-forward.
static const uint8_t MOTION_DP[] = {
    DIR_FORWARD,
    DIR_NEUTRAL,
    DIR_DOWN,
    DIR_DOWN | DIR_FORWARD,
};

// Full 360 (Zangief SPD-style). Starts/ends on DOWN, never rests on bare UP -- most games commit
// to a jump the instant pure "up" is read, and that can't be undone by releasing it later. If it
// still jumps in your game, drop the UP_BACK/UP_FORWARD steps for a 270 that skips the top.
static const uint8_t MOTION_360[] = {
    DIR_DOWN, DIR_DOWN | DIR_BACK, DIR_BACK, DIR_UP | DIR_BACK, DIR_UP | DIR_FORWARD, DIR_FORWARD, DIR_DOWN | DIR_FORWARD, DIR_DOWN,
};

// 25-41ms/step: clears one 60fps frame (16.67ms) so a step can't be missed, and varies like a
// human's uneven timing instead of a metronome.
#define MOTION_STEP_BASE_MS 33
#define MOTION_STEP_JITTER_MS 8

static uint32_t motion_rng_state;
static bool     motion_rng_seeded = false;

// xorshift32 -- doesn't need to be a good PRNG, just non-repeating.
static uint32_t motion_next_rand(void) {
    motion_rng_state ^= motion_rng_state << 13;
    motion_rng_state ^= motion_rng_state >> 17;
    motion_rng_state ^= motion_rng_state << 5;
    return motion_rng_state;
}

static uint8_t motion_jittered_step_ms(void) {
    if (!motion_rng_seeded) {
        motion_rng_state  = timer_read32() | 1; // Must stay non-zero or xorshift gets stuck at 0.
        motion_rng_seeded = true;
    }
    const uint8_t jitter = motion_next_rand() % (2 * MOTION_STEP_JITTER_MS + 1);
    return MOTION_STEP_BASE_MS - MOTION_STEP_JITTER_MS + jitter;
}

// State for the single in-flight motion -- see motion_play()'s guard.
typedef struct {
    const uint8_t *steps;
    uint8_t        len;
    uint8_t        idx;
    uint8_t        held;
    uint16_t       forward_kc;
    uint16_t       back_kc;
    bool           active;
} motion_playback_t;

static motion_playback_t motion_playback;

static inline void motion_apply_step(uint8_t want) {
    const uint8_t to_release = motion_playback.held & ~want;
    const uint8_t to_press   = want & ~motion_playback.held;

    if (motion_unlikely(to_release & DIR_UP)) unregister_code(KC_W);
    if (motion_unlikely(to_release & DIR_DOWN)) unregister_code(KC_S);
    if (motion_unlikely(to_release & DIR_FORWARD)) unregister_code(motion_playback.forward_kc);
    if (motion_unlikely(to_release & DIR_BACK)) unregister_code(motion_playback.back_kc);

    if (motion_unlikely(to_press & DIR_UP)) register_code(KC_W);
    if (motion_unlikely(to_press & DIR_DOWN)) register_code(KC_S);
    if (motion_unlikely(to_press & DIR_FORWARD)) register_code(motion_playback.forward_kc);
    if (motion_unlikely(to_press & DIR_BACK)) register_code(motion_playback.back_kc);

    motion_playback.held = want;
}

// Applies one step per call, reschedules for the next jittered interval -- non-blocking, so
// scanning and other keys are never stalled mid-motion.
static uint32_t motion_step_cb(uint32_t trigger_time, void *cb_arg) {
    if (motion_unlikely(motion_playback.idx >= motion_playback.len)) {
        // Release what the last step held; the attack press is yours, separately.
        motion_apply_step(DIR_NEUTRAL);
        motion_playback.active = false;
        return 0; // Stop rescheduling.
    }

    motion_apply_step(motion_playback.steps[motion_playback.idx]);
    motion_playback.idx++;
    return motion_jittered_step_ms();
}

static void motion_play(const uint8_t *steps, uint8_t len) {
    // Refuse to (re-)start while already playing, or while a real WASD key is held: raw
    // register_code here bypasses socd_cleaner's own state tracking and could desync it.
    if (motion_unlikely(motion_playback.active || is_key_pressed(KC_W) || is_key_pressed(KC_A) || is_key_pressed(KC_S) || is_key_pressed(KC_D))) {
        return;
    }

    motion_playback.steps      = steps;
    motion_playback.len        = len;
    motion_playback.idx        = 0;
    motion_playback.held       = DIR_NEUTRAL;
    motion_playback.forward_kc = motion_kc_forward();
    motion_playback.back_kc    = motion_kc_back();
    motion_playback.active     = true;

    defer_exec(1, motion_step_cb, NULL);
}
