#include <stdio.h>
#include <math.h>
#include <string.h>
#include <project.h>
#include <corelib_file.h>
#include <utils.h>

struct Project project;

struct FXName fxNames[256];

// FX Names (in the order as they appear in FX select screen)
struct FXName fxCommon[] = {
  {fxARP, "ARP"}, {fxARC, "ARC"}, {fxPVB, "PVB"}, {fxPBN, "PBN"}, {fxPSL, "PSL"}, {fxPIT, "PIT"}, // Pitch
  {fxRET, "RET"}, {fxDEL, "DEL"}, {fxOFF, "OFF"}, {fxKIL, "KIL"}, // Sequencer
  {fxTIC, "TIC"}, {fxTBL, "TBL"}, {fxTBX, "TBX"}, {fxTHO, "THO"}, // Table
  {fxGRV, "GRV"}, {fxGGR, "GGR"}, {fxSNG, "SNG"},
};
int fxCommonCount = sizeof(fxCommon) / sizeof(struct FXName);

struct FXName fxAY[] = {
  {fxAYM, "AYM"}, {fxERT, "ERT"}, {fxNOI, "NOI"}, {fxNOA, "NOA"},
  {fxEAU, "EAU"}, {fxEVB, "EVB"}, {fxEBN, "EBN"}, {fxESL, "ESL"},
  {fxENA, "ENA"}, {fxENR, "ENR"}, {fxEPR, "EPR"}, {fxEPL, "EPL"}, {fxEPH, "EPH"},
};
int fxAYCount = sizeof(fxAY) / sizeof(struct FXName);

// Fill FX names
void fillFXNames() {
  for (int c = 0; c < 256; c++) {
    strcpy(fxNames[c].name, "---");
    fxNames[c].fx = c;
  }

  for (int c = 0; c < fxCommonCount; c++) {
    strcpy(fxNames[fxCommon[c].fx].name, fxCommon[c].name);
    fxNames[fxCommon[c].fx].fx = fxCommon[c].fx;
  }

  for (int c = 0; c < fxAYCount; c++) {
    strcpy(fxNames[fxAY[c].fx].name, fxAY[c].name);
    fxNames[fxAY[c].fx].fx = fxAY[c].fx;
  }
}


// Create 12TET scale
void calculatePitchTableAY(struct Project* p) {
  static char noteStrings[12][4] = { "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1" };

  float clock = (float)(p->chipSetup.ay.clock);
  int octaves = 9;
  float cfreq = 16.35159783; // C-0 frequency for A4 = 440Hz. It's too low for 1.75MHz AY, but we'll keep it
  float freq = cfreq;
  float semitone = powf(2., 1. / 12.);

  sprintf(p->pitchTable.name, "12TET %dHz", p->chipSetup.ay.clock);
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
      strcpy(p->pitchTable.noteNames[o * 12 + c], noteStrings[c]);

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
    p->instruments[c].name[0] = 0;
  }

  // Simple default instrument
  p->instruments[0].type = instAY;
  strcpy(p->instruments[0].name, "LEAD1");
  p->instruments[0].tableSpeed = 1;
  p->instruments[0].transposeEnabled = 1;
  p->instruments[0].chip.ay.veA = 0;
  p->instruments[0].chip.ay.veD = 12;
  p->instruments[0].chip.ay.veS = 10;
  p->instruments[0].chip.ay.veR = 12;
  p->instruments[0].chip.ay.autoEnvN = 0;
  p->instruments[0].chip.ay.autoEnvD = 0;

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
    if (project.phrases[phrase].notes[c] < PROJECT_MAX_PITCHES) {
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

// Instrument name
char* instrumentName(uint8_t instrument) {
  if (project.instruments[instrument].type == instNone) return "None";
  if (strlen(project.instruments[instrument].name) == 0) {
    switch (project.instruments[instrument].type) {
      case instAY:
        return "AY";
        break;
      default:
        return "";
        break;
    }
  } else {
    return project.instruments[instrument].name;
  }
}

// Note name in phrase
char* noteName(uint8_t note) {
  if (note == NOTE_OFF) {
    return "OFF";
  } else if (note < project.pitchTable.length) {
    return project.pitchTable.noteNames[note];
  } else {
    return "---";
  }
}


///////////////////////////////////////////////////////////////////////////////
//
// Load/save project
//

static char *lpstr;
static char chipNames[][16] = { "AY8910" };
char projectFileError[41];

///////////////////////////////////////////////////////////////////////////////
// Load

// Convenience function to read a non-empty string
static char* readString(int fileId) {
  while (1) {
    lpstr = fileReadString(fileId);
    if (lpstr == NULL) {
      sprintf(projectFileError, "Couldn't read string");
      return NULL;
    }
    // Skip empty lines and lines with ```
    if (strlen(lpstr) > 0 && strcmp(lpstr, "```")) break;
  }
  sprintf(projectFileError, "%s", lpstr);

  return lpstr;
}

static uint8_t scanByteOrEmpty(char* str) {
  static char buf[3];
  buf[0] = str[0];
  buf[1] = str[1];
  buf[2] = 0;
  if (buf[0] == '-' && buf[1] == '-') {
    return EMPTY_VALUE_8;
  } else {
    uint8_t result;
    if (sscanf(buf, "%hhX", &result) != 1) return EMPTY_VALUE_8;
    return result;
  }
}

static uint8_t scanNote(char* str, struct Project* p) {
  // Silly linear search through pitch table. To replace with a simple hash
  static char buf[4];
  buf[0] = str[0];
  buf[1] = str[1];
  buf[2] = str[2];
  buf[3] = 0;

  if (!strcmp(buf, "---")) return EMPTY_VALUE_8;
  if (!strcmp(buf, "OFF")) return NOTE_OFF;

  for (int c = 0; c < p->pitchTable.length; c++) {
    if (!strcmp(buf, p->pitchTable.noteNames[c])) return c;
  }

  return EMPTY_VALUE_8;
}

static uint8_t scanFX(char* str, struct Project* p) {
  // Silly linear search through the list of FX. To replace with a simple hash
  static char buf[4];
  buf[0] = str[0];
  buf[1] = str[1];
  buf[2] = str[2];
  buf[3] = 0;

  if (!strcmp(buf, "---")) return EMPTY_VALUE_8;

  // Common FX
  for (int c = 0; c < fxCommonCount; c++) {
    if (!strcmp(buf, fxCommon[c].name)) return fxCommon[c].fx;
  }

  // AY FX
  for (int c = 0; c < fxAYCount; c++) {
    if (!strcmp(buf, fxAY[c].name)) return fxAY[c].fx;
  }

  return EMPTY_VALUE_8;
}

#define READ_STRING readString(fileId); if (lpstr == NULL) return 1;

static int projectLoadPitchTable(int fileId, struct Project* p) {
  char buf[128];

  READ_STRING; if (strcmp(lpstr, "## Pitch table")) return 1;
  READ_STRING; if (sscanf(lpstr, "- Title: %[^\n]", p->pitchTable.name) != 1) return 1;

  int idx = 0;
  int period;
  while (1) {
    READ_STRING;
    if (sscanf(lpstr, "%s %d", buf, &period) != 2) break;
    if (strlen(buf) != 3) return 1;
    strcpy(p->pitchTable.noteNames[idx], buf);
    p->pitchTable.values[idx] = period;
    idx++;
  }
  p->pitchTable.length = idx;

  // Detect octave size
  char oct = p->pitchTable.noteNames[0][2];
  for (int c = 0; c < p->pitchTable.length; c++) {
    if (p->pitchTable.noteNames[c][2] != oct) {
      p->pitchTable.octaveSize = c;
      break;
    }
  }

  return 0;
}

static int projectLoadSong(int fileId, struct Project* p) {
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

static int projectLoadChains(int fileId, struct Project* p) {
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

static int projectLoadGrooves(int fileId, struct Project* p) {
  int idx;

  if (strcmp(lpstr, "## Grooves")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Groove", 10)) break;
    if (sscanf(lpstr, "### Groove %X", &idx) != 1) return 1;

    for (int c = 0; c < 16; c++) {
      READ_STRING;
      if (strlen(lpstr) != 2) return 1;
      p->grooves[idx].speed[c] = scanByteOrEmpty(lpstr);
    }
  }

  return 0;
}

static int projectLoadPhrases(int fileId, struct Project* p) {
  int idx;

  if (strcmp(lpstr, "## Phrases")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Phrase", 10)) break;
    if (sscanf(lpstr, "### Phrase %X", &idx) != 1) return 1;

    for (int c = 0; c < 16; c++) {
      READ_STRING;
      if (strlen(lpstr) != 30) return 1;
      // Note
      p->phrases[idx].notes[c] = scanNote(lpstr, p);
      // Instrument
      p->phrases[idx].instruments[c] = scanByteOrEmpty(lpstr + 4);
      // Volume
      p->phrases[idx].volumes[c] = scanByteOrEmpty(lpstr + 7);
      // FX1
      p->phrases[idx].fx[c][0][0] = scanFX(lpstr + 10, p);
      p->phrases[idx].fx[c][0][1] = scanByteOrEmpty(lpstr + 14);
      // FX2
      p->phrases[idx].fx[c][1][0] = scanFX(lpstr + 17, p);
      p->phrases[idx].fx[c][1][1] = scanByteOrEmpty(lpstr + 21);
      // FX3
      p->phrases[idx].fx[c][2][0] = scanFX(lpstr + 24, p);
      p->phrases[idx].fx[c][2][1] = scanByteOrEmpty(lpstr + 28);
    }
  }

  return 0;
}

static int projectLoadInstruments(int fileId, struct Project* p) {

  return 0;
}

static int projectLoadTables(int fileId, struct Project* p) {

  return 0;
}

static int projectLoadInternal(int fileId) {
  char buf[128];
  struct Project p;

  projectInit(&p);

  sprintf(projectFileError, "Module header");
  READ_STRING; if (strcmp(lpstr, "# ChipNomad Tracker Module 1.0")) return 1;
  READ_STRING;
  if (!strncmp(lpstr, "- Title:", 8)) {
    if (sscanf(lpstr, "- Title: %[^\n]", p.title) != 1) {
      p.title[0] = 0; // Empty title
    }
  } else {
    return 1;
  }
  READ_STRING;
  if (!strncmp(lpstr, "- Author:", 9)) {
    if (sscanf(lpstr, "- Author: %[^\n]", p.author) != 1) {
      p.title[0] = 0; // Empty author
    }
  }

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

  sprintf(projectFileError, "Pitch table");

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

int projectLoad(const char* path) {

  projectFileError[0] = 0;

  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  int result = projectLoadInternal(fileId);
  fileClose(fileId);
  return result;
}

/////////////////////////////////////////////////////////////////////////////
// Save

static int projectSavePitchTable(int fileId) {
  filePrintf(fileId, "\n## Pitch table\n\n");
  filePrintf(fileId, "- Title: %s\n\n```\n", project.pitchTable.name);

  for (int c = 0; c < project.pitchTable.length; c++) {
    filePrintf(fileId, "%s %d\n", project.pitchTable.noteNames[c], project.pitchTable.values[c]);
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
        filePrintf(fileId, "%s\n", byteToHexOrEmpty(project.grooves[c].speed[d]));
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
        filePrintf(fileId, "%s %s %s %s %s %s %s %s %s\n",
          noteName(project.phrases[c].notes[d]),
          byteToHexOrEmpty(project.phrases[c].instruments[d]),
          byteToHexOrEmpty(project.phrases[c].volumes[d]),
          fxNames[project.phrases[c].fx[d][0][0]].name,
          byteToHex(project.phrases[c].fx[d][0][1]),
          fxNames[project.phrases[c].fx[d][1][0]].name,
          byteToHex(project.phrases[c].fx[d][1][1]),
          fxNames[project.phrases[c].fx[d][2][0]].name,
          byteToHex(project.phrases[c].fx[d][2][1])
        );
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

static int projectSaveInternal(int fileId) {
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
  return 0;
}

int projectSave(const char* path) {
  projectFileError[0] = 0;

  int fileId = fileOpen(path, 1);
  if (fileId == -1) return 1;

  int result = projectSaveInternal(fileId);
  fileClose(fileId);
  return result;
}