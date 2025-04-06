#include <string.h>
#include <project.h>

struct Project project;

void projectInit() {
  // Init for AY
  project.tracksCount = 3;

  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      project.song[c][d] = 0xff; // FF is empty
    }
  }

  project.song[0][0] = 0x00;

}

int projectLoad(const char* path) {
  return 0;
}

int projectSave(const char* path) {
  return 0;
}