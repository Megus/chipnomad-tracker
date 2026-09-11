#include "corelib_audio.h"
#include "chipnomad_lib.h"

// NOTE: The old `chipnomadState` global (type ChipNomadState*) was removed here
// as part of the Engine/Player refactor. Tests that needed it (copy_paste,
// return_values, edit_common) are excluded until the main-app Project-ownership
// migration re-homes that global.

int audioSetup(AudioCallback* audioCallback, int sampleRate, int bufferSize) { return 0; }
void audioPause(int isPaused) {}
void audioCleanup(void) {}