# niko's QMK firmware

Personal keyboard bindings, based on a Keychron K8 Pro with OpenRGB support ported forward onto
Keychron's current fork. See `keyboards/keychron/k8_pro/ansi/rgb/keymaps/niko/` for the keymap.

## Enabled features

- **OpenRGB / VIA** — `Fn+O` toggles OpenRGB direct mode vs. VIA lighting control.
- **Caps Word** — tap both shifts to toggle.
- **Caps Lock → Escape** — `Fn+Caps Lock` sends the actual Caps Lock.
- **Mouse Turbo Click** — `Fn+P`, hold for rapid clicks, double-tap to lock.
- **xcase** — `Fn+Y`/`Fn+U`/`Fn+I` for snake/kebab/camelCase, `Fn+J` to turn off.
- **PaletteFx (reactive)** — keypress-driven RGB, cycle palettes with `RM_HUEU`/`RM_HUED`.
- **Repeat Key** — `Fn+H` repeats the last keypress, `Fn+K` for its alternate action.
- **Super Leader** — `Fn+L`, then a sequence (see `keymaps/niko/super_leader.def`).
- **SOCD Cleaner** — WASD opposing-direction filtering, on by default; toggle with the `Q+W` combo.

## Credits

- [Keychron/qmk_firmware](https://github.com/Keychron/qmk_firmware)
- [Ferrhat/qmk_firmware_k8_pro](https://github.com/Ferrhat/qmk_firmware_k8_pro)
