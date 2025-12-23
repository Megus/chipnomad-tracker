#include <stdio.h>
#include <string.h>
#include "project.h"
#include "corelib/corelib_file.h"
#include "utils.h"

FXName fxNames[256];

// FX Names (in the order as they appear in FX select screen)
FXName fxNamesCommon[] = {
  {fxARP, "ARP"}, {fxARC, "ARC"}, {fxPVB, "PVB"}, {fxPBN, "PBN"}, {fxPSL, "PSL"}, {fxPIT, "PIT"}, {fxPRD, "PRD"},
  {fxVOL, "VOL"}, {fxRET, "RET"}, {fxDEL, "DEL"}, {fxOFF, "OFF"}, {fxKIL, "KIL"},
  {fxTIC, "TIC"}, {fxTBL, "TBL"}, {fxTBX, "TBX"}, {fxTHO, "THO"}, {fxTXH, "TXH"},
  {fxGRV, "GRV"}, {fxGGR, "GGR"}, {fxHOP, "HOP"},
};
int fxCommonCount = sizeof(fxNamesCommon) / sizeof(FXName);

FXName fxNamesAY[] = {
  {fxAYM, "AYM"}, {fxERT, "ERT"}, {fxNOI, "NOI"}, {fxNOA, "NOA"},
  {fxEAU, "EAU"}, {fxEVB, "EVB"}, {fxEBN, "EBN"}, {fxESL, "ESL"},
  {fxENT, "ENT"}, {fxEPT, "EPT"}, {fxEPL, "EPL"}, {fxEPH, "EPH"},
};
int fxAYCount = sizeof(fxNamesAY) / sizeof(FXName);

// Fill FX names
void fillFXNames() {
  for (int c = 0; c < 256; c++) {
    strcpy(fxNames[c].name, "---");
    fxNames[c].fx = c;
  }

  for (int c = 0; c < fxCommonCount; c++) {
    strcpy(fxNames[fxNamesCommon[c].fx].name, fxNamesCommon[c].name);
    fxNames[fxNamesCommon[c].fx].fx = fxNamesCommon[c].fx;
  }

  for (int c = 0; c < fxAYCount; c++) {
    strcpy(fxNames[fxNamesAY[c].fx].name, fxNamesAY[c].name);
    fxNames[fxNamesAY[c].fx].fx = fxNamesAY[c].fx;
  }
}

// Initialize project
void projectInit(Project* p) {
  // Title
  strcpy(p->title, "");
  strcpy(p->author, "");
  p->linearPitch = 0;

  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      p->song[c][d] = EMPTY_VALUE_16;
    }
  }

  // Clean chains
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    chainClear(&p->chains[c]);
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
    phraseClear(&p->phrases[c]);
  }

  // Clean instruments
  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    instrumentClear(&p->instruments[c]);
  }

  // Clean tables
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    tableClear(&p->tables[c]);
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

static uint8_t scanNote(char* str, Project* p) {
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

static uint8_t scanFX(char* str, Project* p) {
  // Silly linear search through the list of FX. To replace with a simple hash
  static char buf[4];
  buf[0] = str[0];
  buf[1] = str[1];
  buf[2] = str[2];
  buf[3] = 0;

  if (!strcmp(buf, "---")) return EMPTY_VALUE_8;

  // Common FX
  for (int c = 0; c < fxCommonCount; c++) {
    if (!strcmp(buf, fxNamesCommon[c].name)) return fxNamesCommon[c].fx;
  }

  // AY FX
  for (int c = 0; c < fxAYCount; c++) {
    if (!strcmp(buf, fxNamesAY[c].name)) return fxNamesAY[c].fx;
  }

  return EMPTY_VALUE_8;
}

#define READ_STRING readString(fileId); if (lpstr == NULL) return 1;

static int projectLoadPitchTable(int fileId, Project* p) {
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

static int projectLoadSong(int fileId, Project* p) {
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

static int projectLoadChains(int fileId, Project* p) {
  int idx;

  if (strcmp(lpstr, "## Chains")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Chain", 9)) break;
    if (sscanf(lpstr, "### Chain %X", &idx) != 1) return 1;

    for (int c = 0; c < 16; c++) {
      READ_STRING;
      if (strlen(lpstr) != 6) return 1;

      if (lpstr[0] == '-') {
        p->chains[idx].rows[c].phrase = EMPTY_VALUE_16;
        if (sscanf(lpstr + 4, "%hhX", &p->chains[idx].rows[c].transpose) != 1) return 1;
      } else {
        if (sscanf(lpstr, "%hX %hhX", &p->chains[idx].rows[c].phrase, &p->chains[idx].rows[c].transpose) != 2) return 1;
      }
    }
  }

  return 0;
}

static int projectLoadGrooves(int fileId, Project* p) {
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

static int projectLoadPhrases(int fileId, Project* p) {
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
      p->phrases[idx].rows[c].note = scanNote(lpstr, p);
      // Instrument
      p->phrases[idx].rows[c].instrument = scanByteOrEmpty(lpstr + 4);
      // Volume
      p->phrases[idx].rows[c].volume = scanByteOrEmpty(lpstr + 7);
      // FX1
      p->phrases[idx].rows[c].fx[0][0] = scanFX(lpstr + 10, p);
      p->phrases[idx].rows[c].fx[0][1] = scanByteOrEmpty(lpstr + 14);
      // FX2
      p->phrases[idx].rows[c].fx[1][0] = scanFX(lpstr + 17, p);
      p->phrases[idx].rows[c].fx[1][1] = scanByteOrEmpty(lpstr + 21);
      // FX3
      p->phrases[idx].rows[c].fx[2][0] = scanFX(lpstr + 24, p);
      p->phrases[idx].rows[c].fx[2][1] = scanByteOrEmpty(lpstr + 28);
    }
  }

  return 0;
}

static int loadInstrument(int fileId, Instrument* instrument, Project* p) {
  // Read name
  READ_STRING;
  if (!strncmp(lpstr, "- Name:", 7)) {
    if (sscanf(lpstr, "- Name: %[^\n]", instrument->name) != 1) {
      instrument->name[0] = 0; // Empty instrument name
    }
  }

  // Read type
  READ_STRING;
  if (sscanf(lpstr, "- Type: %hhd", &instrument->type) != 1) return 1;

  // Read table speed
  READ_STRING;
  if (sscanf(lpstr, "- Table speed: %hhu", &instrument->tableSpeed) != 1) return 1;

  // Read transpose enabled
  READ_STRING;
  if (sscanf(lpstr, "- Transpose: %hhu", &instrument->transposeEnabled) != 1) return 1;

  if (instrument->type == instAY) {
    // Read volume envelope
    READ_STRING;
    if (sscanf(lpstr, "- Volume envelope: %hhu,%hhu,%hhu,%hhu",
      &instrument->chip.ay.veA, &instrument->chip.ay.veD,
      &instrument->chip.ay.veS, &instrument->chip.ay.veR) != 4) return 1;

    // Read auto envelope
    READ_STRING;
    if (sscanf(lpstr, "- Auto envelope: %hhu,%hhu",
      &instrument->chip.ay.autoEnvN, &instrument->chip.ay.autoEnvD) != 2) return 1;
  }
  return 0;
}

static int projectLoadInstruments(int fileId, Project* p) {
  int idx;

  if (strcmp(lpstr, "## Instruments")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Instrument", 13)) break;
    if (sscanf(lpstr, "### Instrument %X", &idx) != 1) return 1;
    if (loadInstrument(fileId, &p->instruments[idx], p)) return 1;
  }

  return 0;
}

static int loadTable(int fileId, Table* table, Project* p) {
  for (int d = 0; d < 16; d++) {
    READ_STRING;
    if (strlen(lpstr) < 35) return 1;  // Minimum length check

    // Read pitch flag (single char)
    table->rows[d].pitchFlag = (lpstr[0] == '=') ? 1 : 0;

    // Read pitch offset
    table->rows[d].pitchOffset = scanByteOrEmpty(lpstr + 2);

    // Read volume
    table->rows[d].volume = scanByteOrEmpty(lpstr + 5);

    // Read FX1
    table->rows[d].fx[0][0] = scanFX(lpstr + 8, p);
    table->rows[d].fx[0][1] = scanByteOrEmpty(lpstr + 12);

    // Read FX2
    table->rows[d].fx[1][0] = scanFX(lpstr + 15, p);
    table->rows[d].fx[1][1] = scanByteOrEmpty(lpstr + 19);

    // Read FX3
    table->rows[d].fx[2][0] = scanFX(lpstr + 22, p);
    table->rows[d].fx[2][1] = scanByteOrEmpty(lpstr + 26);

    // Read FX4
    table->rows[d].fx[3][0] = scanFX(lpstr + 29, p);
    table->rows[d].fx[3][1] = scanByteOrEmpty(lpstr + 33);
  }
  return 0;
}

static int projectLoadTables(int fileId, Project* p) {
  int idx;

  if (strcmp(lpstr, "## Tables")) return 1;

  while (1) {
    READ_STRING;
    if (strncmp(lpstr, "### Table", 9)) break;
    if (sscanf(lpstr, "### Table %X", &idx) != 1) return 1;
    if (loadTable(fileId, &p->tables[idx], p)) return 1;
  }

  return 0;
}

static int projectLoadInternal(int fileId, Project* project) {
  char buf[128];
  Project p;

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

  READ_STRING; if (sscanf(lpstr, "- Frame rate: %f", &p.tickRate) != 1) return 1;
  READ_STRING; if (sscanf(lpstr, "- Chips count: %d", &p.chipsCount) != 1) return 1;

  // Try to read linear pitch (optional for backwards compatibility)
  READ_STRING;
  int tempLinearPitch;
  if (sscanf(lpstr, "- Linear pitch: %d", &tempLinearPitch) == 1) {
    p.linearPitch = (uint8_t)tempLinearPitch;
    READ_STRING; // Read next line for chip type
  }
  // If linear pitch not found, lpstr already contains the chip type line
  if (sscanf(lpstr, "- Chip type: %s", buf) != 1) return 1;

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
    int tempIsYM;
    READ_STRING; if (sscanf(lpstr, "- *AY8910* AY/YM: %d", &tempIsYM) != 1) return 1;
    p.chipSetup.ay.isYM = (uint8_t)tempIsYM;
    // TODO: Remove old pan logic for the first public release
    READ_STRING;
    if (strncmp(lpstr, "- *AY8910* PanA:", 15) == 0) {
      // Old pan storage
      READ_STRING; // Skip B
      READ_STRING; // Skip C
      // Default to ABC
      p.chipSetup.ay.stereoMode = ayStereoABC;
      p.chipSetup.ay.stereoSeparation = 50;
    } else if (strncmp(lpstr, "- *AY8910* Stereo:", 18) == 0) {
      // New pan storage
      if (sscanf(lpstr, "- *AY8910* Stereo: %s", buf) != 1) return 1;
      if (strcmp(buf, "ABC") == 0) {
        p.chipSetup.ay.stereoMode = ayStereoABC;
      } else if (strcmp(buf, "ACB") == 0) {
        p.chipSetup.ay.stereoMode = ayStereoACB;
      } else if (strcmp(buf, "BAC") == 0) {
        p.chipSetup.ay.stereoMode = ayStereoBAC;
      } else {
        return 1;
      }
      READ_STRING; if (sscanf(lpstr, "- *AY8910* Stereo separation: %hhu", &p.chipSetup.ay.stereoSeparation) != 1) return 1;
    } else {
      // Error, pan information should be here
      return 1;
    }
    break;
  default:
    break;
  }

  p.tracksCount = projectGetTotalTracks(&p);

  sprintf(projectFileError, "Pitch table");

  if (projectLoadPitchTable(fileId, &p)) return 1;
  if (projectLoadSong(fileId, &p)) return 1;
  if (projectLoadChains(fileId, &p)) return 1;
  if (projectLoadGrooves(fileId, &p)) return 1;
  if (projectLoadPhrases(fileId, &p)) return 1;
  if (projectLoadInstruments(fileId, &p)) return 1;
  if (projectLoadTables(fileId, &p)) return 1;

  // Copy loaded project to the target project
  memcpy(project, &p, sizeof(Project));

  return 0;
}

int projectLoad(Project* p, const char* path) {
  projectFileError[0] = 0;

  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  int result = projectLoadInternal(fileId, p);
  fileClose(fileId);
  return result;
}

/////////////////////////////////////////////////////////////////////////////
// Save

static int projectSavePitchTable(int fileId, Project* project) {
  filePrintf(fileId, "\n## Pitch table\n\n");
  filePrintf(fileId, "- Title: %s\n\n```\n", project->pitchTable.name);

  for (int c = 0; c < project->pitchTable.length; c++) {
    filePrintf(fileId, "%s %d\n", project->pitchTable.noteNames[c], project->pitchTable.values[c]);
  }

  filePrintf(fileId, "```\n");

  return 0;
}

static int projectSaveSong(int fileId, Project* project) {
  filePrintf(fileId, "\n## Song\n\n```\n");

  // Find the last row with values
  int songLength = PROJECT_MAX_LENGTH;
  for (songLength = PROJECT_MAX_LENGTH - 1; songLength >= 0; songLength--) {
    int isEmpty = 1;
    for (int c = 0; c < project->tracksCount; c++) {
      if (project->song[songLength][c] != EMPTY_VALUE_16) {
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
    for (int d = 0; d < project->tracksCount; d++) {
      int chain = project->song[c][d];
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

static int projectSaveChains(int fileId, Project* project) {
  filePrintf(fileId, "\n## Chains\n");

  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    if (!chainIsEmpty(project, c)) {
      filePrintf(fileId, "\n### Chain %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        int phrase = project->chains[c].rows[d].phrase;
        if (phrase == EMPTY_VALUE_16) {
          filePrintf(fileId, "--- %s\n", byteToHex(project->chains[c].rows[d].transpose));
        } else {
          filePrintf(fileId, "%03X %s\n", project->chains[c].rows[d].phrase, byteToHex(project->chains[c].rows[d].transpose));
        }
      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

static int projectSaveGrooves(int fileId, Project* project) {
  filePrintf(fileId, "\n## Grooves\n");

  for (int c = 0; c < PROJECT_MAX_GROOVES; c++) {
    if (!grooveIsEmpty(project, c)) {
      filePrintf(fileId, "\n### Groove %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        filePrintf(fileId, "%s\n", byteToHexOrEmpty(project->grooves[c].speed[d]));
      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

static int projectSavePhrases(int fileId, Project* project) {
  filePrintf(fileId, "\n## Phrases\n\n");

  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    if (!phraseIsEmpty(project, c)) {
      filePrintf(fileId, "### Phrase %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        filePrintf(fileId, "%s %s %s %s %s %s %s %s %s\n",
          noteName(project, project->phrases[c].rows[d].note),
          byteToHexOrEmpty(project->phrases[c].rows[d].instrument),
          byteToHexOrEmpty(project->phrases[c].rows[d].volume),
          fxNames[project->phrases[c].rows[d].fx[0][0]].name,
          byteToHex(project->phrases[c].rows[d].fx[0][1]),
          fxNames[project->phrases[c].rows[d].fx[1][0]].name,
          byteToHex(project->phrases[c].rows[d].fx[1][1]),
          fxNames[project->phrases[c].rows[d].fx[2][0]].name,
          byteToHex(project->phrases[c].rows[d].fx[2][1])
        );
      }
      filePrintf(fileId, "```\n");
    }
  }

  return 0;
}

static int saveInstrument(int fileId, int idx, Instrument* instrument) {
  filePrintf(fileId, "\n### Instrument %X\n\n", idx);
  filePrintf(fileId, "- Name: %s\n", instrument->name);
  filePrintf(fileId, "- Type: %hhd\n", instrument->type);
  filePrintf(fileId, "- Table speed: %hhu\n", instrument->tableSpeed);
  filePrintf(fileId, "- Transpose: %hhu\n", instrument->transposeEnabled);

  if (instrument->type == instAY) {
    filePrintf(fileId, "- Volume envelope: %hhu,%hhu,%hhu,%hhu\n",
      instrument->chip.ay.veA, instrument->chip.ay.veD,
      instrument->chip.ay.veS, instrument->chip.ay.veR);
    filePrintf(fileId, "- Auto envelope: %hhd,%hhd\n",
      instrument->chip.ay.autoEnvN, instrument->chip.ay.autoEnvD);
  }
  return 0;
}

static int saveTable(int fileId, int idx, Table* table) {
  filePrintf(fileId, "\n### Table %X\n\n```\n", idx);
  for (int d = 0; d < 16; d++) {
    filePrintf(fileId, "%c %s %s %s %s %s %s %s %s %s %s\n",
      table->rows[d].pitchFlag ? '=' : '~',
      byteToHex(table->rows[d].pitchOffset),
      byteToHexOrEmpty(table->rows[d].volume),
      fxNames[table->rows[d].fx[0][0]].name, byteToHex(table->rows[d].fx[0][1]),
      fxNames[table->rows[d].fx[1][0]].name, byteToHex(table->rows[d].fx[1][1]),
      fxNames[table->rows[d].fx[2][0]].name, byteToHex(table->rows[d].fx[2][1]),
      fxNames[table->rows[d].fx[3][0]].name, byteToHex(table->rows[d].fx[3][1]));
  }
  filePrintf(fileId, "```\n");
  return 0;
}

static int projectSaveInstruments(int fileId, Project* project) {
  filePrintf(fileId, "\n## Instruments\n");
  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    if (!instrumentIsEmpty(project, c)) {
      saveInstrument(fileId, c, &project->instruments[c]);
    }
  }
  return 0;
}

static int projectSaveTables(int fileId, Project* project) {
  filePrintf(fileId, "\n## Tables\n");
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    if (!tableIsEmpty(project, c)) {
      saveTable(fileId, c, &project->tables[c]);
    }
  }
  return 0;
}

static int projectSaveInternal(int fileId, Project* project) {
  filePrintf(fileId, "# ChipNomad Tracker Module 1.0\n\n");

  filePrintf(fileId, "- Title: %s\n", project->title);
  filePrintf(fileId, "- Author: %s\n", project->author);

  filePrintf(fileId, "- Frame rate: %f\n", project->tickRate);
  filePrintf(fileId, "- Chips count: %d\n", project->chipsCount);
  filePrintf(fileId, "- Linear pitch: %d\n", project->linearPitch);
  filePrintf(fileId, "- Chip type: %s\n", chipNames[project->chipType]);

  switch (project->chipType) {
  case chipAY:
    filePrintf(fileId, "- *AY8910* Clock: %d\n", project->chipSetup.ay.clock);
    filePrintf(fileId, "- *AY8910* AY/YM: %d\n", project->chipSetup.ay.isYM);
    switch (project->chipSetup.ay.stereoMode) {
    case ayStereoABC:
      filePrintf(fileId, "- *AY8910* Stereo: ABC\n");
      break;
    case ayStereoACB:
      filePrintf(fileId, "- *AY8910* Stereo: ACB\n");
      break;
    case ayStereoBAC:
      filePrintf(fileId, "- *AY8910* Stereo: BAC\n");
      break;
    }
    filePrintf(fileId, "- *AY8910* Stereo separation: %d\n", project->chipSetup.ay.stereoSeparation);
    break;
  default:
    break;
}

  projectSavePitchTable(fileId, project);
  projectSaveSong(fileId, project);
  projectSaveChains(fileId, project);
  projectSaveGrooves(fileId, project);
  projectSavePhrases(fileId, project);
  projectSaveInstruments(fileId, project);
  projectSaveTables(fileId, project);
  filePrintf(fileId, "EOF\n");
  return 0;
}

int projectSave(Project* p, const char* path) {
  projectFileError[0] = 0;

  int fileId = fileOpen(path, 1);
  if (fileId == -1) return 1;

  int result = projectSaveInternal(fileId, p);
  fileClose(fileId);
  return result;
}

// Convenience functions

// Is chain empty?
int8_t chainIsEmpty(Project* project, int chain) {
  for (int c = 0; c < 16; c++) {
    if (project->chains[chain].rows[c].phrase != EMPTY_VALUE_16) return 0;
  }
  return 1;
}

// Is phrase empty?
int8_t phraseIsEmpty(Project* project, int phrase) {
  for (int c = 0; c < 16; c++) {
    if (project->phrases[phrase].rows[c].note != EMPTY_VALUE_8) return 0;
    if (project->phrases[phrase].rows[c].instrument != EMPTY_VALUE_8) return 0;
    if (project->phrases[phrase].rows[c].volume != EMPTY_VALUE_8) return 0;
    for (int d = 0; d < 3; d++) {
      if (project->phrases[phrase].rows[c].fx[d][0] != EMPTY_VALUE_8) return 0;
      if (project->phrases[phrase].rows[c].fx[d][1] != 0) return 0;
    }
  }

  return 1;
}

// Is instrument empty?
int8_t instrumentIsEmpty(Project* project, int instrument) {
  return project->instruments[instrument].type == instNone;
}

// Is table empty?
int8_t tableIsEmpty(Project* project, int table) {
  for (int c = 0; c < 16; c++) {
    if (project->tables[table].rows[c].pitchFlag != 0) return 0;
    if (project->tables[table].rows[c].pitchOffset != 0) return 0;
    if (project->tables[table].rows[c].volume != EMPTY_VALUE_8) return 0;
    for (int d = 0; d < 4; d++) {
      if (project->tables[table].rows[c].fx[d][0] != EMPTY_VALUE_8) return 0;
      if (project->tables[table].rows[c].fx[d][1] != 0) return 0;
    }
  }

  return 1;
}

// Is groove empty?
int8_t grooveIsEmpty(Project* project, int groove) {
  for (int c = 0; c < 16; c++) {
    if (project->grooves[groove].speed[c] != EMPTY_VALUE_8) return 0;
  }
  return 1;
}

// Note name in phrase
char* noteName(Project* project, uint8_t note) {
  if (note == NOTE_OFF) {
    return "OFF";
  } else if (note < project->pitchTable.length) {
    return project->pitchTable.noteNames[note];
  } else {
    return "---";
  }
}

// Save instrument to file
int instrumentSave(Project* project, const char* path, int instrumentIdx) {
  projectFileError[0] = 0;

  int fileId = fileOpen(path, 1);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  filePrintf(fileId, "# ChipNomad Instrument\n\n");
  saveInstrument(fileId, 0, &project->instruments[instrumentIdx]);
  saveTable(fileId, 0, &project->tables[instrumentIdx]);

  fileClose(fileId);
  return 0;
}

static int instrumentLoadInternal(int fileId, Project* project, int instrumentIdx) {
  READ_STRING; if (strcmp(lpstr, "# ChipNomad Instrument")) return 1;
  READ_STRING; if (strncmp(lpstr, "### Instrument", 14)) return 1;

  if (loadInstrument(fileId, &project->instruments[instrumentIdx], project)) return 1;

  READ_STRING; if (strncmp(lpstr, "### Table", 9)) return 1;

  if (loadTable(fileId, &project->tables[instrumentIdx], project)) return 1;

  return 0;
}

// Load instrument from file
int instrumentLoad(Project* project, const char* path, int instrumentIdx) {
  projectFileError[0] = 0;

  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  int result = instrumentLoadInternal(fileId, project, instrumentIdx);
  fileClose(fileId);
  return result;
}

// Get number of tracks for a chip at index
int projectGetChipTracks(Project* p, int chipIndex) {
  // Hardcoded for AY chips (3 channels) for now
  return 3;
}

// Get total number of tracks for the project
int projectGetTotalTracks(Project* p) {
  int totalTracks = 0;
  for (int i = 0; i < p->chipsCount; i++) {
    totalTracks += projectGetChipTracks(p, i);
  }
  return totalTracks;
}

// Clear a single phrase with proper initialization
void phraseClear(Phrase* phrase) {
  for (int d = 0; d < 16; d++) {
    phrase->rows[d].note = EMPTY_VALUE_8;
    phrase->rows[d].instrument = EMPTY_VALUE_8;
    phrase->rows[d].volume = EMPTY_VALUE_8;
    for (int e = 0; e < 3; e++) {
      phrase->rows[d].fx[e][0] = EMPTY_VALUE_8;
      phrase->rows[d].fx[e][1] = 0;
    }
  }
}

// Clear a single chain with proper initialization
void chainClear(Chain* chain) {
  for (int d = 0; d < 16; d++) {
    chain->rows[d].phrase = EMPTY_VALUE_16;
    chain->rows[d].transpose = 0;
  }
}

// Clear a single instrument with proper initialization
void instrumentClear(Instrument* instrument) {
  instrument->type = instNone;
  instrument->name[0] = 0;
}

// Clear a single table with proper initialization
void tableClear(Table* table) {
  for (int d = 0; d < 16; d++) {
    table->rows[d].pitchFlag = 0;
    table->rows[d].pitchOffset = 0;
    table->rows[d].volume = EMPTY_VALUE_8;
    for (int e = 0; e < 4; e++) {
      table->rows[d].fx[e][0] = EMPTY_VALUE_8;
      table->rows[d].fx[e][1] = 0;
    }
  }
}