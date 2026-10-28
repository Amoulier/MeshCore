# SuperBase OLED emoji artwork

Artwork: [OpenMoji](https://openmoji.org/), by the OpenMoji contributors,
[release 17.0.0](https://github.com/hfg-gmuend/openmoji/releases/tag/17.0.0).
The original artwork and the derived `OpenMojiData.inc` bitmap/trie data are
licensed under **CC BY-SA 4.0**; see [LICENSE.txt](LICENSE.txt).
The renderer's source code retains the repository's source-code license.

The generated font recognizes all 3,953 standard emoji sequences in that
release's metadata, excluding OpenMoji-specific extras. It includes joystick,
faces, flags, keycaps, skin modifiers and joined sequences. Variation selectors
are consumed without drawing separate boxes. Longest-sequence matching keeps
joined emoji together during measurement, copying, ellipsis and line wrapping.
The node's UTF-8 name, contacts and message contents are not rewritten.

The OLED is monochrome: these are simplified **10 x 10 pixel** pictograms with
an 11-pixel advance, scaled with the existing text-size setting. Color differences
such as skin tones and many flags cannot be distinguished in one bit; different
sequences may intentionally share a bitmap. Detailed artwork loses detail at
this size. Unsupported code points use `?`; this is not a general Unicode text
font. ASCII and common Spanish letters use the existing CP437 text font.

## Regeneration

Ordinary firmware builds use the committed data and need no image tooling.
To regenerate, use Python/Pillow and Node/Sharp in a development environment:

1. Download `openmoji-svg-black.zip` and `openmoji-svg-color.zip` from the pinned
   release, plus `data/openmoji.json` from the repository's `17.0.0` tag.
   The generator checks all three SHA-256 hashes (recorded in `manifest.json`).
2. Select metadata records whose `group` does not start with `extras-`. Extract
   their `<hexcode>.svg` files from the black archive into an SVG directory.
   Use only hexadecimal names from the metadata, never arbitrary archive paths.
   Render with `node bin/rasterize-openmoji.cjs SVG_DIR PNG_DIR`.
3. The upstream black SVG for `1F195` (NEW button) is empty. Extract that one
   file from the pinned color archive into a separate directory; render it with
   the same command into `PNG_DIR`, replacing only `1F195.png`.
4. Run `python bin/generate-superbase-emoji.py openmoji-svg-black.zip openmoji.json
   --color-fallback-archive openmoji-svg-color.zip --raster-dir PNG_DIR` on one
   command line from the repository root.

Modifications to the original artwork: composite black ink with transparency,
crop to ink bounds, thicken with a 3-pixel maximum filter, reduce with Lanczos,
center on a 10 x 10 canvas, threshold at 80/255, and deduplicate identical
bitmaps. The color fallback uses luminance to produce the same monochrome form.
The lookup trie removes presentation selectors and shares prefixes; no artwork
is loaded or allocated dynamically on the device.

`python test/superbase/run.py` verifies every generated sequence and the renderer.
Linux/WSL also supports `--sanitize` for malformed UTF-8 and buffer boundaries.
`test/superbase/emoji.cpp --render` (compiled executable) can produce a 128 x 128
PGM preview using the installed Adafruit GFX classic font bytes, without needing
the node or changing its name.
