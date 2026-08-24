# Combat2EF playable lane

This revision deliberately separates ROM production from unresolved LOADW/WIMP research.

- Frozen development visual generator source commit: `14a5f7a739e69de39d3912e8b70c8f33dd9ccc8b`
- Frozen generator Git blob: `75a371c2e92685982666a080adcedccb3aa5a52c`
- Historical Midway source commit: `1280555b4d041dd025198c8e85ed14b4c1c91cfb`
- Final strict converter remains: `tools/fix39_character_assets.py`
- ROM-producing development converter: `tools/fix39_character_assets_development_frozen.py`

The frozen converter reproduces the last hardware-working Combat2AM character-art pipeline. It is a development fixture, not a claim that LOADW palette/index semantics are solved. Final art conversion remains gated by the strict EC source-proof path.
