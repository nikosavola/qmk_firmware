// Same as PaletteFx Reactive (modules/getreuer/palettefx), except LEDs with no
// nearby keypress stay fully off instead of sitting at a ~25% background glow.

// Needs g_last_hit_tracker, which quantum/rgb_matrix/post_config.h only
// declares when some other enabled effect (a stock "reactive" animation, in
// this keymap's case) has already pulled in RGB_MATRIX_KEYPRESSES. Matches
// the guard upstream's own PALETTEFX_REACTIVE uses.
#ifndef RGB_MATRIX_KEYREACTIVE_ENABLED
#    error PALETTEFX_REACTIVE_DARK needs RGB_MATRIX_KEYREACTIVE_ENABLED (enable a keypress-reactive RGB Matrix animation in keyboard.json/rules.mk).
#endif

RGB_MATRIX_EFFECT(PALETTEFX_REACTIVE_DARK)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#include "palettefx.h"

static bool PALETTEFX_REACTIVE_DARK(effect_params_t* params) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);
  const uint16_t* palette = palettefx_get_palette_data();
  const uint8_t count = g_last_hit_tracker.count;

  uint8_t amplitude(uint8_t t) {  // Bump amplitude as a function of time.
    if (t <= 55) {
      return (t < 32) ? (4 + 8 * t) : 255;
    } else {
      t = (((uint16_t)(255 - t)) * UINT16_C(164)) >> 7;
      return scale8(t, t);
    }
  }

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
    uint8_t value = 0;

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

#endif  // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
