# ChipNomad Library

This is the core ChipNomad library containing all the platform-independent music playback logic. It can be used as a standalone library in other projects.

## Contents

- **project.h/c** - Project file format handling and data structures
- **playback.h/c** - Main playback engine
- **playback_*.c** - Playback implementation files (FX, chip-specific logic)
- **chips/** - Sound chip emulation (AY-3-8910/YM2149F)
- **external/ayumi/** - Ayumi AY chip emulator by Peter Sovietov
- **export.h** - Export functionality interface
- **export_*.c** - Export implementations (WAV, PSG)
- **utils.h/c** - Utility functions
- **corelib/** - Platform abstraction headers (implementations are platform-specific)

## Usage

Include the main header in your project:

```c
#include <chipnomad_lib.h>
```

This will give you access to all the core functionality:
- Project loading/saving
- Playback engine
- Sound chip emulation
- Export functionality

## Dependencies

The library requires a platform-specific implementation of `corelib_file.h` for file operations. The main ChipNomad project provides implementations in `platforms/shared/corelib_file.c`.

## Example

```c
#include <chipnomad_lib.h>

// Initialize library
chipnomadInit();

// Load a project
if (projectLoad("song.cnm") == 0) {
    // Initialize playback
    struct PlaybackState playback;
    playbackInit(&playback, &project);
    
    // Create sound chip
    struct SoundChip chip = createChipAY(44100, project.chipSetup);
    chip.init(&chip);
    
    // Start playback
    playbackStartSong(&playback, 0, 0, 1);
    
    // Render audio frames
    float buffer[1024];
    while (!playbackNextFrame(&playback, &chip)) {
        chip.render(&chip, buffer, 512);
        // Output buffer to audio system
    }
    
    chip.cleanup(&chip);
}
```