# ChipNomad User Manual

## Introduction

ChipNomad is a multi-platform tracker designed for creating chiptune music. It features a compact 40x20 character interface inspired by LSDJ and M8 Tracker, making it perfect for portable devices with physical controls. ChipNomad currently supports only AY-3-8910/YM2149F chips with more chips coming in the future.

### Common controls

If you're familiar with LSDJ or M8, you can expect most shortcuts to work the same in ChipNomad.

Navigation:
- Move cursor: LEFT, RIGHT, UP, DOWN
- Screen navigation: Hold SELECT + \[LEFT, RIGHT, UP, DOWN\]
- Jump 16 rows/positions: Hold B + \[UP or DOWN\]
- Jump between tracks: Hold B + \[LEFT or RIGHT\]

Editing:
- Change value (fine): Hold A + \[LEFT or RIGHT\]
- Change value (coarse): Hold A + \[UP or DOWN\]
- Cut value: B + A
- Insert/Enter value: A
- Create new item: Double-tap A

Selection and Clipboard:
- Enter selection mode: SELECT + B
- Copy selection: B
- Cut selection: B + A
- Paste: SELECT + A
- Multi-edit: A + \[LEFT, RIGHT, UP, DOWN\]

Playback:
- Play from cursor: START
- Play all tracks (outside Song screen): SELECT + START
- Stop playback: START (while playing)

System:
- Quit ChipNomad: MENU + X (on consoles)

Control mapping on desktop:
- D-Pad - Cursor keys
- SELECT - Left Shift
- START - Space
- A - X
- B - Y or Z for qwerty and qwertz keyboard layouts

### Keyboard Layout Support

ChipNomad automatically detects your keyboard layout based on system locale and provides manual override options in the Settings screen:

- **AUTO**: Automatically detect based on system locale
- **QWERTY**: US/UK keyboard layout (Z key for B button)
- **QWERTZ**: German/Austrian keyboard layout (Y key for B button)
- **AZERTY**: French keyboard layout
- **DVORAK**: Dvorak keyboard layout

The layout detection works by checking:
1. System locale settings (LANG, LC_ALL, etc.)
2. Regional keyboard conventions
3. SDL keyboard information when available

Supported locales:
- German-speaking regions (DE, AT, CH, LU) → QWERTZ
- French-speaking regions (FR, BE, CA) → AZERTY
- English-speaking regions → QWERTY
- Other European regions → Appropriate local layouts

## Project Structure

The core UI concept and song structure is the same as in LSDJ or M8 Tracker:
multiple screens, each screen is dedicated to a single function. All screens, except for the Song,
don't have any scrolling content, so the length of chains, phrases, and tables is naturally limited to 16.

A ChipNomad song is organized in a hierarchical structure. The lowest level of the hierarchy is a **Phrase**. Phrase is a sequence of 16 tracker rows for one track. Phrases are grouped into **Chains**. A chain can have up to 16 phrases. You can re-use phrases across multiple chains. Chains are sequenced in a **Song**.

- Song (up to 256 positions)
- Chains (up to 255)
- Phrases (up to 1024)
- Up to 10 tracks for multi-chip setups
- 128 instruments (00-7F)
- 255 tables (128 instrument tables + 127 additional tables)
- 32 grooves (00-1F)

## Screens

ChipNomad screens are laid out in the map:

```
P G
SCPIT
S  P
```

- **P**roject: Project settings (chip type, tick rate, etc)
- **S**ong: Main song sequencing screen
- **S**ettings: Application settings
- **C**hain: Chain editor
- **P**hrase: Phrase editor
- **I**nstrument: Instrument editor
- Instrument **P**ool: Instrument pool and management
- **T**able: Table editor
- **G**roove: Groove editor

### Project Screen
Configure global project settings:
- Chip type selection
- Master tick rate
- Project name
- Save/Load project files

### Song Screen
Arrange chains into a complete song:
- Up to 256 positions (00-FF)
- Up to 10 tracks
- Each position contains chain numbers for each track

Controls:
- Jump 16 positions: Hold B + \[UP or DOWN\]
- Shallow clone chains: select range, then SELECT + A
- Deep clone chains: select range, then SELECT + A + A

### Chain Screen
Create sequences of phrases:
- 16 rows per chain
- Each row contains phrase number and transpose value
- Chains can be reused across different tracks

Controls:
- Jump between tracks: Hold B + \[LEFT or RIGHT\]
- Jump between chains: Hold B + \[UP or DOWN\]
- Clone phrases: select range, then SELECT + A

### Phrase Screen
Create note patterns:
- 16 rows per phrase
- Contains notes, instruments, and effects
- 3 effect columns per row

Controls:
- Insert note off: B + A on empty note
- Jump between tracks: Hold B + \[LEFT or RIGHT\]
- Jump between phrases: Hold B + \[UP or DOWN\]

### Instrument Screen
Configure instrument parameters:
- Name (15 characters max)
- Table speed (tics per table step)
- Transpose enable/disable
- ADSR envelope settings
- Auto-envelope configuration

Each instrument has a corresponding default table which has the same number (00-7F range).

Controls:
- Jump between instruments: Hold B + \[LEFT or RIGHT\]
- Preview instrument: A + START
- Copy instrument: SELECT + B
- Paste instrument: SELECT + A

### Table Screen
Tables are the main sound design tool in ChipNomad. If you're familiar with Vortex Tracker, tables are
the mix of instruments and ornaments and can do even more. If you're familiar with LSDJ and M8, you know
what tables are.

Pitch column can have relative or absolute pitch values in semitones. Volume is applied on top of ADSR
envelope. Four FX lanes are generally equal to FX lanes in phrase, however, there are minor differences
in the behavior of some FX in phrases and tables.

Tables 00-7F are reserved for default instrument tables, tables 80-FE can be used as aux tables (TBX effect).

- 16 rows per table
- Pitch column (relative/absolute)
- Volume column
- 4 effect columns

Controls:
- Jump between tables: Hold B + \[LEFT, RIGHT, UP, DOWN\]

### Groove Screen
Create custom timing patterns:
- 16 steps per groove
- Set speed value for each step. Zero skips the phrase row.
- Grooves affect playback timing globally or per track

Controls:
- Jump between grooves: Hold B + \[LEFT, RIGHT, UP, DOWN\]

### Instrument Pool Screen
Manage and organize instruments:
- View all 128 instruments (00-7F)
- See which instruments are used in the song
- Reorder instruments

Controls:
- Edit instrument: A (jumps to Instrument screen)
- Copy instrument: SELECT + B
- Paste instrument: SELECT + A
- Reorder instruments: hold A, then UP/DOWN

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
- `TXH XX` — Hop all aux table columns to row XX
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
- `EPH XX` — Envelope period high
- `EPL XX` — Envelope period low
