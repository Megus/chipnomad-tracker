#include "corelib_audio.h"
#include "chipnomad_lib.h"

// Global state for tests - defined here so tests can link
ChipNomadState* chipnomadState = nullptr;

int audioSetup(AudioCallback* audioCallback, int sampleRate, int bufferSize) { return 0; }
void audioPause(int isPaused) {}
void audioCleanup(void) {}