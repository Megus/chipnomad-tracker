# ChipNomad Tracker

ChipNomad is a multi-platform tracker with LSDJ-like interface designed for creating chiptune music. Primary target platforms are handheld game consoles like Anbernic RG35xx.

[ChipNomad manual](docs/manual.md)

[Join ChipNomad Discord server](https://discord.gg/PJarAn2QCW)

## Currently supported platforms

- [PortMaster](https://portmaster.games) ([list of supported devices](https://portmaster.games/supported-devices.html))
- Pre-2024 Anbernic RG35xx with GarlicOS 1.4
- macOS
- Windows

## Hardware Requirements

ChipNomad is written in pure C99 and can be ported to any platform that satisfies these requirements:

- Display capable of 40x20 characters
- 8 buttons: LEFT, RIGHT, UP, DOWN, A, B, START, SELECT
- Stereo 16-bit audio output
- CPU capable of running chip emulation or a platform with real chips

## Background

I (Megus) started this project because I want to make real chiptune music on the go. LSDJ is amazing but it's only
for GameBoy music. I come from ZX Spectrum scene, so I want to make music for AY-3-8910/YM2149F chips. I use M8 Tracker
every day and love it. This is how I chose the approach to the UI. I considered making an app for iOS/Android, but I find
using touchscreen for music making a painful experience and prefer a device with physical buttons.
I have Anbernic RG35xx with Garlic OS which allows installing 3rd party native apps. This is how I chose the platform.

## Building

### Quick Start

```bash
# Build for current platform (macOS/Linux)
make desktop

# Build all platform releases
./deploy-all.sh
# or
make deploy-all
```

### Platform-Specific Builds

#### Desktop (macOS/Linux)
```bash
# Simple build
make desktop
./build-desktop.sh

# macOS app bundle with packaging
make macOS-deploy
```

#### Windows
```bash
# Cross-compile from macOS/Linux (recommended)
make windows-deploy

# Native Windows build
build-windows.bat  # On Windows with MinGW
```

#### Handheld Consoles
```bash
# PortMaster (multiple ARM devices)
make PortMaster-deploy

# RG35xx specific
make RG35xx-deploy
```

### Requirements

#### For Cross-Platform Builds (Docker)
- Docker installed and running
- No additional setup required

#### For Native Builds
- **macOS**: Xcode Command Line Tools, Homebrew, SDL2
- **Linux**: GCC, SDL2 development libraries
- **Windows**: MinGW-w64, SDL2 development libraries

### Native Windows Setup (Optional)

For Windows developers who prefer native builds:

1. Install [MinGW-w64](https://www.mingw-w64.org/) or [MSYS2](https://www.msys2.org/)
2. Download [SDL2 development libraries](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.6)
3. Set `SDL_PATH` environment variable (optional)
4. Run `build-windows.bat`

## Acknowledgements

ChipNomad wouldn't be possible without:

- [LSDJ](https://www.littlesounddj.com/lsd/index.php) by Johan Kotlinski
- [M8 Tracker](https://dirtywave.com) by Trash80
- [RG35xx app template](https://github.com/anderson-/simplegfx) by Anderson Antunes
- [Ayumi AY chip emulator](https://github.com/true-grue/ayumi) by Peter Sovietov
