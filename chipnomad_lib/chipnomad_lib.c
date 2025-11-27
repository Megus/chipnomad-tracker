#include "chipnomad_lib.h"
#include <stddef.h>

static struct SoundChip chips[PROJECT_MAX_CHIPS];
static int currentSampleRate = 0;

void chipnomadInit(void) {
    fillFXNames();
}

void chipnomadInitChips(int sampleRate) {
    // Cleanup existing chip if already initialized
    if (currentSampleRate > 0 && chips[0].cleanup) {
        chips[0].cleanup(&chips[0]);
    }
    currentSampleRate = sampleRate;
    chips[0] = createChipAY(sampleRate, project.chipSetup);
}

struct SoundChip* chipnomadGetChip(int index) {
    if (index >= 0 && index < PROJECT_MAX_CHIPS) {
        return &chips[index];
    }
    return NULL;
}