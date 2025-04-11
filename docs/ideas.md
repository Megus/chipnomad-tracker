# ChipNomad

This document is for feature ideas and UI sketches.

ChipNomad is a multi-chip tracker for real chips. What chips I consider supporting (in the order of my personal preference):

- AY-3-8910/YM2149F
- SID
- Yamaha FM chips like YM2612
- 2A03
- POKEY

## UI Concept

Tracker "screen" resolution: 40x20 characters. The core UI concept and song structure is the same as in LSDJ or M8 Tracker:
multiple screens, each screen is dedicated to a single function. All screens, except for the **SONG**,
don't have any scrolling content, so the length of chains, phrases, and tables is naturally limited to 16.

- Main screens: **S**ong, **C**hain, **P**hrase, **I**nstrument, **T**able
- Additional sceens: **P**roject, **G**roove

Screen map:

```
P G
SCPIT
```

## Project

You arrange a song as a list of chains on each track. A chain is a list of phrases with optional transpose
for each phrase. A phrase is the lowest sequencing level, it has 16 rows of notes. Phrases can be reused
in multiple chains, and chains can be used on any track.

- 256 positions in song (00-FF)
- 255 chains (00-FE)
- 1024 phrases (000-3FF).
- 16 Grooves (0-F)
- 128 instruments (00-7F)
- 255 tables (128 instrument tables + 127 additional)

Project also has chip setup. I'm starting with AY/YM only. Chip parameters for AY/YM:

- Chip clock speed (1.75, 1.77...)
- Frame rate (48.828, 50, 60...)
- Chip type (AY/YM)
- Stereo layout (ABC, ACB, BAC...)
- Number of chips (1-3)
- Pitch table:
  - Auto-calculated for clock speed
  - Selection of classic tables (ST, ASC, PT...)
  - Load table from a text file

## Song

40x20 screen allows to have up to 10 tracks on the screen. It opens the possibility
to make music for multi-chip setups (ZX TurboSound, ZX Spectrum Next 3xAY, 2xSID). Currently
I don't consider supporting mixing different chip types in a song, however,
tracker design will allow it.

UI is generally identical to LSDJ.

```
.        .         .         .
012346789012345678901234567890123456789

   1  2  3  4  5  6  7  8  9  10
00 11 22 33 44 55 66 77 88 99 --  1 ---
```

## Chain

A chain is a list of phrases with optional transposition for each phrase. A chain can have up to 16 phrases.

```
.        .         .         .
012346789012345678901234567890123456789

   P   T
00 012 03                         1 ---
```

## Phrase

A phrase has 16 tracker rows. UI is identical to M8: each row has note, instrument, volume, and 3 FX.
AY only has 16 volume levels, but because I'm designing for multiple chips, volume column has 2 characters.

```
.        .         .         .
012346789012345678901234567890123456789

  N   I  V  FX1   FX2   FX3
0 C-5 00 00 PVB00 PBNF0 ARP37     1 C-5
```

## FX

There are sequencer FX which are common for all chips, and also chip-specific FX. The way M8 implements
UI to pick FX allows to define chip-specific FX easily.

### Sequencer FX

- ARP - Arpeggio
- ARC - Arpeggio config
- THO - Table hop
- SNG - Song hop
- PVB - Pitch Vibrato
- PBN - Pitch bend
- PSL - Pitch slide (portamento)
- PIT - 1-time pitch offset
- TBL - Table (replaces default instrument table)
- TBX - Additional table (adds another table)
- RET - Retrigger
- DEL - Note delay
- OFF - Note off after X ticks
- KIL - Kill note after X ticks
- GRV - Select track groove
- GGR - Select global groove
- TIC - Set table speed

For future: Generative elements: randomization, scale following (like on M8).

### Chip-specific FX:

#### AY

- AYM - Mixer settings (tone, noise, env shape)
- EVB - Env Vibrato
- ERT - Env retrig
- EBN - Env pitch bend
- ESL - Env pitch slide (portamento)
- ENA - Env pitch as a note, absolute
- ENR - Env pitch as a note, relative (for arpeggios in tables, for example)
- EPR - Env pitch value relative
- EPL, EPH - Env period as a value, Low and High regs
- EPA - Env auto-pitch: off, or rate
- NOI - Noise offset
- NOA - Noise absolute value

## Instruments

Instrument settings are different for each chip. Tables open additional sound design possibilities.

Common instrument settings:

- Name
- Transpose enabled on/off (good for drums)
- Table default speed

### AY

- Volume ADSR envelope
- Env auto-pitch: off, or rate (1:1, 2:1, etc)

## Tables

Tables are the main sound design tool in ChipNomad. If you're familiar with Vortex Tracker, tables are
the mix of instruments and ornaments and can do even more. If you're familiar with LSDJ and M8, you know
what tables are.

Pitch column can have relative or absolute pitch values in semitones. Volume is relative to the main ADSR
envelope. Four FX lanes are generally equal to FX lanes in phrase, however, there can be minor differences
in the behavior of FX in phrases and tables.

```
.        .         .         .
012346789012345678901234567890123456789

  P   V  FX1   FX2   FX3   FX4
0 +04 FF PVB00 PBNF0 NOI01 NOI01  1 C-5
1 -05 --
2 =18 --
3 =18 --
```
