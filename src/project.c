#include <stdio.h>
#include <math.h>
#include <string.h>
#include <project.h>
#include <corelib_file.h>
#include <utils.h>

struct Project project;

// Create 12TET scale
void calculatePitchTableAY() {
  char noteStrings[12][4] = { "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1" };

  float clock = (float)(project.chipSetup.ay.clock);
  int octaves = 9;
  float cfreq = 16.35159783; // C-0 frequency for A4 = 440Hz. It's too low for 1.75MHz AY, but we'll keep it
  float freq = cfreq;
  float semitone = powf(2., 1. / 12.);

  sprintf(project.pitchTable.tableName, "12TET %dHz", project.chipSetup.ay.clock);

  printf("%s\n", project.pitchTable.tableName);

  for (int o = 0; o < octaves; o++) {
    for (int c = 0; c < 12; c++) {
      noteStrings[c][2] = 48 + o;

      float periodf = clock / 16. / freq;

      float freqL = clock / 16. / floorf(periodf);
      float freqH = clock / 16. / ceilf(periodf);

      int period = (fabsf(freqL - freq) < fabsf(freqH - freq)) ? floorf(periodf) : ceilf(periodf);
      if (period > 4095) period = 4095; // AY only has 12 bits for period

      project.pitchTable.values[o * 12 + c] = period;
      strcpy(project.pitchTable.names[o * 12 + c], noteStrings[c]);

      freq *= semitone;
    }

    // Reset frequency calculation on each octave to minimize rounding errors
    cfreq *= 2.;
    freq = cfreq;
  }
}

// Initialize project
void projectInit() {
  // Init for AY
  project.chipType = chipAY;
  project.chipsCount = 1;
  project.chipSetup.ay = (struct ChipSetupAY){
    .clock = 1750000,
    .isYM = 1,
    .panA = 64,
    .panB = 128,
    .panC = 192,
  };

  project.tracksCount = project.chipsCount * 3; // AY/YM has 3 channels

  calculatePitchTableAY();

  // Title
  strcpy(project.title, "");
  strcpy(project.author, "");


  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      project.song[c][d] = EMPTY_VALUE_16;
    }
  }

  // Clean chains
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    project.chains[c].hasNoNotes = -1;
    for (int d = 0; d < 16; d++) {
      project.chains[c].phrases[d] = EMPTY_VALUE_16;
      project.chains[c].transpose[d] = 0;
    }
  }

  // Clean phrases
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    project.phrases[c].hasNoNotes = -1;
    for (int d = 0; d < 16; d++) {

    }
  }

  // Clean instruments
  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    project.instruments[c].type = instNone;
  }

  // Clean tables
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {

  }

}

///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//

// Is chain empty?
int chainIsEmpty(int chain) {
  int isEmpty = 1;

  for (int c = 0; c < 16; c++) {
    if (project.chains[chain].phrases[c] != EMPTY_VALUE_16) {
      isEmpty = 0;
      break;
    }
  }

  return isEmpty;
}

// Does chain have notes?
int chainHasNotes(int chain) {
  return 0;
}

// Is phrase empty?
int phraseIsEmpty(int phrase) {
  return 0;
}

// Does phrase have notes?
int phraseHasNotes(int phrase) {
  return 0;
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
    if (!chainIsEmpty(c)) {
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

