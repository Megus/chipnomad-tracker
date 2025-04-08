#include <string.h>
#include <project.h>

struct Project project;

void projectInit() {
  // Init for AY
  project.tracksCount = 3;

  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      project.song[c][d] = EMPTY_VALUE;
    }
  }

  // Clean chains
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    project.chains[c].isEmpty = 1;
    for (int d = 0; d < 16; d++) {
      project.chains[c].phrases[0] = EMPTY_VALUE;
      project.chains[c].transpose[0] = 0;
    }
  }

  // Clean phrases


  // Clean instruments


  // Clean tables

}

int projectLoad(const char* path) {
  return 0;
}

int projectSave(const char* path) {
  return 0;
}