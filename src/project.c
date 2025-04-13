#include <stdio.h>
#include <string.h>
#include <project.h>
#include <corelib_file.h>
#include <utils.h>

const char noteStrings[8 * 12][4] = {
  "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
  "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
  "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
  "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
  "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
  "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
  "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7",
  "C-8", "C#8", "D-8", "D#8", "E-8", "F-8", "F#8", "G-8", "G#8", "A-8", "A#8", "B-8",
};

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

int isPhraseEmpty(int phrase) {

  return 0;
}

const char* noteString(uint8_t note) {
  return noteStrings[note];
}


///////////////////////////////////////////////////////////////////////////////
//
// Load/save project
//

int projectLoad(const char* path) {
  struct Project p;

  return 1;
}


/////////////////////////////////////////////////////////////////////////////
// Save

static int projectSaveSong(int fileId) {
  filePrintf(fileId, "\n## Song\n");

  // Find the last row with values
  int songLength = PROJECT_MAX_LENGTH;
  for (songLength = PROJECT_MAX_LENGTH - 1; songLength >= 0; songLength--) {
    int isEmpty = 1;
    for (int c = 0; c < project.tracksCount; c++) {
      if (project.song[songLength][c] != EMPTY_VALUE_16) {
        isEmpty = 0;
        break;
      }
    }
    if (!isEmpty) {
      break;
    }
  }
  songLength++;
  printf("Song length: %d\n", songLength);

  for (int c = 0; c < songLength; c++) {
    for (int d = 0; d < project.tracksCount; d++) {
      int chain = project.song[c][d];
      if (chain == EMPTY_VALUE_16) {
        filePrintf(fileId, "-- ");
      } else {
        filePrintf(fileId, "%s ", byteToHex(chain));
      }
    }
    filePrintf(fileId, "\n");
  }

  return 0;
}

static int projectSaveChains(int fileId) {
  filePrintf(fileId, "\n## Chains\n");

  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    if (!isChainEmpty(c)) {
      filePrintf(fileId, "### Chain %X\n", c);
      for (int d = 0; d < 16; d++) {
        int phrase = project.chains[c].phrases[d];
        if (phrase == EMPTY_VALUE_16) {
          filePrintf(fileId, "--- %s\n", byteToHex(project.chains[c].transpose[d]));
        } else {
          filePrintf(fileId, "%03X %s\n", project.chains[c].phrases[d], byteToHex(project.chains[c].transpose[d]));
        }
      }
      filePrintf(fileId, "\n");
    }
  }

  return 0;
}

static int projectSavePhrases(int fileId) {
  filePrintf(fileId, "\n## Phrases\n");

  return 0;
}

static int projectSaveInstruments(int fileId) {
  filePrintf(fileId, "\n## Instruments\n");

  return 0;
}

static int projectSaveTables(int fileId) {
  filePrintf(fileId, "\n## Tables\n");

  return 0;
}

int projectSave(const char* path) {
  printf("Size of project structure in memory: %lu\n", sizeof(project));

  int fileId = fileOpen(path, 1);
  if (fileId == -1) return 1;

  filePrintf(fileId, "# ChipNomad Tracker Module v1\n");
  filePrintf(fileId, "- Tracks: %d\n", project.tracksCount);

  projectSaveSong(fileId);
  projectSaveChains(fileId);
  projectSavePhrases(fileId);
  projectSaveInstruments(fileId);
  projectSaveTables(fileId);

  fileClose(fileId);
  return 0;
}

