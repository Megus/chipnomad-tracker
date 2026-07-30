---
title: Project Screen
description: ChipNomad Manual - Project Screen
layout: single
---

# Project Screen

{{< toc >}}

![](project.png)

Configure global project settings:

- Filename, song title, song author
- Linear pitch on/off
- Tick rate (up to 200Hz)
- Chip type (AY/YM chips currently)

AY-specific settings:

- Chips count (1-3)
- Chip subtype: AY-3-8910 or YM2149F
- Stereo layout (ABC, ACB, BAC)
- Stereo width
- PWM range (for AY Plus instruments)
- Chip clock (select from several most common values)
- Pitch table operations: load, save, generate

## Manage Project

![](manage.png)

Additonal screen with helpful project management options. Press **OPT** to leave the screen.

## Pitch Table

![](pitch-table.png)

Load/save pitch tables and generate a standard 12TET table (A4 = 440Hz) for the selected chip clock.

## Export

![](export.png)

Export to WAV as a mix, or create stems (each track is a separate WAV file).

Export to PSG format to use with players native to retro platforms that use AY/YM chips (ZX Spectrum, Atari ST, Amstrad CPC, etc). PSG doesn't support software oscillators (4th oscillator in AY Plus instruments).

Export to VGM format to use with the wide variety of VGM players. VGM will capture AY/YM software oscillators but some sounds may not be captured precisely. Sample data export is not optimized, so if you use samples in the project then VGM file may be very big.