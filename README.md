# ChipNomad Tracker

ChipNomad is a multi-platform tracker with LSDJ-like interface designed for creating chiptune music. Primary target platforms are handheld game consoles like Anbernic RG35xx.

[ChipNomad manual](docs/manual.md)

## Currently supported platforms

- Anbernic RG35xx with GarlicOS 1.4
- Miyoo Mini with MiyooCFW (not tested but should work)
- macOS (Apple Silicon)
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

ChipNomad can be built using either the traditional makefile or CMake:

### Using Makefile

```bash
# Build for desktop (Linux/macOS)
./build-desktop.sh

# Build for Windows
# On Windows, run:
build-windows.bat

# Build for RG35xx
./build-rg.sh
```

### Windows-specific Setup

To build ChipNomad on Windows:

1. Install MinGW (Minimalist GNU for Windows) which includes gcc, make, and other build tools
   - Download from [MinGW website](https://www.mingw-w64.org/) or use [MSYS2](https://www.msys2.org/)
   - Make sure MinGW's bin directory is in your PATH environment variable

2. Download SDL development libraries for MinGW from [GitHub](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.6)
   - Extract the SDL files to a directory of your choice (e.g., `C:\SDL`)

3. Set the SDL_PATH environment variable to your SDL installation directory (optional)
   ```
   set SDL_PATH=C:\path\to\SDL
   ```
   - If not set, the build script will use the default path `C:\SDL`

4. Run `build-windows.bat` to build the application
   - The script will automatically copy SDL.dll to the build directory if SDL_PATH is set

5. The executable will be created at `build\chipnomad.exe`

#### Troubleshooting

If you encounter issues:

- **SDL.dll not found**: Copy SDL.dll from your SDL installation's bin directory to the build directory or to your Windows system directory
- **Compilation errors**: Verify that MinGW is properly installed and in your PATH
- **Linker errors**: Verify that SDL development libraries are correctly installed and SDL_PATH is set correctly
- **Runtime errors**: Make sure SDL.dll is accessible to the application

## Acknowledgements

ChipNomad wouldn't be possible without:

- [LSDJ](https://www.littlesounddj.com/lsd/index.php) by Johan Kotlinski
- [M8 Tracker](https://dirtywave.com) by Trash80
- [RG35xx app template](https://github.com/anderson-/simplegfx) by Anderson Antunes
- [Ayumi AY chip emulator](https://github.com/true-grue/ayumi) by Peter Sovietov
