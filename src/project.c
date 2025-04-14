#include <stdio.h>
#include <math.h>
#include <string.h>
#include <project.h>
#include <corelib_file.h>
#include <utils.h>

struct Project project;

// Start from 0
char fxCommon[][4] = {
  "ARP", "ARC", "PVB", "PBN", "PSL", "PIT", // Pitch
  "RET", "DEL", "OFF", "KIL", // Sequencer
  "TIC", "TBL", "TBX", "THO", // Table
  "GRV", "GGR", "SNG",
};

// Start from 32
char fxAY[][4] = {
  "AYM", "ERT", "NOI", "NOA",
  "EAU", "EVB", "EBN", "ESL", "ENA", "ENR", "EPR", "EPL", "EPH",
};

// Create 12TET scale
void calculatePitchTableAY(struct Project* p) {
  static char noteStrings[12][4] = { "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1" };

  float clock = (float)(p->chipSetup.ay.clock);
  int octaves = 9;
  float cfreq = 16.35159783; // C-0 frequency for A4 = 440Hz. It's too low for 1.75MHz AY, but we'll keep it
  float freq = cfreq;
  float semitone = powf(2., 1. / 12.);

  sprintf(p->pitchTable.title, "12TET %dHz", p->chipSetup.ay.clock);
  p->pitchTable.length = octaves * 12;
  p->pitchTable.octaveSize = 12;

  for (int o = 0; o < octaves; o++) {
    for (int c = 0; c < 12; c++) {
      noteStrings[c][2] = 48 + o;

      float periodf = clock / 16. / freq;

      float freqL = clock / 16. / floorf(periodf);
      float freqH = clock / 16. / ceilf(periodf);

      int period = (fabsf(freqL - freq) < fabsf(freqH - freq)) ? floorf(periodf) : ceilf(periodf);
      if (period > 4095) period = 4095; // AY only has 12 bits for period

      p->pitchTable.values[o * 12 + c] = period;
      strcpy(p->pitchTable.names[o * 12 + c], noteStrings[c]);

      freq *= semitone;
    }

    // Reset frequency calculation on each octave to minimize rounding errors
    cfreq *= 2.;
    freq = cfreq;
  }
}

// Initialize project
void projectInit(struct Project* p) {
  // Init for AY
  p->frameRate = 50;
  p->chipType = chipAY;
  p->chipsCount = 1;
  p->chipSetup.ay = (struct ChipSetupAY){
    .clock = 1750000,
    .isYM = 1,
    .panA = 64,
    .panB = 128,
    .panC = 192,
  };

  p->tracksCount = p->chipsCount * 3; // AY/YM has 3 channels

  calculatePitchTableAY(p);

  // Title
  strcpy(p->title, "");
  strcpy(p->author, "");

  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      p->song[c][d] = EMPTY_VALUE_16;
    }
  }

  // Clean chains
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    p->chains[c].hasNotes = -1;
    for (int d = 0; d < 16; d++) {
      p->chains[c].phrases[d] = EMPTY_VALUE_16;
      p->chains[c].transpose[d] = 0;
    }
  }

  // Clean grooves
  for (int c = 0; c < PROJECT_MAX_GROOVES; c++) {
    for (int d = 0; d < 16; d++) {
      p->grooves[c].speed[d] = EMPTY_VALUE_8;
    }
  }

  // Default groove
  p->grooves[0].speed[0] = 6;
  p->grooves[0].speed[1] = 6;


  // Clean phrases
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    p->phrases[c].hasNotes = -1;
    for (int d = 0; d < 16; d++) {
      p->phrases[c].notes[d] = EMPTY_VALUE_8;
      p->phrases[c].instruments[d] = EMPTY_VALUE_8;
      p->phrases[c].volumes[d] = EMPTY_VALUE_8;
      for (int e = 0; e < 3; e++) {
        p->phrases[c].fx[d][e][0] = EMPTY_VALUE_8;
        p->phrases[c].fx[d][e][1] = 0;
      }
    }
  }

  // Clean instruments
  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    p->instruments[c].type = instNone;
  }

  // Clean tables
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    for (int d = 0; d < 16; d++) {
      p->tables[c].pitchFlags[d] = 0;
      p->tables[c].pitchOffsets[d] = 0;
      p->tables[c].volumeOffsets[d] = 0;
      for (int e = 0; e < 4; e++) {
        p->tables[c].fx[d][e][0] = EMPTY_VALUE_8;
        p->tables[c].fx[d][e][1] = 0;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//

// Is chain empty?
int8_t chainIsEmpty(int chain) {
  for (int c = 0; c < 16; c++) {
    if (project.chains[chain].phrases[c] != EMPTY_VALUE_16) return 0;
  }

  return 1;
}

// Does chain have notes?
int8_t chainHasNotes(int chain) {
  int8_t v = project.chains[chain].hasNotes;
  if (v != -1) return v;

  v = 0;
  for (int c = 0; c < 16; c++) {
    int phrase = project.chains[chain].phrases[c];
    if (phrase != EMPTY_VALUE_16) v = phraseHasNotes(phrase);
    if (v == 1) break;
  }

  project.chains[chain].hasNotes = v;

  return v;
}

// Is phrase empty?
int8_t phraseIsEmpty(int phrase) {
  for (int c = 0; c < 16; c++) {
    if (project.phrases[phrase].notes[c] != EMPTY_VALUE_8) return 0;
    if (project.phrases[phrase].instruments[c] != EMPTY_VALUE_8) return 0;
    if (project.phrases[phrase].volumes[c] != EMPTY_VALUE_8) return 0;
    for (int d = 0; d < 3; d++) {
      if (project.phrases[phrase].fx[c][d][0] != EMPTY_VALUE_8) return 0;
      if (project.phrases[phrase].fx[c][d][1] != 0) return 0;
    }
  }

  return 1;
}

// Does phrase have notes?
int8_t phraseHasNotes(int phrase) {
  int8_t v = project.phrases[phrase].hasNotes;
  if (v != -1) return v;

  v = 0;
  for (int c = 0; c < 16; c++) {
    if (project.phrases[phrase].notes[c] != EMPTY_VALUE_8) {
      v = 1;
      break;
    }
  }

  project.phrases[phrase].hasNotes = v;

  return v;
}

// Is instrument empty?
int8_t instrumentIsEmpty(int instrument) {
  return project.instruments[instrument].type == instNone;
}

// Is table empty?
int8_t tableIsEmpty(int table) {
  for (int c = 0; c < 16; c++) {
    if (project.tables[table].pitchFlags[c] != 0) return 0;
    if (project.tables[table].pitchOffsets[c] != 0) return 0;
    if (project.tables[table].volumeOffsets[c] != 0) return 0;
    for (int d = 0; d < 4; d++) {
      if (project.tables[table].fx[c][d][0] != EMPTY_VALUE_8) return 0;
      if (project.tables[table].fx[c][d][1] != 0) return 0;
    }
  }

  return 1;
}

// Is groove empty?
int8_t grooveIsEmpty(int groove) {
  for (int c = 0; c < 16; c++) {
    if (project.grooves[groove].speed[c] != EMPTY_VALUE_8) return 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////////
//
// Load/save project
//

static char *lpstr;
static char chipNames[][16] = { "AY8910" };

///////////////////////////////////////////////////////////////////////////////
// Load

// Convenience function to read a non-empty string
static char* readString(int fileId) {
  while (1) {
    lpstr = fileReadString(fileId);
    if (lpstr == NULL) return NULL;
    // Skip empty lines and lines with ```
    if (strlen(lpstr) > 0 && strcmp(lpstr, "```")) break;
  }
  return lpstr;
}

#define READ_STRING readString(fileId); if (lpstr == NULL) return 1;

int projectLoadPitchTable(int fileId, struct Project* p) {
  char buf[128];

  READ_STRING; if (strcmp(lpstr, "## Pitch table")) return 1;
  READ_STRING; if (sscanf(lpstr, "- Title: %[^\n]", p->pitchTable.title) != 1) return 1;

  int idx = 0;
  int period;
  while (1) {
    READ_STRING;
    if (sscanf(lpstr, "%s %d", buf, &period) != 2) break;
    if (strlen(buf) != 3) return 1;
    strcpy(p->pitchTable.names[idx], buf);
    p->pitchTable.values[idx] = period;
    idx++;
  }
  p->pitchTable.length = idx;

  // Detect octave size
  char oct = p->pitchTable.names[0][2];
  for (int c = 0; c < p->pitchTable.length; c++) {
    if (p->pitchTable.names[c][2] != oct) {
      p->pitchTable.octaveSize = c;
      break;
    }
  }

  return 0;
}

int projectLoadSong(int fileId, struct Project* p) {
  char buf[3];
  if (strcmp(lpstr, "## Song")) return 1;

  int idx = 0;
  while (1) {
    READ_STRING;
    if (lpstr[0] == '#') break;
    if (strlen(lpstr) != (p->tracksCount * 3 - 1)) return 1;
    for (int c = 0; c < p->tracksCount; c++) {
      buf[0] = lpstr[c * 3];
      buf[1] = lpstr[c * 3 + 1];
      buf[2] = 0;
      if (buf[0] == '-' && buf[1] == '-') {
        p->song[idx][c] = EMPTY_VALUE_16;
      } else {
        if (sscanf(buf, "%hX", &p->song[idx][c]) != 1) return 1;
      }
    }
    idx++;
  }

  return 0;
}

int projectLoadChains(int fileId, struct Project* p) {
  int idx;

  if (strcmp(lpstr, "## Chains")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Chain", 9)) break;
    if (sscanf(lpstr, "### Chain %X", &idx) != 1) return 1;

    p->chains[idx].hasNotes = -1;
    for (int c = 0; c < 16; c++) {
      READ_STRING;
      if (strlen(lpstr) != 6) return 1;

      if (lpstr[0] == '-') {
        p->chains[idx].phrases[c] = EMPTY_VALUE_16;
        if (sscanf(lpstr + 4, "%hhX", &p->chains[idx].transpose[c]) != 1) return 1;
      } else {
        if (sscanf(lpstr, "%hX %hhX", &p->chains[idx].phrases[c], &p->chains[idx].transpose[c]) != 2) return 1;
      }
    }
  }

  return 0;
}

int projectLoadGrooves(int fileId, struct Project* p) {
  int idx;

  if (strcmp(lpstr, "## Grooves")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Groove", 10)) break;
    if (sscanf(lpstr, "### Groove %X", &idx) != 1) return 1;

    for (int c = 0; c < 16; c++) {
      READ_STRING;
      if (strlen(lpstr) != 2) return 1;

      if (lpstr[0] == '-') {
        p->grooves[idx].speed[c] = EMPTY_VALUE_8;
      } else {
        if (sscanf(lpstr, "%hhX", &p->grooves[idx].speed[c]) != 1) return 1;
      }
    }
  }

  return 0;
}

int projectLoadPhrases(int fileId, struct Project* p) {

  return 0;
}

int projectLoadInstruments(int fileId, struct Project* p) {

  return 0;
}

int projectLoadTables(int fileId, struct Project* p) {

  return 0;
}

int projectLoad(const char* path) {
  char buf[128];
  struct Project p;

  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;

  projectInit(&p);

  READ_STRING; if (strcmp(lpstr, "# ChipNomad Tracker Module 1.0")) return 1;
  READ_STRING; if (sscanf(lpstr, "- Title: %[^\n]", p.title) != 1) return 1;
  READ_STRING; if (sscanf(lpstr, "- Author: %[^\n]", p.author) != 1) return 1;

  READ_STRING; if (sscanf(lpstr, "- Frame rate: %f", &p.frameRate) != 1) return 1;
  READ_STRING; if (sscanf(lpstr, "- Chips count: %d", &p.chipsCount) != 1) return 1;
  READ_STRING; if (sscanf(lpstr, "- Chip type: %s", buf) != 1) return 1;

  int found = 0;
  for (int c = 0; c < chipTotalCount; c++) {
    if (strcmp(buf, chipNames[c]) == 0) {
      found = 1;
      p.chipType = c;
      break;
    }
  }
  if (!found) return 1;

  switch (p.chipType) {
    case chipAY:
      READ_STRING; if (sscanf(lpstr, "- *AY8910* Clock: %d", &p.chipSetup.ay.clock) != 1) return 1;
      READ_STRING; if (sscanf(lpstr, "- *AY8910* AY/YM: %d", &p.chipSetup.ay.isYM) != 1) return 1;
      READ_STRING; if (sscanf(lpstr, "- *AY8910* PanA: %hhu", &p.chipSetup.ay.panA) != 1) return 1;
      READ_STRING; if (sscanf(lpstr, "- *AY8910* PanB: %hhu", &p.chipSetup.ay.panB) != 1) return 1;
      READ_STRING; if (sscanf(lpstr, "- *AY8910* PanC: %hhu", &p.chipSetup.ay.panC) != 1) return 1;
      break;
    default:
      break;
  }

  p.tracksCount = p.chipsCount * 3; // Hardcoded for AY for now

  if (projectLoadPitchTable(fileId, &p)) return 1;
  if (projectLoadSong(fileId, &p)) return 1;
  if (projectLoadChains(fileId, &p)) return 1;
  if (projectLoadGrooves(fileId, &p)) return 1;
  if (projectLoadPhrases(fileId, &p)) return 1;
  if (projectLoadInstruments(fileId, &p)) return 1;
  if (projectLoadTables(fileId, &p)) return 1;

  // Copy loaded project to the current project
  memcpy(&project, &p, sizeof(p));

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Save

static int projectSavePitchTable(int fileId) {
  filePrintf(fileId, "\n## Pitch table\n\n");
  filePrintf(fileId, "- Title: %s\n\n```\n", project.pitchTable.title);

  for (int c = 0; c < project.pitchTable.length; c++) {
    filePrintf(fileId, "%s %d\n", project.pitchTable.names[c], project.pitchTable.values[c]);
  }

  filePrintf(fileId, "```\n");

  return 0;
}

static int projectSaveSong(int fileId) {
  filePrintf(fileId, "\n## Song\n\n```\n");

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

  filePrintf(fileId, "```\n");

  return 0;
}

static int projectSaveChains(int fileId) {
  filePrintf(fileId, "\n## Chains\n");

  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    if (!chainIsEmpty(c)) {
      filePrintf(fileId, "\n### Chain %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        int phrase = project.chains[c].phrases[d];
        if (phrase == EMPTY_VALUE_16) {
          filePrintf(fileId, "--- %s\n", byteToHex(project.chains[c].transpose[d]));
        } else {
          filePrintf(fileId, "%03X %s\n", project.chains[c].phrases[d], byteToHex(project.chains[c].transpose[d]));
        }
      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

static int projectSaveGrooves(int fileId) {
  filePrintf(fileId, "\n## Grooves\n");

  for (int c = 0; c < PROJECT_MAX_GROOVES; c++) {
    if (!grooveIsEmpty(c)) {
      filePrintf(fileId, "\n### Groove %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        int phrase = project.grooves[c].speed[d];
        if (phrase == EMPTY_VALUE_8) {
          filePrintf(fileId, "--\n");
        } else {
          filePrintf(fileId, "%s\n", byteToHex(project.grooves[c].speed[d]));
        }
      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;

}

static int projectSavePhrases(int fileId) {
  filePrintf(fileId, "\n## Phrases\n\n");

  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    if (!phraseIsEmpty(c)) {
      filePrintf(fileId, "### Phrase %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {

      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

static int projectSaveInstruments(int fileId) {
  filePrintf(fileId, "\n## Instruments\n\n");

  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    if (!instrumentIsEmpty(c)) {
      filePrintf(fileId, "### Instrument %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {

      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

static int projectSaveTables(int fileId) {
  filePrintf(fileId, "\n## Tables\n\n");

  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    if (!tableIsEmpty(c)) {
      filePrintf(fileId, "### Table %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {

      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

int projectSave(const char* path) {
  int fileId = fileOpen(path, 1);
  if (fileId == -1) return 1;

  filePrintf(fileId, "# ChipNomad Tracker Module 1.0\n\n");

  filePrintf(fileId, "- Title: %s\n", project.title);
  filePrintf(fileId, "- Author: %s\n", project.author);

  filePrintf(fileId, "- Frame rate: %f\n", project.frameRate);
  filePrintf(fileId, "- Chips count: %d\n", project.chipsCount);
  filePrintf(fileId, "- Chip type: %s\n", chipNames[project.chipType]);

  switch (project.chipType) {
    case chipAY:
      filePrintf(fileId, "- *AY8910* Clock: %d\n", project.chipSetup.ay.clock);
      filePrintf(fileId, "- *AY8910* AY/YM: %d\n", project.chipSetup.ay.isYM);
      filePrintf(fileId, "- *AY8910* PanA: %d\n", project.chipSetup.ay.panA);
      filePrintf(fileId, "- *AY8910* PanB: %d\n", project.chipSetup.ay.panB);
      filePrintf(fileId, "- *AY8910* PanC: %d\n", project.chipSetup.ay.panC);
      break;
    default:
      break;
  }

  projectSavePitchTable(fileId);
  projectSaveSong(fileId);
  projectSaveChains(fileId);
  projectSaveGrooves(fileId);
  projectSavePhrases(fileId);
  projectSaveInstruments(fileId);
  projectSaveTables(fileId);

  fileClose(fileId);
  return 0;
}