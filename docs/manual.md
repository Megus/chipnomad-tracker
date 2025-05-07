# ChipNomad User Manual

## Introduction

ChipNomad is a multi-chip tracker designed for creating chiptune music. It features a compact 40x20 character interface inspired by LSDJ and M8 Tracker, making it perfect for portable devices with physical controls. ChipNomad will support multiple chips, but currently it supports only AY-3-8910/YM2149F chips.

### Hardware Requirements

- Display capable of 40x20 characters
- 8 buttons: LEFT, RIGHT, UP, DOWN, A, B, START, SELECT
- Stereo 16-bit audio output

## Project Structure

A ChipNomad song is organized in a hierarchical structure:
- Song (up to 256 positions)
- Chains (up to 255)
- Phrases (up to 1024)
- Up to 10 tracks for multi-chip setups
- 128 instruments (00-7F)
- 255 tables (128 instrument tables + 127 additional)
- 31 grooves (00-1F)

Song is built as a sequence of chains on each track. A chain is a sequence of phrases. A phrase is a 16-note long pattern.

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
- `ARP`: Arpeggio
- `ARC`: Arpeggio configuration
- `PVB`: Vibrato
- `PBN`: Pitch bend
- `PSL`: Pitch slide (portamento)
- `PIT`: Set pitch (relative)
- `VOL`: Set volume (relative)
- `RET`: Retrigger
- `DEL`: Delay note
- `OFF`: Note off
- `KIL`: Kill note
- `TIC`: Set table speed
- `TBL`: Set instrument table
- `TBX`: Set aux table
- `THO`: Hop to table row (in tables: all table columns)
- `HOP`: In Tables: hop to a row in the current column
- `GRV`: Set track groove
- `GGR`: Global groove

### AY-Specific Effects
- `AYM`: AY mixer (tone on/off, noise on/off, envelope shape)
- `ERT`: Retrig envelope
- `NOI`: Noise period (relative)
- `NOA`: Noise period (absolute)
- `EAU`: Auto-envelope settings
- `EVB`: Envelope vibrato
- `EBN`: Envelope pitch bend
- `ESL`: Envelope pitch slide
- `ENT`: Envelope pitch as a note
- `EPT`: Envelope pitch (relative)
- `EPH`: Envelope period high
- `EPL`: Envelope period low
