#include <string.h>
#include <project.h>

struct Project project;

void projectInit() {
  // Init for AY
  project.tracksCount = 3;

  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      project.song[c][d] = EMPTY_VALUE_16;
    }
  }

  // Clean chains
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    project.chains[c].hasNoNotes = 1;
    for (int d = 0; d < 16; d++) {
      project.chains[c].phrases[d] = EMPTY_VALUE_16;
      project.chains[c].transpose[d] = 0;
    }
  }

  // Clean phrases
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    project.phrases[c].hasNoNotes = 1;
    for (int d = 0; d < 16; d++) {

    }
  }


  // Clean instruments


  // Clean tables

}

int projectLoad(const char* path) {
  return 0;
}

int projectSave(const char* path) {
  return 0;
}

int isChainEmpty(int chain) {
  int isEmpty = 1;

  for (int c = 0; c < 16; c++) {
    if (project.chains[chain].phrases[c] != EMPTY_VALUE_16) {
      isEmpty = 0;
      break;
    }
  }

  return isEmpty;
}
