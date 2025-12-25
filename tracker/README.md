# ChipNomad Tracker - Build Instructions

This directory contains the main ChipNomad tracker application.

## Quick Start

```bash
# Build for current platform (macOS/Linux)
make desktop

# Build all platform releases
./deploy-all.sh
```

## Platform-Specific Builds

### Desktop Platforms

#### macOS
```bash
# Simple build
make desktop

# macOS app bundle with packaging
make macOS-deploy
```

#### Linux (x86_64 and ARM)
```bash
# Native build (works on x86_64, ARM, Raspberry Pi)
make linux

# Create portable package
make linux-package

# Full deployment
make linux-package-deploy
```

**Linux Requirements:**
- GCC or Clang
- SDL2 development libraries: `sudo apt install build-essential libsdl2-dev`

#### Windows
```bash
# Cross-compile from macOS/Linux (recommended)
make windows-deploy

# Native Windows build
build-windows.bat  # On Windows with MinGW
```

### Handheld Consoles

#### PortMaster (Multiple ARM devices)
```bash
make PortMaster-deploy
```

#### RG35xx Specific
```bash
make RG35xx-deploy
```

## Build Requirements

### Cross-Platform Builds (Docker)
- Docker installed and running
- No additional setup required

### Native Builds
- **macOS**: Xcode Command Line Tools, Homebrew, SDL2
- **Linux**: GCC, SDL2 development libraries
- **Windows**: MinGW-w64, SDL2 development libraries

### Native Windows Setup (Optional)

1. Install [MinGW-w64](https://www.mingw-w64.org/) or [MSYS2](https://www.msys2.org/)
2. Download [SDL2 development libraries](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.6)
3. Set `SDL_PATH` environment variable (optional)
4. Run `build-windows.bat`

## Architecture Support

ChipNomad supports multiple architectures:
- **x86_64**: Intel/AMD 64-bit (Windows, macOS, Linux)
- **ARM64**: Apple Silicon (macOS), ARM64 Linux
- **ARM**: Raspberry Pi, handheld consoles

The Linux build automatically detects your architecture and creates appropriately named packages.

## Output

Built binaries and packages are created in:
- `build/` - Platform-specific build directories
- `../../releases/` - Final release packages (when using deploy targets)