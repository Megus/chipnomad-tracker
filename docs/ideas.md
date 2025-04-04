# Tracker ideas

This document is for feature ideas and UI sketches.

ChipNomad is a multi-chip tracker for real chips. What chips I consider supporting (in the order of my personal preference):

- AY-3-8910/YM2149F
- SID
- Yamaha FM chips like YM2612
- 2A03
- POKEY

## UI Concept

The core UI concept is the same as in LSDJ or M8 Tracker: multiple screens, each screen dedicated to a single function.

Main screens:

- **S**ong
- **C**hain
- **P**hrase
- **I**nstrument
- **T**able

Additional sceens:

- **P**roject
- **G**roove

```
P G
SCPIT
```

## Project

256 positions in song
256 chains
1024 phrases (000-3FF).
16 Grooves
128 instruments
128 tables

Chip parameters for AY/YM:

- Chip clock speed (1.75, 1.77...)
- Frame rate (48.828, 50, 60...)
- Chip type (AY/YM)
- Stereo layout (ABC, ACB, BAC...)
- Number of chips (1-3)
- Pitch table – specific or auto-calculated for chip speed

## Song

40x20 screen allows to have up to 10 tracks on the screen. It opens the possibility
to make music for multi-chip setups (ZX TurboSound, ZX Spectrum Next 3xAY, 2xSID). Currently
I don't consider supporting mixing different chip types in a song.

UI is generally identical to LSDJ:

```
.        .         .         .
012346789012345678901234567890123456789

   A1 B1 C1 A2 B2 C2 A3 B3 C3 ..
00 11 22 33 44 55 66 77 88 99 ..  1 ---
```

## Chain

Chains are identical to LSDJ: list of phrases with transposition.

```
.        .         .         .
012346789012345678901234567890123456789

   P  T
00 12 03                          1 ---
```

## Phrase

UI is identical to M8: each row has note, instrument, volume, and 3 FX.

```
.        .         .         .
012346789012345678901234567890123456789

  N   I  V  FX1   FX2   FX3
0 C-5 00 00 PVB00 PBNF0 ARP37     1 C-5
```

## FX

Ideas for FX. Some FX in the list are AY/YM-specific. The way M8 implements UI to pick FX allows to
define chip-specific FX easily.

- Arpeggio
- Table hop
- Song hop
- Pitch Vibrato
- Env Vibrato
- Pitch bend (slide)
- Portamento
- Table
- Additional table
- Retrigger
- Note delay
- Note off after X ticks
- Env pitch as a note, absolute
- Env pitch as a note, relative (for arpeggios in tables, for example)
- Env pitch value relative
- Env pitch as a value H and L regs
- Env retrig
- Env pitch bend
- Env portamento
- Noise offset
- 1-time pitch offset

For future: Generative elements: randomization, scale following (like on M8).

## Instruments

Instrument parameters:

- Name
- Transpose enabled on/off
- Tic speed

In LSDJ and M8 instruments have much more parameters, but there aren't many more parameters for AY/YM instruments.
I see that the majority of controls should be in the instrument table.

Mixer control: TNE
Pitch control - absolute and relative, in semitones
Volume control - absolute and relative
Env pitch control - absolute and relative - via FX
Noise pitch control - absolute and relative. Maybe also via FX?

Env retrig - FX command (useful when shape doesn't change).

Instrument table UI idea for AY/YM instruments

```
.        .         .         .
012346789012345678901234567890123456789

  P    N   TNE V  FX1   FX2
0 .04 05+ TN. F. PVB00 PBNF0      1 C-5
1 .FC 00. T.C F.
2 =18 00. T.. E-
3 =18 00. ..E E.
```

With noise control in FX we can have 3 FX lanes:

```
.        .         .         .
012346789012345678901234567890123456789

  P   TNE V  FX1   FX2   FX3
0 .04 TN. F. PVB00 PBNF0 NSE01    1 C-5
1 .FC T.C F.
2 =18 T.. E-
3 =18 ..E E.
```

Relative volume can also be an FX and not a copy of VT UI.

## Tables

M8 allows to use additional tables, the idea similar to ornaments in Vortex Tracker.