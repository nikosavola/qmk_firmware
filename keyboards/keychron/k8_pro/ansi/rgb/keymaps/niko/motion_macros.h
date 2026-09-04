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

// Offline-only fighting-game motion macros (half-circle, dragon-punch/"Z",
// 360), gated behind the GAME layer toggle so they can never fire during
// normal typing or get left on by accident going into online play -- most
// fighting games' own ranked/tournament rulesets explicitly ban macro'd
// special-move motions (removes the execution skill being tested), unlike
// SOCD cleaning, which every major ruleset allows since it just resolves a
// keyboard's physical limitation.
//
// Motions are direction-only: they end holding whatever the final step
// holds, then return control to you to press the actual attack button
// yourself, on purpose -- auto-firing the attack too would remove the
// combo-confirm timing that's the actual skill involved.
//
// Driven by defer_exec() (quantum/deferred_exec.h, already compiled into
// this build via other core features -- no rules.mk change needed), NOT a
// wait_ms() loop. wait_ms() blocks process_record_user, which blocks the
// whole matrix scan: a ~130-230ms motion would silently drop any other key
// (including the attack button you press right after) for its entire
// duration. Stepping one direction transition per deferred callback keeps
// scanning alive the whole time.

#include "quantum.h"
#include "deferred_exec.h"

// Purely a documentation/hint tool, not a measured optimization -- at the
// ~30ms-per-step rate this code runs at, a branch misprediction (a few CPU
// cycles) is unmeasurably small either way. Applied only where the skew is
// actually known ahead of time: each direction bit is unset in most
// press/release checks (a step usually changes 1-2 of 4 bits), and a motion
// is mid-sequence on every step but its last.
#define motion_unlikely(x) __builtin_expect(!!(x), 0)

typedef enum {
    DIR_NEUTRAL = 0,
    DIR_UP      = 1 << 0,
    DIR_DOWN    = 1 << 1,
    DIR_FORWARD = 1 << 2, // Resolved to left/right at runtime -- see motion_facing_right.
    DIR_BACK    = 1 << 3,
} motion_dir_t;

// Quarter/half-circle motions are defined in relative terms (forward/back).
// Default is "facing left" (forward = KC_D, i.e. your character stands on
// the left side of the screen moving right); toggle this (MACRO_FACE_TOG,
// bound to Tab on the GAME layer) once your character ends up on the right
// side facing left instead -- e.g. after a cross-up or side-switching combo.
// Nothing here can detect that automatically: a keyboard has no visibility
// into which way your character is actually facing in-game.
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

// Full 360 (Zangief SPD-style). Starts and ends on DOWN, not UP: most
// fighting games commit to a jump the instant a pure "up" direction is read
// (often within a single frame), and that commitment can't be undone by
// releasing up a moment later -- so resting a whole macro step on bare
// DIR_UP (as an earlier version of this did) reliably turned the 360 into a
// jump instead of a grab. Going UP_BACK -> UP_FORWARD directly keeps UP
// held continuously through the top of the rotation without ever being the
// *only* direction held, which is what actually seems to trigger the jump
// commit in most games' input readers -- this narrows the risk but hasn't
// been confirmed jump-free on real hardware/game combinations yet. If it
// still jumps in your game, the standard fallback is a 270 that skips the
// top of the circle entirely (drop the UP_BACK/UP_FORWARD steps and go
// BACK -> FORWARD directly); most modern fighting games' input leniency
// accepts that as equivalent to a full 360.
static const uint8_t MOTION_360[] = {
    DIR_DOWN, DIR_DOWN | DIR_BACK, DIR_BACK, DIR_UP | DIR_BACK, DIR_UP | DIR_FORWARD, DIR_FORWARD, DIR_DOWN | DIR_FORWARD, DIR_DOWN,
};

// A human doing these motions dwells on each direction for a couple of
// frames, unevenly -- not a metronomic constant. 25-41ms (1.5-2.5 frames at
// 60fps) keeps every step comfortably above one frame (16.67ms) so a fast
// game's input reader can't poll straight through a step without ever
// seeing it, while still varying enough to not look scripted.
#define MOTION_STEP_BASE_MS 33
#define MOTION_STEP_JITTER_MS 8

static uint32_t motion_rng_state;
static bool     motion_rng_seeded = false;

// xorshift32. Doesn't need to be a good PRNG, just needs consecutive macro
// runs to not land on the exact same millisecond every time.
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

// Playback state for the single in-flight motion (only one can run at a
// time -- see motion_play()'s guard).
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

// defer_exec callback: applies one step per invocation, then reschedules
// itself for the next jittered interval. Runs from the same non-blocking
// task hook as everything else (deferred_exec_task(), called every scan),
// so matrix scanning and other keypresses are never stalled while a motion
// plays out.
static uint32_t motion_step_cb(uint32_t trigger_time, void *cb_arg) {
    if (motion_unlikely(motion_playback.idx >= motion_playback.len)) {
        // Release whatever the last step left held. The attack button
        // itself is a separate, deliberate press from you afterward.
        motion_apply_step(DIR_NEUTRAL);
        motion_playback.active = false;
        return 0; // Stop rescheduling.
    }

    motion_apply_step(motion_playback.steps[motion_playback.idx]);
    motion_playback.idx++;
    return motion_jittered_step_ms();
}

static void motion_play(const uint8_t *steps, uint8_t len) {
    // Ignore re-triggers while a motion is already mid-playback, and refuse
    // to start one while a real WASD direction is physically held: raw
    // register_code/unregister_code here bypasses socd_cleaner's own
    // event-based state tracking (it only reacts to real key events, not a
    // continuous scan), so overlapping a macro with a real held direction
    // can desync the two -- e.g. releasing a key here while your finger is
    // still holding it down, or sending two opposing directions at once.
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
