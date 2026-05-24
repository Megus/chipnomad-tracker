#include <stdio.h>
#include <string.h>
#include "project.h"
#include "project_io_common.h"
#include "corelib/corelib_file.h"
#include "utils.h"

// Shared state
char projectFileError[41];
int projectFileVersion = 2;  // Default to current version
static char chipNames[][16] = { "AY8910" };

// Peek/consume implementation - single global buffer (ChipNomad is single-threaded)
static char* currentLine = NULL;
static int isConsumed = 1;

void resetPeekConsume(void) {
  currentLine = NULL;
  isConsumed = 1;
}

char* peekLine(int fileId) {
  // If current line is consumed, read next non-empty line
  if (isConsumed) {
    while (1) {
      currentLine = fileReadString(fileId);
      if (currentLine == NULL) {
        sprintf(projectFileError, "Unexpected end of file");
        return NULL;
      }
      // Skip empty lines
      if (strlen(currentLine) > 0) {
        isConsumed = 0;
        break;
      }
    }
  }

  return currentLine;
}

void consumeLine(int fileId) {
  (void)fileId;  // Unused in single-buffer implementation
  isConsumed = 1;
}

// Helper functions for parsing
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

  // Scan all FX groups
  extern FXGroup fxGroups[];
  extern int fxGroupCount;
  for (int g = 0; g < fxGroupCount; g++) {
    for (int c = 0; c < fxGroups[g].count; c++) {
      if (!strcmp(buf, fxGroups[g].fxList[c].name)) return fxGroups[g].fxList[c].fx;
    }
  }

  return EMPTY_VALUE_8;
}

///////////////////////////////////////////////////////////////////////////////
// Load functions

static int projectLoadPitchTable(int fileId, Project* p) {
  char buf[128];

  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Pitch table")) return 1;
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (sscanf(line, "- Title: %[^\n]", p->pitchTable.name) != 1) return 1;
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  consumeLine(fileId);  // Skip opening ```

  int idx = 0;
  int period;
  while (1) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (sscanf(line, "%s %d", buf, &period) != 2) {
      // Couldn't parse - must be closing ``` or next section
      break;
    }
    if (strlen(buf) != 3) return 1;
    strcpy(p->pitchTable.noteNames[idx], buf);
    p->pitchTable.values[idx] = period;
    idx++;
    consumeLine(fileId);
  }
  p->pitchTable.length = idx;

  // Calculate octave size (find first note name change)
  if (idx > 0) {
    char firstOctave = p->pitchTable.noteNames[0][2];
    p->pitchTable.octaveSize = 12; // Default
    for (int i = 1; i < idx; i++) {
      if (p->pitchTable.noteNames[i][2] != firstOctave) {
        p->pitchTable.octaveSize = i;
        break;
      }
    }
  }

  // Consume the closing ``` if present
  if (line[0] == '`') {
    consumeLine(fileId);
  }

  return 0;
}

static int projectLoadSong(int fileId, Project* p) {
  char buf[4];
  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Song")) return 1;
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  consumeLine(fileId);  // Skip opening ```

  int idx = 0;
  while (1) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '#' || line[0] == '`') break;  // Next section or closing ```
    // Minimum length: 2 chars per cell, plus 1 separator for each cell (optional for last)
    if (strlen(line) < (p->tracksCount * 2 + p->tracksCount - 1)) return 1;

    for (int c = 0; c < p->tracksCount; c++) {
      buf[0] = line[c * 3];
      buf[1] = line[c * 3 + 1];
      buf[2] = 0;
      if (buf[0] == '-' && buf[1] == '-') {
        p->song[idx][c] = EMPTY_VALUE_16;
      } else {
        uint16_t result;
        if (sscanf(buf, "%hX", &result) != 1) return 1;
        p->song[idx][c] = result;
      }
      // Check for highlight marker
      if (line[c * 3 + 2] == '*') {
        p->songHighlight[idx][c] = 1;
      } else {
        p->songHighlight[idx][c] = 0;
      }
    }
    idx++;
    consumeLine(fileId);
  }

  // Consume closing ``` if present
  if (line[0] == '`') {
    consumeLine(fileId);
  }

  return 0;
}

static int projectLoadChains(int fileId, Project* p) {
  int idx;

  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Chains")) return 1;
  consumeLine(fileId);

  while (1) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (strncmp(line, "### Chain", 9)) break;
    if (sscanf(line, "### Chain %X", &idx) != 1) return 1;
    consumeLine(fileId);

    // Skip opening ```
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '`') {
      consumeLine(fileId);
    }

    for (int c = 0; c < 16; c++) {
      line = peekLine(fileId);
      if (line == NULL) return 1;
      if (strlen(line) != 6) return 1;

      if (line[0] == '-' && line[1] == '-' && line[2] == '-') {
        p->chains[idx].rows[c].phrase = EMPTY_VALUE_16;
      } else {
        uint16_t result;
        if (sscanf(line, "%hX", &result) != 1) return 1;
        p->chains[idx].rows[c].phrase = result;
      }
      p->chains[idx].rows[c].transpose = scanByteOrEmpty(line + 4);
      consumeLine(fileId);
    }

    // Skip closing ```
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '`') {
      consumeLine(fileId);
    }
  }

  return 0;
}

static int projectLoadGrooves(int fileId, Project* p) {
  int idx;

  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Grooves")) return 1;
  consumeLine(fileId);

  while (1) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (strncmp(line, "### Groove", 10)) break;
    if (sscanf(line, "### Groove %X", &idx) != 1) return 1;
    consumeLine(fileId);

    // Skip opening ```
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '`') {
      consumeLine(fileId);
    }

    for (int c = 0; c < 16; c++) {
      line = peekLine(fileId);
      if (line == NULL) return 1;
      if (strlen(line) != 2) return 1;
      p->grooves[idx].speed[c] = scanByteOrEmpty(line);
      consumeLine(fileId);
    }

    // Skip closing ```
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '`') {
      consumeLine(fileId);
    }
  }

  return 0;
}

static int projectLoadPhrases(int fileId, Project* p) {
  int idx;

  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Phrases")) return 1;
  consumeLine(fileId);

  while (1) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (strncmp(line, "### Phrase", 10)) break;
    if (sscanf(line, "### Phrase %X", &idx) != 1) return 1;
    consumeLine(fileId);

    // Skip opening ```
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '`') {
      consumeLine(fileId);
    }

    for (int c = 0; c < 16; c++) {
      line = peekLine(fileId);
      if (line == NULL) return 1;
      if (strlen(line) != 30) return 1;
      // Note
      p->phrases[idx].rows[c].note = scanNote(line, p);
      // Instrument
      p->phrases[idx].rows[c].instrument = scanByteOrEmpty(line + 4);
      // Volume
      p->phrases[idx].rows[c].volume = scanByteOrEmpty(line + 7);
      // FX
      for (int d = 0; d < 3; d++) {
        p->phrases[idx].rows[c].fx[d][0] = scanFX(line + 10 + d * 7, p);
        p->phrases[idx].rows[c].fx[d][1] = scanByteOrEmpty(line + 14 + d * 7);
      }
      consumeLine(fileId);
    }

    // Skip closing ```
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '`') {
      consumeLine(fileId);
    }
  }

  return 0;
}

static int projectLoadInstruments(int fileId, Project* p) {
  int idx;

  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Instruments")) return 1;
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  while (strncmp(line, "### Instrument", 14) == 0) {
    if (sscanf(line, "### Instrument %X", &idx) != 1) return 1;
    consumeLine(fileId);
    if (instrumentLoadData(fileId, &p->instruments[idx], p)) return 1;
    line = peekLine(fileId);
    if (line == NULL) return 1;
  }

  return 0;
}

static int loadTable(int fileId, Table* table, Project* p) {
  // Skip opening ```
  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (line[0] == '`') {
    consumeLine(fileId);
  }

  for (int d = 0; d < 16; d++) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (strlen(line) < 35) return 1;  // Minimum length check

    // Pitch flag
    table->rows[d].pitchFlag = (line[0] == '=') ? 1 : 0;
    // Pitch offset
    table->rows[d].pitchOffset = scanByteOrEmpty(line + 2);
    // Volume
    table->rows[d].volume = scanByteOrEmpty(line + 5);
    // FX
    for (int e = 0; e < 4; e++) {
      table->rows[d].fx[e][0] = scanFX(line + 8 + e * 7, p);
      table->rows[d].fx[e][1] = scanByteOrEmpty(line + 12 + e * 7);
    }
    consumeLine(fileId);
  }

  // Skip closing ```
  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (line[0] == '`') {
    consumeLine(fileId);
  }

  return 0;
}

static int projectLoadTables(int fileId, Project* p) {
  int idx;

  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strcmp(line, "## Tables")) return 1;
  consumeLine(fileId);

  while (1) {
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (strncmp(line, "### Table", 9)) break;
    if (sscanf(line, "### Table %X", &idx) != 1) return 1;
    consumeLine(fileId);
    if (loadTable(fileId, &p->tables[idx], p)) return 1;
  }

  return 0;
}

static int projectLoadInternal(int fileId, Project* project) {
  char buf[128];
  Project p;
  projectInit(&p);

  sprintf(projectFileError, "Module header");
  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strncmp(line, "# ChipNomad Tracker Module", 26)) {
    sprintf(projectFileError, "Incorrect module format");
    return 1;
  }

  // Detect version
  if (strlen(line) > 26) {
    if (strncmp(line + 26, " 2.0", 4) == 0) {
      projectFileVersion = 2;
    } else if (strncmp(line + 26, " 1.0", 4) == 0) {
      projectFileVersion = 1;
    } else {
      sprintf(projectFileError, "Incorrect module version");
      return 1;
    }
  } else {
    // No version specified = 1.0 (legacy)
    projectFileVersion = 1;
  }
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (!strncmp(line, "- Title:", 8)) {
    if (sscanf(line, "- Title: %[^\n]", p.title) != 1) {
      sprintf(projectFileError, "Invalid title");
      return 1;
    }
  } else {
    sprintf(projectFileError, "Invalid title");
    return 1;
  }
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (!strncmp(line, "- Author:", 9)) {
    if (sscanf(line, "- Author: %[^\n]", p.author) != 1) {
      p.author[0] = 0;
    }
  }
  consumeLine(fileId);

  sprintf(projectFileError, "Invalid project settings");
  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (sscanf(line, "- Frame rate: %f", &p.tickRate) != 1) return 1;
  consumeLine(fileId);

  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (sscanf(line, "- Chips count: %d", &p.chipsCount) != 1) return 1;
  consumeLine(fileId);

  // Try to read linear pitch (optional for backwards compatibility)
  line = peekLine(fileId);
  if (line == NULL) return 1;
  int tempLinearPitch;
  if (sscanf(line, "- Linear pitch: %d", &tempLinearPitch) == 1) {
    p.linearPitch = (uint8_t)tempLinearPitch;
    consumeLine(fileId);
    line = peekLine(fileId);  // Read next line for chip type
    if (line == NULL) return 1;
  }
  // If linear pitch not found, line already contains the chip type line

  // Chip type
  if (sscanf(line, "- Chip type: %s", buf) != 1) {
    sprintf(projectFileError, "Invalid chip type");
    return 1;
  }

  if (!strcmp(buf, "AY8910")) {
    p.chipType = chipAY;
  } else {
    sprintf(projectFileError, "Unknown chip type");
    return 1;
  }
  consumeLine(fileId);

  // Chip-specific settings
  switch (p.chipType) {
  case chipAY:
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (sscanf(line, "- *AY8910* Clock: %d", &p.chipSetup.ay.clock) != 1) return 1;
    consumeLine(fileId);

    int tempIsYM;
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (sscanf(line, "- *AY8910* AY/YM: %d", &tempIsYM) != 1) return 1;
    p.chipSetup.ay.isYM = (uint8_t)tempIsYM;
    consumeLine(fileId);

    // TODO: Remove old pan logic for the first public release
    line = peekLine(fileId);
    if (line == NULL) return 1;
    if (strncmp(line, "- *AY8910* PanA:", 15) == 0) {
      // Old pan storage
      consumeLine(fileId);
      line = peekLine(fileId);  // Skip B
      if (line == NULL) return 1;
      consumeLine(fileId);
      line = peekLine(fileId);  // Skip C
      if (line == NULL) return 1;
      consumeLine(fileId);
      line = peekLine(fileId);  // Skip stereo mode
      if (line == NULL) return 1;
      consumeLine(fileId);
      // Default to ABC
      p.chipSetup.ay.stereoMode = ayStereoABC;
      p.chipSetup.ay.stereoSeparation = 100;
    } else {
      // New stereo mode storage
      if (strncmp(line, "- *AY8910* Stereo:", 17) == 0) {
        if (sscanf(line, "- *AY8910* Stereo: %s", buf) != 1) {
          sprintf(projectFileError, "Invalid stereo mode");
          return 1;
        }
        if (!strcmp(buf, "ABC")) {
          p.chipSetup.ay.stereoMode = ayStereoABC;
        } else if (!strcmp(buf, "ACB")) {
          p.chipSetup.ay.stereoMode = ayStereoACB;
        } else if (!strcmp(buf, "BAC")) {
          p.chipSetup.ay.stereoMode = ayStereoBAC;
        } else {
          sprintf(projectFileError, "Unknown stereo mode");
          return 1;
        }
      } else {
        sprintf(projectFileError, "Invalid stereo mode");
        return 1;
      }
      consumeLine(fileId);

      line = peekLine(fileId);
      if (line == NULL) return 1;
      if (sscanf(line, "- *AY8910* Stereo separation: %hhu", &p.chipSetup.ay.stereoSeparation) != 1) return 1;
      consumeLine(fileId);
    }
    break;
  default:
    break;
  }

  p.tracksCount = projectGetTotalTracks(&p);

  sprintf(projectFileError, "Invalid pitch table");
  if (projectLoadPitchTable(fileId, &p)) return 1;
  sprintf(projectFileError, "Invalid song data");
  if (projectLoadSong(fileId, &p)) return 1;
  sprintf(projectFileError, "Invalid chain data");
  if (projectLoadChains(fileId, &p)) return 1;
  sprintf(projectFileError, "Invalid groove data");
  if (projectLoadGrooves(fileId, &p)) return 1;
  sprintf(projectFileError, "Invalid phrase data");
  if (projectLoadPhrases(fileId, &p)) return 1;
  sprintf(projectFileError, "Invalid instrument data");
  if (projectLoadInstruments(fileId, &p)) return 1;
  sprintf(projectFileError, "Invalid table data");
  if (projectLoadTables(fileId, &p)) return 1;

  *project = p;
  return 0;
}

int projectLoad(Project* p, const char* path) {
  projectFileError[0] = 0;
  resetPeekConsume();  // Ensure clean state

  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  int result = projectLoadInternal(fileId, p);
  fileClose(fileId);
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Save functions

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
  extern FXName fxNames[256];

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
      int isHighlighted = project->songHighlight[c][d];
      if (chain == EMPTY_VALUE_16) {
        filePrintf(fileId, "--");
      } else {
        filePrintf(fileId, "%s", byteToHex(chain));
      }
      // Add asterisk if highlighted, otherwise space (except after last column)
      if (isHighlighted) {
        filePrintf(fileId, "*");
      } else if (d < project->tracksCount - 1) {
        filePrintf(fileId, " ");
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
  extern FXName fxNames[256];

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

int saveTable(int fileId, int idx, Table* table) {
  extern FXName fxNames[256];

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
      instrumentSaveData(fileId, c, &project->instruments[c]);
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
  filePrintf(fileId, "# ChipNomad Tracker Module 2.0\n\n");

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

///////////////////////////////////////////////////////////////////////////////
// Instrument file I/O

int instrumentSave(Project* project, const char* path, int instrumentIdx) {
  projectFileError[0] = 0;

  int fileId = fileOpen(path, 1);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  filePrintf(fileId, "# ChipNomad Instrument 2.0\n\n");
  instrumentSaveData(fileId, 0, &project->instruments[instrumentIdx]);
  saveTable(fileId, 0, &project->tables[instrumentIdx]);

  fileClose(fileId);
  return 0;
}

static int instrumentLoadInternal(int fileId, Project* project, int instrumentIdx) {
  char* line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strncmp(line, "# ChipNomad Instrument", 22)) {
    sprintf(projectFileError, "Incorrect instrument format");
    return 1;
  }

  // Detect version
  if (strlen(line) > 22) {
    if (strncmp(line + 22, " 2.0", 4) == 0) {
      projectFileVersion = 2;
    } else if (strncmp(line + 22, " 1.0", 4) == 0) {
      projectFileVersion = 1;
    } else {
      sprintf(projectFileError, "Incorrect instrument version");
      return 1;
    }
  } else {
    // No version specified = 1.0 (legacy)
    projectFileVersion = 1;
  }
  consumeLine(fileId);

  sprintf(projectFileError, "Invalid instrument data");
  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strncmp(line, "### Instrument", 14)) return 1;
  consumeLine(fileId);

  if (instrumentLoadData(fileId, &project->instruments[instrumentIdx], project)) return 1;
  // instrumentLoadData leaves the next section header in peek buffer
  sprintf(projectFileError, "Invalid table data");
  line = peekLine(fileId);
  if (line == NULL) return 1;
  if (strncmp(line, "### Table", 9)) return 1;
  consumeLine(fileId);
  if (loadTable(fileId, &project->tables[instrumentIdx], project)) return 1;

  return 0;
}

int instrumentLoad(Project* project, const char* path, int instrumentIdx) {
  projectFileError[0] = 0;
  resetPeekConsume();  // Ensure clean state

  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    sprintf(projectFileError, "Can't open file");
    return 1;
  }

  int result = instrumentLoadInternal(fileId, project, instrumentIdx);
  fileClose(fileId);
  return result;
}
