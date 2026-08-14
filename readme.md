# niko's QMK firmware

Personal keyboard bindings, based on a Keychron K8 Pro with OpenRGB support ported forward onto
Keychron's current fork. Keymap source: `keyboards/keychron/k8_pro/ansi/rgb/keymaps/niko/`.

## Layers

Large letter = what your OS's Dvorak layer actually types at that physical key (firmware ships
QWERTY; Dvorak is applied in software, not on the keyboard). Small letter underneath = the
firmware's own QWERTY keycode.

| | |
|---|---|
| ⬜ standard keycode | 🟩 RGB / system (inherited from the openrgb baseline) |
| 🟧 added in this keymap | ▫️ transparent — falls through to base |
| 🔲 part of a combo | |

### Mac Base

![Mac Base layer](niko/images/mac-base-light.png#gh-light-mode-only)
![Mac Base layer](niko/images/mac-base-dark.png#gh-dark-mode-only)

### Mac Fn

![Mac Fn layer](niko/images/mac-fn-light.png#gh-light-mode-only)
![Mac Fn layer](niko/images/mac-fn-dark.png#gh-dark-mode-only)

### Win Base

![Win Base layer](niko/images/win-base-light.png#gh-light-mode-only)
![Win Base layer](niko/images/win-base-dark.png#gh-dark-mode-only)

### Win Fn

![Win Fn layer](niko/images/win-fn-light.png#gh-light-mode-only)
![Win Fn layer](niko/images/win-fn-dark.png#gh-dark-mode-only)

(Mac vs. Win base layer is Keychron's own OS-select key combo, not specific to this keymap. An
interactive, Dvorak-aware version of these diagrams — with hover detail — is also kept up to date
as a Claude artifact; ask if you want the link.)

## Combos and sequences

A few bindings don't fit in a single key square, since they involve more than one physical key or
a typed sequence:

| Trigger | Action |
|---|---|
| **`Q` + `W`** held together, either base layer | Sends `SOCDTOG`, toggling SOCD Cleaner (on by default) on the W/S and A/D pairs. Fires on firmware keycodes before the OS's Dvorak layer sees anything, so it isn't affected by which OS keyboard layout is active. A combo rather than an `Fn` key since it's flipped rarely — no reason to spend a slot on it. |
| **`Fn+L`**, then a sequence | Starts a Super Leader sequence. Currently defined: `B` `O` `O` `T` → reboot to bootloader (for flashing). Sequences live in [`keymaps/niko/super_leader.def`](keyboards/keychron/k8_pro/ansi/rgb/keymaps/niko/super_leader.def) — add more there. |
| **`Fn+H`** | Repeat Key — replays the last keypress, including any modifiers held at the time. |
| **`Fn+K`** | Alt Repeat Key — replays the last key's *alternate* action instead (e.g. repeating `C(KC_C)` gives `C(KC_V)`). |
| **`Fn+Del`** / **`Fn+End`** | Start recording Dynamic Macro 1 / 2. |
| **`Fn+\`** | Stop recording the macro currently being recorded. |
| **`Fn+;`** / **`Fn+'`** | Play back Dynamic Macro 1 / 2. |

## Other enabled features

- **OpenRGB / VIA** — `Fn+O` toggles OpenRGB direct mode vs. VIA lighting control.
- **Caps Word** — tap both shifts to toggle.
- **Caps Lock → Escape** — `Fn+Caps Lock` sends the actual Caps Lock.
- **Mouse Turbo Click** — `Fn+P`, hold for rapid clicks, double-tap to lock.
- **xcase** — `Fn+Y`/`Fn+U`/`Fn+I` for snake/kebab/camelCase, `Fn+J` to turn off.
- **PaletteFx (reactive)** — keypress-driven RGB, cycle palettes with `RM_HUEU`/`RM_HUED`.
- **Sentence Case** — auto-capitalizes the first letter of a sentence; on by default, toggle with `Fn+]`.
- **Autocorrect** — fixes common typos as you finish typing them (70-entry default dictionary);
  off until enabled once with `Fn+[`, then the setting persists.

## Credits

- [Keychron/qmk_firmware](https://github.com/Keychron/qmk_firmware)
- [Ferrhat/qmk_firmware_k8_pro](https://github.com/Ferrhat/qmk_firmware_k8_pro)
