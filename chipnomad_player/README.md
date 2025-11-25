# ChipNomad Player

A desktop video player for ChipNomad tracks (.cnm files) designed for creating social media content.

## Features

- **1920x1080 window resolution** - Perfect for video recording
- **ChipNomad .cnm file playback** - Uses the same audio engine as the tracker
- **Simple controls** - Space to play/pause, Escape to quit
- **Real-time audio rendering** - AY-3-8910/YM2149F chip emulation
- **Looping playback** - Automatically loops tracks for continuous recording

## Building

### Quick Build
```bash
./build.sh
```

### Manual Build
```bash
make clean
make
```

## Usage

```bash
./chipnomad_player <track.cnm>
```

### Examples
```bash
# Play demo track
./chipnomad_player ../tracker/packaging/common/projects/DEMO1.cnm

# Play your own track
./chipnomad_player /path/to/your/track.cnm
```

## Controls

- **Space**: Play/Pause toggle
- **Escape**: Quit application

## Requirements

- **macOS**: SDL2 via Homebrew (`brew install sdl2`)
- **ChipNomad tracker source code** - Uses chipnomad_lib for playback
- **Xcode Command Line Tools** - For compilation

## Recording Videos

The player creates a 1920x1080 window optimized for screen recording:

### Recommended Recording Tools
- **OBS Studio** - Free, cross-platform
- **QuickTime Player** - Built-in macOS screen recording
- **FFmpeg** - Command-line screen capture

### Recording Tips
- Start playback before recording
- The track will loop automatically
- Green rectangle indicates playback is active
- Use Space to pause/resume as needed

## Architecture

The player is built using:
- **SDL2** - Window management and audio output
- **ChipNomad Library** - Project loading and playback engine
- **Ayumi** - AY-3-8910/YM2149F chip emulation
- **Minimal UI** - Focus on audio with simple visual feedback