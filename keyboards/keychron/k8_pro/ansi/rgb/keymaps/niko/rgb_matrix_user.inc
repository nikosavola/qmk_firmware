// Same as PaletteFx Reactive (modules/getreuer/palettefx), except LEDs with no
// nearby keypress stay fully off instead of sitting at a ~25% background glow,
// and a held key's own LED stays lit at full brightness for as long as it's
// held, fading out smoothly only after release.

// Needs g_last_hit_tracker, which quantum/rgb_matrix/post_config.h only
// declares when some other enabled effect (a stock "reactive" animation, in
// this keymap's case) has already pulled in RGB_MATRIX_KEYPRESSES. Matches
// the guard upstream's own PALETTEFX_REACTIVE uses.
#ifndef RGB_MATRIX_KEYREACTIVE_ENABLED
#    error PALETTEFX_REACTIVE_DARK needs RGB_MATRIX_KEYREACTIVE_ENABLED (enable a keypress-reactive RGB Matrix animation in keyboard.json/rules.mk).
#endif

RGB_MATRIX_EFFECT(PALETTEFX_REACTIVE_DARK)

// Solid-highlights the fighting-game GAME layer's addressable keys (WASD,
// grab/Ctrl, the six attack buttons, Tab/F/R/C) and does a slow, dim hue
// wave across everything else. Swapped in/out by GAME_TOG in keymap.c
// (saving/restoring whatever mode was active before), not selected via
// RM_NEXT, since it's only meaningful while GAME is actually on.
RGB_MATRIX_EFFECT(GAME_MODE_HIGHLIGHT)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#include "palettefx.h"
#include "matrix.h"
#include "timer.h"

// Bump amplitude as a function of time: ramps up over t=0..32, holds at 255
// over t=32..55, then decays. Used both for upstream's decay-since-hit (via
// g_last_hit_tracker's tick) and for our own hold-sustain decay-since-release
// below -- same curve, so a held key's release fades exactly like a tap's
// natural decay, just starting from t=55 (the end of the plateau) instead of
// t=0, with no discontinuity.
static uint8_t amplitude(uint8_t t) {
  if (t <= 55) {
    return (t < 32) ? (4 + 8 * t) : 255;
  } else {
    t = (((uint16_t)(255 - t)) * UINT16_C(164)) >> 7;
    return scale8(t, t);
  }
}

// g_last_hit_tracker only records *when* a key was hit, not whether it's
// still held, and its 8-entry buffer can evict a held key's own entry if
// enough other keys are hit while it's down. So hold-sustain is tracked
// entirely separately here, per LED, independent of that shared buffer.
// Needs the reverse of g_led_config.matrix_co (LED index -> row/col) to ask
// the matrix scanner directly via matrix_is_on(). Built once, lazily, since
// g_led_config isn't necessarily populated yet at effect-registration time.
static uint8_t led_to_row[RGB_MATRIX_LED_COUNT];
static uint8_t led_to_col[RGB_MATRIX_LED_COUNT];
static bool sustain_held[RGB_MATRIX_LED_COUNT];
static bool sustain_decaying[RGB_MATRIX_LED_COUNT];
static uint32_t sustain_release_time[RGB_MATRIX_LED_COUNT];
static bool led_lookup_ready = false;

static void ensure_led_lookup(void) {
  if (led_lookup_ready) { return; }
  for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; ++i) {
    led_to_row[i] = MATRIX_ROWS;  // Sentinel: no physical key at this LED.
  }
  for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
    for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
      const uint8_t led_index = g_led_config.matrix_co[row][col];
      if (led_index != NO_LED) {
        led_to_row[led_index] = row;
        led_to_col[led_index] = col;
      }
    }
  }
  led_lookup_ready = true;
}

// Refreshes held/releasing state for every LED with a physical key. Cheap
// (a couple hundred LEDs at most) and safe to call from every effect
// invocation, including the chunked re-invocations RGB Matrix makes per
// frame for large LED counts -- idempotent once a transition is observed.
static void sustain_update(void) {
  for (uint8_t led_index = 0; led_index < RGB_MATRIX_LED_COUNT; ++led_index) {
    if (led_to_row[led_index] == MATRIX_ROWS) { continue; }
    if (matrix_is_on(led_to_row[led_index], led_to_col[led_index])) {
      sustain_held[led_index] = true;
      sustain_decaying[led_index] = false;
    } else if (sustain_held[led_index]) {
      // Just released: start the decay clock fresh from the plateau, not
      // from whatever g_last_hit_tracker's own tick (which never paused
      // while held) happens to say.
      sustain_held[led_index] = false;
      sustain_decaying[led_index] = true;
      sustain_release_time[led_index] = timer_read32();
    }
  }
}

static uint8_t sustain_amplitude(uint8_t led_index) {
  if (sustain_held[led_index]) {
    return 255;
  }
  if (sustain_decaying[led_index]) {
    uint32_t elapsed = timer_elapsed32(sustain_release_time[led_index]);
    if (elapsed > UINT16_MAX) { elapsed = UINT16_MAX; }
    const uint16_t t = 55 + scale16by8((uint16_t)elapsed,
        1 + rgb_matrix_config.speed / 4);
    if (t <= 255) {
      return amplitude((uint8_t)t);
    }
    sustain_decaying[led_index] = false;  // Decay finished.
  }
  return 0;
}

static bool PALETTEFX_REACTIVE_DARK(effect_params_t* params) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);
  ensure_led_lookup();
  if (params->init) {
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; ++i) {
      sustain_held[i] = false;
      sustain_decaying[i] = false;
    }
  }
  sustain_update();

  const uint16_t* palette = palettefx_get_palette_data();
  const uint8_t count = g_last_hit_tracker.count;

  uint8_t hit_amplitude[LED_HITS_TO_REMEMBER] = {0};
  for (uint8_t j = 0; j < count; ++j) {
    const uint16_t tick = scale16by8(g_last_hit_tracker.tick[j],
        1 + rgb_matrix_config.speed / 4);
    if (tick <= 255) {
      hit_amplitude[j] = amplitude((uint8_t)tick);
    }
  }

  for (uint8_t i = led_min; i < led_max; ++i) {
    RGB_MATRIX_TEST_LED_FLAGS();
    uint8_t value = sustain_amplitude(i);

    for (uint8_t j = 0; j < count; ++j) {
      if (hit_amplitude[j] == 0) { continue; }

      uint8_t dx = abs8((g_led_config.point[i].x - g_last_hit_tracker.x[j]) / 2);
      uint8_t dy = abs8((g_led_config.point[i].y - g_last_hit_tracker.y[j]) / 2);
      if (dx < 21 && dy < 21) {
        const uint16_t dist_sqr = dx * dx + dy * dy;
        if (dist_sqr < 21 * 21) {  // Accumulate a radial bump for each hit.
          const uint8_t dist = sqrt16(dist_sqr);
          value = qadd8(value, scale8(255 - 12 * dist, hit_amplitude[j]));
          // Early loop exit where the value has saturated.
          if (value == 255) { break; }
        }
      }
    }

    hsv_t hsv = palettefx_interp_color(palette, value);
    if (value == 0) {  // Fully off at idle, unlike upstream Reactive's ~25% floor.
      hsv.v = 0;
    } else if (value < 32) {
      hsv.v = scale8(hsv.v, 8 * value);
    }

    const rgb_t rgb = rgb_matrix_hsv_to_rgb(hsv);
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return rgb_matrix_check_finished_leds(led_max);
}

// Matrix positions of the GAME layer's addressable keys, verified against
// keyboards/keychron/k8_pro/info.json's LAYOUT_tkl_ansi "matrix" fields
// (cross-checked against each key's x/y there) rather than assumed from
// keymap.c's argument order -- this keymap already found one silent
// wrong-assumption bug from skipping that kind of verification (see the
// `enum layers` comment in keymap.c), not eager to add a second.
static const keypos_t GAME_MODE_ADDRESSABLE_KEYS[] = {
    {.row = 2, .col = 2},  // W
    {.row = 3, .col = 1},  // A
    {.row = 3, .col = 2},  // S
    {.row = 3, .col = 3},  // D
    {.row = 5, .col = 0},  // Ctrl (grab)
    {.row = 1, .col = 14}, // Ins
    {.row = 2, .col = 14}, // Del
    {.row = 1, .col = 15}, // Home
    {.row = 2, .col = 15}, // End
    {.row = 1, .col = 16}, // PgUp
    {.row = 2, .col = 16}, // PgDn
    {.row = 2, .col = 0},  // Tab (MACRO_FACE_TOG)
    {.row = 3, .col = 4},  // F (MACRO_HALF_CIRCLE)
    {.row = 2, .col = 4},  // R (MACRO_DP)
    {.row = 4, .col = 4},  // C (MACRO_360)
};

static bool game_mode_addressable[RGB_MATRIX_LED_COUNT];
static bool game_mode_addressable_ready = false;

static void ensure_game_mode_addressable(void) {
    if (game_mode_addressable_ready) { return; }
    for (uint8_t i = 0; i < ARRAY_SIZE(GAME_MODE_ADDRESSABLE_KEYS); ++i) {
        const keypos_t pos = GAME_MODE_ADDRESSABLE_KEYS[i];
        const uint8_t  led = g_led_config.matrix_co[pos.row][pos.col];
        if (led != NO_LED) {
            game_mode_addressable[led] = true;
        }
    }
    game_mode_addressable_ready = true;
}

static bool GAME_MODE_HIGHLIGHT(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    ensure_game_mode_addressable();

    // Hue drifts by x-position plus a slowly incrementing phase -- a full
    // wave sweep takes several seconds, low saturation/value so it reads as
    // calm background motion, not competing for attention with the solid
    // highlight color.
    const uint8_t wave_phase = (uint8_t)(timer_read32() >> 6);

    for (uint8_t i = led_min; i < led_max; ++i) {
        RGB_MATRIX_TEST_LED_FLAGS();
        if (game_mode_addressable[i]) {
            rgb_matrix_set_color(i, 0, 200, 255); // Same cyan as the GAME_TOG indicator flash.
        } else {
            const hsv_t hsv = {.h = (uint8_t)(g_led_config.point[i].x + wave_phase), .s = 180, .v = 40};
            const rgb_t rgb = rgb_matrix_hsv_to_rgb(hsv);
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        }
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif  // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
