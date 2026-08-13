#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "project.h"
#include "project_io_common.h"
#include "utils.h"
#include "error.h"

// Shared state
int projectFileVersion = 2;  // Default to current version
static char chipNames[][16] = { "AY8910" };

// Peek/consume implementation - single global buffer (ChipNomad is single-threaded)
static char lineBuffer[1024];
static char* currentLine = NULL;
static int isConsumed = 1;

void resetPeekConsume(void) {
  currentLine = NULL;
  isConsumed = 1;
}

// Helper: Read and trim a line from file
static char* readAndTrimLine(FILE* file) {
  char* result = fgets(lineBuffer, sizeof(lineBuffer), file);
  if (result == NULL) return NULL;

  // Trim trailing whitespace
  int idx = strlen(lineBuffer) - 1;
  while (idx >= 0 && isspace((unsigned char)lineBuffer[idx])) {
    lineBuffer[idx] = 0;
    idx--;
  }

  return lineBuffer;
}

char* peekLine(FILE* file) {
  // If current line is consumed, read next non-empty line
  if (isConsumed) {
    while (1) {
      currentLine = readAndTrimLine(file);
      if (currentLine == NULL) {
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

void consumeLine(FILE* file) {
  (void)file;  // Unused in single-buffer implementation
  isConsumed = 1;
}

// Binary data encoding/decoding functions
// 6-bit encoding character set (64 printable ASCII characters)
static const char* encodeTable = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";

// Save binary data with 6-bit text encoding
// Encodes 3 bytes into 4 characters, 80 chars per line = 60 bytes per line
// Note: Caller is responsible for writing the section header (#### Section Name)
int saveBinaryData(FILE* file, const uint8_t* data, uint16_t dataLen) {
  if (dataLen == 0 || data == NULL) {
    return 1;  // Nothing to save
  }

  // Write data header (length and data marker)
  fprintf(file, "- Length: %04X\n", dataLen);
  fprintf(file, "- Data:\n");

  char lineBuffer[81]; // 80 chars + null terminator
  int linePos = 0;

  // Encode 3 bytes into 4 characters (6 bits each)
  for (uint16_t i = 0; i < dataLen; i += 3) {
    uint32_t triple = 0;
    int bytesInGroup = 0;

    // Read up to 3 bytes
    for (int j = 0; j < 3 && (i + j) < dataLen; j++) {
      triple = (triple << 8) | data[i + j];
      bytesInGroup++;
    }

    // Pad with zeros if less than 3 bytes
    triple <<= (3 - bytesInGroup) * 8;

    // Extract 4 6-bit values
    for (int j = 0; j < 4; j++) {
      lineBuffer[linePos++] = encodeTable[(triple >> (18 - j * 6)) & 0x3F];

      // Write line when it reaches 80 characters
      if (linePos == 80) {
        lineBuffer[linePos] = '\0';
        fprintf(file, "%s\n", lineBuffer);
        linePos = 0;
      }
    }
  }

  // Write remaining characters
  if (linePos > 0) {
    lineBuffer[linePos] = '\0';
    fprintf(file, "%s\n", lineBuffer);
  }

  return 1;
}

// Load binary data from a #### section
// Expects to be called when "- Data:" line has been consumed
// Returns allocated buffer in outData (caller must free), length in outLen
int loadBinaryData(FILE* file, uint8_t** outData, uint16_t* outLen, uint16_t maxLen) {
  // Initialize decode table (static, initialized once)
  static int8_t decodeTable[256];
  static int decodeTableInitialized = 0;

  if (!decodeTableInitialized) {
    memset(decodeTable, -1, sizeof(decodeTable));
    for (int i = 0; i < 64; i++) {
      decodeTable[(uint8_t)encodeTable[i]] = i;
    }
    decodeTableInitialized = 1;
  }

  if (*outLen == 0 || *outLen > maxLen) {
    chipnomad::Error::set("Invalid data length");
    return 0;
  }

  // Allocate buffer
  uint8_t* buffer = (uint8_t*)malloc(*outLen);
  if (buffer == NULL) {
    chipnomad::Error::set("Memory allocation failed");
    return 0;
  }

  uint16_t bufferPos = 0;

  // Read encoded data lines
  while (bufferPos < *outLen) {
    char* line = peekLine(file);
    if (line == NULL) break;

    // Stop if we hit a section header or end of data
    if (line[0] == '#' || line[0] == '-') {
      break;
    }

    // Decode line (each 4 characters encode 3 bytes)
    int lineLen = strlen(line);
    for (int i = 0; i < lineLen && bufferPos < *outLen; i += 4) {
      // Read 4 6-bit values
      int8_t v[4] = {0, 0, 0, 0};
      int validChars = 0;
      for (int j = 0; j < 4 && (i + j) < lineLen; j++) {
        v[j] = decodeTable[(uint8_t)line[i + j]];
        if (v[j] >= 0) validChars++;
      }

      // Decode to 3 bytes if we have valid characters
      if (validChars >= 2) {
        uint32_t triple = ((uint32_t)v[0] << 18) | ((uint32_t)v[1] << 12) |
                         ((uint32_t)v[2] << 6) | (uint32_t)v[3];

        // Extract bytes
        if (bufferPos < *outLen) {
          buffer[bufferPos++] = (triple >> 16) & 0xFF;
        }
        if (bufferPos < *outLen && validChars >= 3) {
          buffer[bufferPos++] = (triple >> 8) & 0xFF;
        }
        if (bufferPos < *outLen && validChars >= 4) {
          buffer[bufferPos++] = triple & 0xFF;
        }
      }
    }

    consumeLine(file);
  }

  *outData = buffer;
  return 1;
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

static void projectLoadPitchTable(FILE* file, Project* p) {
  static const char *pitchTableError = "Invalid pitch table format";

  char buf[128];

  char* line = peekLine(file);
  if (line == NULL) throw pitchTableError;
  if (strcmp(line, "## Pitch table")) throw pitchTableError;
  consumeLine(file);

  line = peekLine(file);
  if (line == NULL) throw pitchTableError;
  if (sscanf(line, "- Title: %[^\n]", p->pitchTable.name) != 1) throw pitchTableError;
  consumeLine(file);

  line = peekLine(file);
  if (line == NULL) throw pitchTableError;
  consumeLine(file);  // Skip opening ```

  int idx = 0;
  int period;
  while (1) {
    line = peekLine(file);
    if (line == NULL) throw pitchTableError;
    if (sscanf(line, "%s %d", buf, &period) != 2) {
      // Couldn't parse - must be closing ``` or next section
      break;
    }
    if (strlen(buf) != 3) throw pitchTableError;
    strcpy(p->pitchTable.noteNames[idx], buf);
    p->pitchTable.values[idx] = period;
    idx++;
    consumeLine(file);
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
    consumeLine(file);
  }
}

static void projectLoadSong(FILE* file, Project* p) {
  static const char *songError = "Invalid song data format";

  char buf[4];
  char* line = peekLine(file);
  if (line == NULL) throw songError;
  if (strcmp(line, "## Song")) throw songError;
  consumeLine(file);

  line = peekLine(file);
  if (line == NULL) throw songError;
  consumeLine(file);  // Skip opening ```

  int idx = 0;
  while (1) {
    line = peekLine(file);
    if (line == NULL) throw songError;
    if (line[0] == '#' || line[0] == '`') break;  // Next section or closing ```
    // Minimum length: 2 chars per cell, plus 1 separator for each cell (optional for last)
    if (strlen(line) < (p->tracksCount * 2 + p->tracksCount - 1)) throw songError;

    for (int c = 0; c < p->tracksCount; c++) {
      buf[0] = line[c * 3];
      buf[1] = line[c * 3 + 1];
      buf[2] = 0;
      if (buf[0] == '-' && buf[1] == '-') {
        p->song[idx][c] = EMPTY_VALUE_16;
      } else {
        uint16_t result;
        if (sscanf(buf, "%hX", &result) != 1) throw songError;
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
    consumeLine(file);
  }

  // Consume closing ``` if present
  if (line[0] == '`') {
    consumeLine(file);
  }
}

static void projectLoadChains(FILE* file, Project* p) {
  static const char* chainsError = "Invalid chains data format";

  int idx;

  char* line = peekLine(file);
  if (line == NULL) throw chainsError;
  if (strcmp(line, "## Chains")) throw chainsError;
  consumeLine(file);

  while (1) {
    line = peekLine(file);
    if (line == NULL) throw chainsError;
    if (strncmp(line, "### Chain", 9)) break;
    if (sscanf(line, "### Chain %X", &idx) != 1) throw chainsError;
    consumeLine(file);

    // Skip opening ```
    line = peekLine(file);
    if (line == NULL) throw chainsError;
    if (line[0] == '`') {
      consumeLine(file);
    }

    for (int c = 0; c < 16; c++) {
      line = peekLine(file);
      if (line == NULL) throw chainsError;
      if (strlen(line) != 6) throw chainsError;

      if (line[0] == '-' && line[1] == '-' && line[2] == '-') {
        p->chains[idx].rows[c].phrase = EMPTY_VALUE_16;
      } else {
        uint16_t result;
        if (sscanf(line, "%hX", &result) != 1) throw chainsError;
        p->chains[idx].rows[c].phrase = result;
      }
      p->chains[idx].rows[c].transpose = scanByteOrEmpty(line + 4);
      consumeLine(file);
    }

    // Skip closing ```
    line = peekLine(file);
    if (line == NULL) throw chainsError;
    if (line[0] == '`') {
      consumeLine(file);
    }
  }
}

static void projectLoadGrooves(FILE* file, Project* p) {
  static const char* groovesError = "Invalid grooves data format";

  int idx;

  char* line = peekLine(file);
  if (line == NULL) throw groovesError;
  if (strcmp(line, "## Grooves")) throw groovesError;
  consumeLine(file);

  while (1) {
    line = peekLine(file);
    if (line == NULL) throw groovesError;
    if (strncmp(line, "### Groove", 10)) break;
    if (sscanf(line, "### Groove %X", &idx) != 1) throw groovesError;
    consumeLine(file);

    // Skip opening ```
    line = peekLine(file);
    if (line == NULL) throw groovesError;
    if (line[0] == '`') {
      consumeLine(file);
    }

    for (int c = 0; c < 16; c++) {
      line = peekLine(file);
      if (line == NULL) throw groovesError;
      if (strlen(line) != 2) throw groovesError;
      p->grooves[idx].speed[c] = scanByteOrEmpty(line);
      consumeLine(file);
    }

    // Skip closing ```
    line = peekLine(file);
    if (line == NULL) throw groovesError;
    if (line[0] == '`') {
      consumeLine(file);
    }
  }
}

static void projectLoadPhrases(FILE* file, Project* p) {
  static const char* phrasesError = "Invalid phrases data format";

  int idx;

  char* line = peekLine(file);
  if (line == NULL) throw phrasesError;
  if (strcmp(line, "## Phrases")) throw phrasesError;
  consumeLine(file);

  while (1) {
    line = peekLine(file);
    if (line == NULL) throw phrasesError;
    if (strncmp(line, "### Phrase", 10)) break;
    if (sscanf(line, "### Phrase %X", &idx) != 1) throw phrasesError;
    consumeLine(file);

    // Skip opening ```
    line = peekLine(file);
    if (line == NULL) throw phrasesError;
    if (line[0] == '`') {
      consumeLine(file);
    }

    for (int c = 0; c < 16; c++) {
      line = peekLine(file);
      if (line == NULL) throw phrasesError;
      if (strlen(line) != 30) throw phrasesError;
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
      consumeLine(file);
    }

    // Skip closing ```
    line = peekLine(file);
    if (line == NULL) throw phrasesError;
    if (line[0] == '`') {
      consumeLine(file);
    }
  }
}

static void projectLoadInstruments(FILE* file, Project* p) {
  static const char* instrumentsError = "Invalid instruments data format";
  int idx;

  char* line = peekLine(file);
  if (line == NULL) throw instrumentsError;
  if (strcmp(line, "## Instruments")) throw instrumentsError;
  consumeLine(file);

  line = peekLine(file);
  if (line == NULL) throw instrumentsError;
  while (strncmp(line, "### Instrument", 14) == 0) {
    if (sscanf(line, "### Instrument %X", &idx) != 1) throw instrumentsError;
    consumeLine(file);
    if (!instrumentLoadData(file, &p->instruments[idx], p)) throw instrumentsError;
    line = peekLine(file);
    if (line == NULL) throw instrumentsError;
  }
}

static void loadTable(FILE* file, Table* table, Project* p) {
  static const char* tableError = "Invalid table data format";

  // Skip opening ```
  char* line = peekLine(file);
  if (line == NULL) throw tableError;
  if (line[0] == '`') {
    consumeLine(file);
  }

  for (int d = 0; d < 16; d++) {
    line = peekLine(file);
    if (line == NULL) throw tableError;
    if (strlen(line) < 35) throw tableError;  // Minimum length check

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
    consumeLine(file);
  }

  // Skip closing ```
  line = peekLine(file);
  if (line == NULL) throw tableError;
  if (line[0] == '`') {
    consumeLine(file);
  }
}

static void projectLoadTables(FILE* file, Project* p) {
  static const char* tableError = "Invalid tables data format";
  int idx;

  char* line = peekLine(file);
  if (line == NULL) throw tableError;
  if (strcmp(line, "## Tables")) throw tableError;
  consumeLine(file);

  while (1) {
    line = peekLine(file);
    if (line == NULL) throw tableError;
    if (strncmp(line, "### Table", 9)) break;
    if (sscanf(line, "### Table %X", &idx) != 1) throw tableError;
    consumeLine(file);
    loadTable(file, &p->tables[idx], p);
  }
}

static void projectLoadAYWavetables(FILE* file, Project* p) {
  static const char* wavetableError = "Invalid AY wavetable data format";

  char* line = peekLine(file);
  if (line == NULL) throw wavetableError;

  // This section is optional for backwards compatibility
  if (strcmp(line, "## AY Wavetables")) {
    // Section not found, that's OK - just return success
    return;
  }
  consumeLine(file);

  // Read wavetable data lines until we hit EOF or another section
  while (1) {
    line = peekLine(file);
    if (line == NULL) throw wavetableError;

    // Check if we've reached EOF or another section marker
    if (!strcmp(line, "EOF") || !strncmp(line, "##", 2)) {
      break;
    }

    // Parse wavetable line: "XX 0123456789ABCDEF0123456789ABCDEF"
    int wavetableIdx;
    char data[33];  // 32 hex digits + null terminator

    if (sscanf(line, "%X %32s", &wavetableIdx, data) != 2) throw wavetableError;

    // Validate wavetable index
    if (wavetableIdx < 0 || wavetableIdx > 255) throw wavetableError;

    // Validate data length
    if (strlen(data) != 32) throw wavetableError;

    // Parse hex digits and store in wavetable
    for (int i = 0; i < 32; i++) {
      char c = data[i];
      uint8_t value;

      if (c >= '0' && c <= '9') {
        value = c - '0';
      } else if (c >= 'A' && c <= 'F') {
        value = c - 'A' + 10;
      } else if (c >= 'a' && c <= 'f') {
        value = c - 'a' + 10;
      } else {
        throw wavetableError;
      }

      p->ayWavetables[wavetableIdx][i] = value & 0x0F;
    }

    consumeLine(file);
  }
}

int projectLoad(Project* project, const char* path) {
  static const char* formatError = "Incorrect file format";

  resetPeekConsume();
  chipnomad::Error::clear();

  // Open file
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    chipnomad::Error::set("Can't open file");
    return 0;
  }

  // Parse project file
  try {
    char buf[128];
    Project p;
    projectInit(&p);

    char* line = peekLine(file);
    if (line == NULL) throw formatError;
    if (strncmp(line, "# ChipNomad Tracker Module", 26)) {
      throw "Incorrect module format";
    }

    // Detect version
    if (strlen(line) > 26) {
      if (strncmp(line + 26, " 2.0", 4) == 0) {
        projectFileVersion = 2;
      } else if (strncmp(line + 26, " 1.0", 4) == 0) {
        projectFileVersion = 1;
      } else {
        throw "Incorrect module version";
      }
    } else {
      // No version specified = 1.0 (legacy)
      projectFileVersion = 1;
    }
    consumeLine(file);

    line = peekLine(file);
    if (line == NULL) throw formatError;
    if (!strncmp(line, "- Title:", 8)) {
      if (sscanf(line, "- Title: %[^\n]", p.title) != 1) {
        // Empty title is valid, just set it to empty string
        p.title[0] = 0;
      }
    } else {
      throw "Invalid title";
    }
    consumeLine(file);

    line = peekLine(file);
    if (line == NULL) throw formatError;
    if (!strncmp(line, "- Author:", 9)) {
      if (sscanf(line, "- Author: %[^\n]", p.author) != 1) {
        p.author[0] = 0;
      }
    }
    consumeLine(file);

    chipnomad::Error::clear();
    line = peekLine(file);
    if (line == NULL) throw formatError;
    if (sscanf(line, "- Frame rate: %f", &p.tickRate) != 1) throw formatError;
    consumeLine(file);

    line = peekLine(file);
    if (line == NULL) throw formatError;
    if (sscanf(line, "- Chips count: %d", &p.chipsCount) != 1) throw formatError;
    consumeLine(file);

    // Try to read linear pitch (optional for backwards compatibility)
    line = peekLine(file);
    if (line == NULL) throw formatError;
    int tempLinearPitch;
    if (sscanf(line, "- Linear pitch: %d", &tempLinearPitch) == 1) {
      p.linearPitch = (uint8_t)tempLinearPitch;
      consumeLine(file);
      line = peekLine(file);  // Read next line for chip type
      if (line == NULL) throw formatError;
    }
    // If linear pitch not found, line already contains the chip type line

    // Chip type
    if (sscanf(line, "- Chip type: %s", buf) != 1) {
      throw "Invalid chip type";
    }

    if (!strcmp(buf, "AY8910")) {
      p.chipType = ChipType::AY;
    } else {
      throw "Unknown chip type";
    }
    consumeLine(file);

    // Chip-specific settings
    switch (p.chipType) {
    case ChipType::AY:
      line = peekLine(file);
      if (line == NULL) throw formatError;
      if (sscanf(line, "- *AY8910* Clock: %d", &p.chipSetup.ay.clock) != 1) throw formatError;
      consumeLine(file);

      int tempIsYM;
      line = peekLine(file);
      if (line == NULL) throw formatError;
      if (sscanf(line, "- *AY8910* AY/YM: %d", &tempIsYM) != 1) throw formatError;
      p.chipSetup.ay.isYM = (uint8_t)tempIsYM;
      consumeLine(file);

      line = peekLine(file);
      if (line == NULL) throw formatError;
      // AY Stereo mode
      if (strncmp(line, "- *AY8910* Stereo:", 17) == 0) {
        if (sscanf(line, "- *AY8910* Stereo: %s", buf) != 1) {
          throw "Invalid stereo mode";
        }
        if (!strcmp(buf, "ABC")) {
          p.chipSetup.ay.stereoMode = StereoModeAY::ABC;
        } else if (!strcmp(buf, "ACB")) {
          p.chipSetup.ay.stereoMode = StereoModeAY::ACB;
        } else if (!strcmp(buf, "BAC")) {
          p.chipSetup.ay.stereoMode = StereoModeAY::BAC;
        } else {
          throw "Unknown stereo mode";
        }
      } else {
        throw "Invalid stereo mode";
      }
      consumeLine(file);

      line = peekLine(file);
      if (line == NULL) throw formatError;
      if (sscanf(line, "- *AY8910* Stereo separation: %hhu", &p.chipSetup.ay.stereoSeparation) != 1) throw formatError;
      consumeLine(file);

      // PWM range (optional field for backwards compatibility)
      line = peekLine(file);
      if (line != NULL && strncmp(line, "- *AY8910* PWM range: ", 22) == 0) {
        if (sscanf(line, "- *AY8910* PWM range: %hhu", &p.chipSetup.ay.pwmFullRange) != 1) throw formatError;
        consumeLine(file);
      } else {
        // Default to 16-step mode for old files
        p.chipSetup.ay.pwmFullRange = 0;
      }
      break;
    default:
      break;
    }

    p.tracksCount = projectGetTotalTracks(&p);

    projectLoadPitchTable(file, &p);
    projectLoadSong(file, &p);
    projectLoadChains(file, &p);
    projectLoadGrooves(file, &p);
    projectLoadPhrases(file, &p);
    projectLoadInstruments(file, &p);
    projectLoadTables(file, &p);
    projectLoadAYWavetables(file, &p);

    *project = p;
  } catch (const char* error) {
    chipnomad::Error::set(error);
    fclose(file);
    return 0;
  }

  fclose(file);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Save functions

static int projectSavePitchTable(FILE* file, Project* project) {
  fprintf(file, "\n## Pitch table\n\n");
  fprintf(file, "- Title: %s\n\n```\n", project->pitchTable.name);

  for (int c = 0; c < project->pitchTable.length; c++) {
    fprintf(file, "%s %d\n", project->pitchTable.noteNames[c], project->pitchTable.values[c]);
  }

  fprintf(file, "```\n");

  return 1;
}

static int projectSaveSong(FILE* file, Project* project) {
  extern FXName fxNames[256];

  fprintf(file, "\n## Song\n\n```\n");

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
        fprintf(file, "--");
      } else {
        fprintf(file, "%s", byteToHex(chain));
      }
      // Add asterisk if highlighted, otherwise space (except after last column)
      if (isHighlighted) {
        fprintf(file, "*");
      } else if (d < project->tracksCount - 1) {
        fprintf(file, " ");
      }
    }
    fprintf(file, "\n");
  }

  fprintf(file, "```\n");

  return 1;
}

static int projectSaveChains(FILE* file, Project* project) {
  fprintf(file, "\n## Chains\n");

  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    if (!chainIsEmpty(project, c)) {
      fprintf(file, "\n### Chain %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        int phrase = project->chains[c].rows[d].phrase;
        if (phrase == EMPTY_VALUE_16) {
          fprintf(file, "--- %s\n", byteToHex(project->chains[c].rows[d].transpose));
        } else {
          fprintf(file, "%03X %s\n", project->chains[c].rows[d].phrase, byteToHex(project->chains[c].rows[d].transpose));
        }
      }
      fprintf(file, "```\n");
    }
  }

  return 1;
}

static int projectSaveGrooves(FILE* file, Project* project) {
  fprintf(file, "\n## Grooves\n");

  for (int c = 0; c < PROJECT_MAX_GROOVES; c++) {
    if (!grooveIsEmpty(project, c)) {
      fprintf(file, "\n### Groove %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        fprintf(file, "%s\n", byteToHexOrEmpty(project->grooves[c].speed[d]));
      }
      fprintf(file, "```\n");
    }
  }

  return 1;
}

static int projectSavePhrases(FILE* file, Project* project) {
  extern FXName fxNames[256];

  fprintf(file, "\n## Phrases\n\n");

  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    if (!phraseIsEmpty(project, c)) {
      fprintf(file, "### Phrase %X\n\n```\n", c);
      for (int d = 0; d < 16; d++) {
        fprintf(file, "%s %s %s %s %s %s %s %s %s\n",
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
      fprintf(file, "```\n");
    }
  }

  return 1;
}

int saveTable(FILE* file, int idx, Table* table) {
  extern FXName fxNames[256];

  fprintf(file, "\n### Table %X\n\n```\n", idx);
  for (int d = 0; d < 16; d++) {
    fprintf(file, "%c %s %s %s %s %s %s %s %s %s %s\n",
      table->rows[d].pitchFlag ? '=' : '~',
      byteToHex(table->rows[d].pitchOffset),
      byteToHexOrEmpty(table->rows[d].volume),
      fxNames[table->rows[d].fx[0][0]].name, byteToHex(table->rows[d].fx[0][1]),
      fxNames[table->rows[d].fx[1][0]].name, byteToHex(table->rows[d].fx[1][1]),
      fxNames[table->rows[d].fx[2][0]].name, byteToHex(table->rows[d].fx[2][1]),
      fxNames[table->rows[d].fx[3][0]].name, byteToHex(table->rows[d].fx[3][1]));
  }
  fprintf(file, "```\n");
  return 1;
}

static int projectSaveInstruments(FILE* file, Project* project) {
  fprintf(file, "\n## Instruments\n");
  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    if (!instrumentIsEmpty(project, c)) {
      instrumentSaveData(file, c, &project->instruments[c]);
    }
  }
  return 1;
}

static int projectSaveTables(FILE* file, Project* project) {
  fprintf(file, "\n## Tables\n");
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    if (!tableIsEmpty(project, c)) {
      saveTable(file, c, &project->tables[c]);
    }
  }
  return 1;
}

static int projectSaveAYWavetables(FILE* file, Project* project) {
  // Count non-empty wavetables
  int count = 0;
  for (int i = 0; i < 256; i++) {
    if (!wavetableIsEmpty(project, i)) count++;
  }

  // Only write section if there are non-empty wavetables
  if (count == 0) return 1;

  fprintf(file, "\n## AY Wavetables\n\n");

  for (int i = 0; i < 256; i++) {
    if (!wavetableIsEmpty(project, i)) {
      // Write: "XX 0123456789ABCDEF0123456789ABCDEF"
      fprintf(file, "%02X ", i);
      for (int j = 0; j < 32; j++) {
        fprintf(file, "%X", project->ayWavetables[i][j] & 0x0F);
      }
      fprintf(file, "\n");
    }
  }

  return 1;
}

static int projectSaveInternal(FILE* file, Project* project) {
  fprintf(file, "# ChipNomad Tracker Module 2.0\n\n");

  fprintf(file, "- Title: %s\n", project->title);
  fprintf(file, "- Author: %s\n", project->author);

  fprintf(file, "- Frame rate: %f\n", project->tickRate);
  fprintf(file, "- Chips count: %d\n", project->chipsCount);
  fprintf(file, "- Linear pitch: %d\n", project->linearPitch);
  fprintf(file, "- Chip type: %s\n", chipNames[static_cast<int>(project->chipType)]);

  switch (project->chipType) {
  case ChipType::AY:
    fprintf(file, "- *AY8910* Clock: %d\n", project->chipSetup.ay.clock);
    fprintf(file, "- *AY8910* AY/YM: %d\n", project->chipSetup.ay.isYM);
    switch (project->chipSetup.ay.stereoMode) {
    case StereoModeAY::ABC:
      fprintf(file, "- *AY8910* Stereo: ABC\n");
      break;
    case StereoModeAY::ACB:
      fprintf(file, "- *AY8910* Stereo: ACB\n");
      break;
    case StereoModeAY::BAC:
      fprintf(file, "- *AY8910* Stereo: BAC\n");
      break;
    }
    fprintf(file, "- *AY8910* Stereo separation: %d\n", project->chipSetup.ay.stereoSeparation);
    fprintf(file, "- *AY8910* PWM range: %d\n", project->chipSetup.ay.pwmFullRange);
    break;
  default:
    break;
  }

  projectSavePitchTable(file, project);
  projectSaveSong(file, project);
  projectSaveChains(file, project);
  projectSaveGrooves(file, project);
  projectSavePhrases(file, project);
  projectSaveInstruments(file, project);
  projectSaveTables(file, project);
  projectSaveAYWavetables(file, project);
  fprintf(file, "EOF\n");
  return 1;
}

int projectSave(Project* p, const char* path) {
  chipnomad::Error::clear();

  FILE* file = fopen(path, "wb");
  if (file == NULL) return 0;

  int result = projectSaveInternal(file, p);
  fclose(file);
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Instrument file I/O

int instrumentSave(Project* project, const char* path, int instrumentIdx) {
  chipnomad::Error::clear();

  FILE* file = fopen(path, "wb");
  if (file == NULL) {
    chipnomad::Error::set("Can't open file");
    return 0;
  }

  fprintf(file, "# ChipNomad Instrument 2.0\n\n");
  instrumentSaveData(file, 0, &project->instruments[instrumentIdx]);
  saveTable(file, 0, &project->tables[instrumentIdx]);

  fclose(file);
  return 1;
}

static void instrumentLoadInternal(FILE* file, Project* project, int instrumentIdx) {
  const char* instrumentError = "Incorrect instrument file format";

  char* line = peekLine(file);
  if (line == NULL) throw instrumentError;
  if (strncmp(line, "# ChipNomad Instrument", 22)) throw instrumentError;

  // Detect version
  if (strlen(line) > 22) {
    if (strncmp(line + 22, " 2.0", 4) == 0) {
      projectFileVersion = 2;
    } else if (strncmp(line + 22, " 1.0", 4) == 0) {
      projectFileVersion = 1;
    } else {
      throw instrumentError;
    }
  } else {
    // No version specified = 1.0 (legacy)
    projectFileVersion = 1;
  }
  consumeLine(file);

  chipnomad::Error::set("Invalid instrument data");
  line = peekLine(file);
  if (line == NULL) throw instrumentError;
  if (strncmp(line, "### Instrument", 14)) throw instrumentError;
  consumeLine(file);

  if (!instrumentLoadData(file, &project->instruments[instrumentIdx], project)) throw instrumentError;
  // instrumentLoadData leaves the next section header in peek buffer
  chipnomad::Error::set("Invalid table data");
  line = peekLine(file);
  if (line == NULL) throw "Expected table section after instrument data";
  if (strncmp(line, "### Table", 9)) throw "Expected table section after instrument data";
  consumeLine(file);
  loadTable(file, &project->tables[instrumentIdx], project);
}

int instrumentLoad(Project* project, const char* path, int instrumentIdx) {
  resetPeekConsume();  // Ensure clean state
  chipnomad::Error::clear();

  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    chipnomad::Error::set("Can't open file");
    return 0;
  }

  try {
    instrumentLoadInternal(file, project, instrumentIdx);
  } catch (const char* error) {
    chipnomad::Error::set(error);
    fclose(file);
    return 0;
  }
  fclose(file);
  return 1;
}
