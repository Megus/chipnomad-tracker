# ChipNomad Player

A desktop video player for ChipNomad projects (.cnm files) designed for creating social media content.

- Configurable window resolution
- Two display modes: Phrase view or continuous scrolling
- Multiple phrase row rendering modes

## Building

```bash
make clean && make
```

## Usage

```bash
./chipnomad_player <track.cnm>
```

## Configuration

Settings are configured in `player_settings.txt`:

### Display Settings

- **windowWidth** — Window width in pixels (default: 1920)
- **windowHeight** — Window height in pixels (default: 1080)
- **fontSize** — Font size for tracker display (default: 32)
- **fontPath** — Path to TTF file, must be a monospace font (default: fonts/Fragment_Mono/FragmentMono-Regular.ttf)

### Color Settings

All colors use hexadecimal format (0xRRGGBB):

- **backgroundColor** — Background color (default: 0x000000)
- **currentRowColor** — Color of the playback position indicator (default: 0xffff00)
- **noteColor** — Color for note names (default: 0xffffff)
- **instrumentColor** — Color for instrument numbers (default: 0x00ffff)
- **volumeColor** — Color for volume values (default: 0x00ff00)
- **fxColor** — Color for FX commands (default: 0x00ffff)
- **dimmedColor** — Color for empty/inactive cells (default: 0x404040)

### Project Rendering Settings

- **playerMode** (default: phrase)
  - `phrase`: Shows current phrase for each track (16 rows)
  - `scroll`: Continuous scrolling view with configurable row count
- **scrollRows** (default: 32)
  - Number of rows to display in scroll mode
  - Current row appears at center
  - Only used when `playerMode: scroll`
- **phraseRenderMode** (default: full)
  - `full`: Shows all 3 FX columns (widest, 27 chars per track)
  - `medium`: Shows 1 FX column - leftmost non-empty (15 chars per track)
  - `compact`: No FX columns, FX shown in note/instrument when note is empty (9 chars per track)

## Dependencies

The player is built using:

- **SDL2** - Graphics and audio
- **FreeType** - Font rendering
- **ChipNomad Library** - Project loading and playback
