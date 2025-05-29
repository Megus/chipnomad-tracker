# ChipNomad User Manual

## Introduction

ChipNomad is a multi-platform tracker designed for creating chiptune music. It features a compact 40x20 character interface inspired by LSDJ and M8 Tracker, making it perfect for portable devices with physical controls. ChipNomad will support multiple chips, but currently it supports only AY-3-8910/YM2149F chips.

## Project Structure

A ChipNomad song is organized in a hierarchical structure. The lowest level of the hierarchy is a **Phrase**. Phrase is a sequence of 16 tracker rows for one track. Phrases are grouped into **Chains**. A chain can have up to 16 phrases. You can re-use phrases across multiple chains. Chains are sequenced in a **Song**.

- Song (up to 256 positions)
- Chains (up to 255)
- Phrases (up to 1024)
- Up to 10 tracks for multi-chip setups
- 128 instruments (00-7F)
- 255 tables (128 instrument tables + 127 additional tables)
- 31 grooves (00-1F)

## Screens

ChipNomad has 7 main screens laid out in a map:

```
P G
SCPIT
```

- **P**roject: Global project settings (not yet implemented)
- **S**ong: Main sequencing screen
- **C**hain: Chain editor
- **P**hrase: Phrase editor
- **I**nstrument: Instrument editor
- **T**able: Table editor
- **G**roove: Groove editor

Tracker "screen" resolution: 40x20 characters. The core UI concept and song structure is the same as in LSDJ or M8 Tracker:
multiple screens, each screen is dedicated to a single function. All screens, except for the Song,
don't have any scrolling content, so the length of chains, phrases, and tables is naturally limited to 16.

### Common controls

Control mapping on desktop (you will be able to redefine controls in the future):
- D-Pad - Cursor keys
- Select - left Shift
- Start - Space
- A - X
- B - Z

Navigation:
- Move cursor: LEFT, RIGHT, UP, DOWN
- Screen navigation: Hold SELECT + \[LEFT, RIGHT, UP, DOWN\]

Editing:
- Change value (fine): Hold A + \[LEFT or RIGHT\]
- Change value (coarse): Hold A + \[UP or DOWN\]
- Cut value: B + A

Play:
- Play all tracks (outside Song screen): Hold SELECT + START

### Song Screen
Arrange chains into a complete song:
- Up to 256 positions (00-FF)
- Up to 10 tracks
- Each position contains chain numbers for each track

Controls:

- Insert chain: A
- Create new chain: Double-tap A
- Jump 16 Rows: Hold B + \[UP or DOWN\]
- Clone chain: Select + B then A

### Chain Screen
Create sequences of phrases:
- 16 rows per chain
- Each row contains phrase number and transpose value
- Chains can be reused across different tracks

Controls:

- Insert phrase: A
- Create new phrase: Double-tap A
- Jump to Track: Hold B + \[LEFT or RIGHT\]
- Jump to Chain: Hold B + \[UP or DOWN\]
- Clone phrase: Select + B then A

### Phrase Screen
Create note patterns:
- 16 rows per phrase
- Contains notes, instruments, and effects
- 3 effect columns per row

Controls:
- Insert note: A
- Jump to Track: Hold B + \[LEFT or RIGHT\]
- Jump to Phrase: Hold B + \[UP or DOWN\]

### Instrument Screen
Configure instrument parameters:
- Name (15 characters max)
- Table speed
- Transpose enable/disable
- ADSR envelope
- Auto-envelope settings

### Table Screen
Tables are the main sound design tool in ChipNomad. If you're familiar with Vortex Tracker, tables are
the mix of instruments and ornaments and can do even more. If you're familiar with LSDJ and M8, you know
what tables are.

Pitch column can have relative or absolute pitch values in semitones. Volume is applied on top of ADSR
envelope. Four FX lanes are generally equal to FX lanes in phrase, however, there are minor differences
in the behavior of some FX in phrases and tables.

- 16 rows per table
- Pitch column (relative/absolute)
- Volume column
- 4 effect columns

Controls:
- Jump to Table: Hold B + \[LEFT, RIGHT, UP, DOWN\]

### Groove Screen
Create custom timing patterns:
- 16 steps per groove
- Set speed value for each step. Zero skips the phrase row.

Controls:
- Jump to Groove: Hold B + \[LEFT, RIGHT, UP, DOWN\]

## Effects (FX)

### General Effects
- `ARP XY` – Arpeggio: 0, +x, +y semitones
- `ARC XY` – Arpeggio configuration
- `PVB XY` – Vibrato with X speed Y depth
- `PBN XX` – Pitch bend by XX per phrase/table step
- `PSL XX` – Pitch slide (portamento) for XX tics
- `PIT XX` – Pitch offset by XX (FF – -1, etc)
- `VOL XX` – Volume offset by XX
- `RET XY` – Retrigger note every Y tics. X controls volume change
- `DEL XX` – Delay note by XX tics
- `OFF XX` – Note off after XX tics
- `KIL XX` – Kill note after XX tics
- `TIC XX` – Set table speed to XX tics per step
- `TBL XX` – Set instrument table
- `TBX XX` – Set aux table
- `THO XX` – Hop all table columns to row XX
- `HOP XX` – In Tables: hop to a row in the current column
- `GRV XX` – Set track groove
- `GGR XX` – Global groove

### AY-Specific Effects
- `AYM XY` – AY mixer: X – envelope shape, Y - tone/noise control (0 - off, 1 - tone, 2 - noise, 3 - tone+noise)
- `ERT --` – Retrig envelope
- `NOI XX` – Noise period (relative)
- `NOA XX` – Noise period (absolute)
- `EAU XY` – Auto-envelope settings X:Y. X = 0 - auto-envelope off
- `EVB XY` – Envelope vibrato with X speed Y depth
- `EBN XX` – Envelope pitch bend by XX per phrase/table step
- `ESL XX` – Envelope pitch slide (portamento) for XX tics
- `ENT XX` – Envelope pitch as a note (see bottom line for the note name)
- `EPT XX` – Envelope pitch offset
- `EPH XX`: Envelope period high
- `EPL XX`: Envelope period low
