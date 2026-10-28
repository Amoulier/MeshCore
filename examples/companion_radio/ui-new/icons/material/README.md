# SuperBase LED and buzzer artwork

Selected by the user from preview option B. Source artwork: Google Material
Icons, version 3.0.1, Copyright Google Inc., Apache License 2.0; see
[LICENSE.txt](LICENSE.txt). The existing Bluetooth icons are preserved.

- `lightbulb-outline.svg`: https://github.com/google/material-design-icons/blob/3.0.1/action/svg/design/ic_lightbulb_outline_24px.svg
  SHA-256 `81e3018146de97c5c0e8188768aabe4a2664b37189b4234384a4da9154cc98cc`.
- `volume-up.svg`: https://github.com/google/material-design-icons/blob/3.0.1/av/svg/design/ic_volume_up_24px.svg
  SHA-256 `08d6e6b0a28058dd39f56ab46fa80efbbdff719aee27f70bb25fe6f6b90d22e0`.
- `volume-off.svg`: https://github.com/google/material-design-icons/blob/3.0.1/av/svg/design/ic_volume_off_24px.svg
  SHA-256 `4d8a0258b1d31c07a1548feec0158cdc1cd890c1a164ca8d8dc723cdf16aaddd`.

Modifications: rasterize each 24-unit SVG onto a 32 x 32 transparent canvas,
threshold alpha at 128, and pack rows MSB first (128 bytes per bitmap) for the
existing Adafruit drawBitmap renderer. The LED On uses lightbulb outline; Buzzer
On/Off use volume up/off. LED Off uses the lightbulb bitmap with the diagonal
from the existing Bluetooth Off icon, clearing one neighboring pixel before
compositing the white diagonal. These are the exact bitmaps in preview option B.

Rasterizer used for this derivation: Sharp 0.35.5. SVG files are preserved here;
ordinary firmware builds use the committed arrays and need no rasterizer.
Only the four SuperBase LED/buzzer arrays in ../../icons.h change. The original
Bluetooth bitmaps, control handling and sound/LED policies remain unchanged.
