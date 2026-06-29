# ChipNomad Library

This is the core ChipNomad library containing all the platform-independent music playback logic. It can be used as a standalone library in other projects.

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

## Example

### Basic Playback

```c
#include <chipnomad_lib.h>

// Create ChipNomad state
ChipNomadState* state = chipnomadCreate();
if (!state) {
    fprintf(stderr, "Failed to create ChipNomad state\n");
    return -1;
}

// Load project
if (projectLoad(&state->project, "song.cnm") != 0) {
    fprintf(stderr, "Failed to load project\n");
    chipnomadDestroy(state);
    return -1;
}

// Initialize playback with the loaded project
playbackInit(&state->playbackState, &state->project);

// Initialize chips (NULL uses default chip implementations)
chipnomadInitChips(state, 44100, NULL);

// Start playback from beginning
playbackStartSong(&state->playbackState, 0, 0, 1);

// Render audio in a loop
float buffer[1024];  // 512 stereo sample pairs
int isPlaying = 1;

while (isPlaying) {
    // Render audio with automatic tick handling
    int samplesRendered = chipnomadRender(state, buffer, 512);

    // If fewer samples rendered, playback has stopped
    if (samplesRendered < 512) {
        isPlaying = 0;
    }

    // Output buffer to audio system (convert float to int16, etc.)
    // ... your audio output code here ...
}

// Cleanup
chipnomadDestroy(state);
```

### Audio Callback Integration

For real-time audio (e.g., SDL2 audio callback):

```c
void audioCallback(void* userdata, Uint8* stream, int len) {
    ChipNomadState* state = (ChipNomadState*)userdata;
    static float floatBuffer[2048];  // Temp buffer for float samples

    int stereoSamples = len / sizeof(int16_t) / 2;
    int16_t* output = (int16_t*)stream;

    // Render float samples
    int samplesRendered = chipnomadRender(state, floatBuffer, stereoSamples);

    // Convert float [-1.0, 1.0] to int16
    for (int i = 0; i < stereoSamples * 2; i++) {
        float sample = floatBuffer[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        output[i] = (int16_t)(sample * 32767.0f);
    }

    // Fill remaining with silence if playback stopped
    if (samplesRendered < stereoSamples) {
        SDL_memset(&output[samplesRendered * 2], 0,
                   (stereoSamples - samplesRendered) * 2 * sizeof(int16_t));
    }
}
```

See `chipnomad_player/` for a complete working example.