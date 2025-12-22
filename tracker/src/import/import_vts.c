#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "chipnomad_lib.h"
#include "corelib/corelib_file.h"
#include "common.h"
#include "import_common.h"

#define VTS_COL_TONE        0
#define VTS_COL_NOISE       1
#define VTS_COL_ENVELOPE    2
#define VTS_COL_PITCH       4
#define VTS_COL_NOISE_OFFSET 10
#define VTS_COL_VOLUME      15
#define VTS_COL_NOISE_FREQ  16
#define VTS_COL_LOOP_START  16
#define VTS_COL_LOOP_END    25

#define VTS_PITCH_THRESHOLD 256
#define CHIPNOMAD_PIT_MAX   127
#define CHIPNOMAD_PIT_MIN   -128
#define TABLE_ROW_COUNT     16
#define VTS_REFERENCE_NOTE  "C-4"

#define FX_SLOT_CONTROL  0
#define FX_SLOT_NOISE    1
#define FX_SLOT_MIXER    2
#define FX_SLOT_PITCH    3

#define AYM_TONE_BIT     0x01
#define AYM_NOISE_BIT    0x02
#define AYM_ENVELOPE_BIT 0xC0

static int parseVTSHex(char c) {
  if (c == '_') return 0;
  return parseHexChar(c);
}

static int parseVTSOffset(const char* str) {
  int sign = 1;
  int value = 0;

  if (str[0] == '-') sign = -1;
  if (str[0] == '+') sign = 1;

  for (int i = 1; i <= 3; i++) {
    if (str[i] == '_') break;

    int digit = 0;
    if (str[i] >= '0' && str[i] <= '9') {
      digit = str[i] - '0';
    } else if (str[i] >= 'A' && str[i] <= 'F') {
      digit = str[i] - 'A' + 10;
    } else if (str[i] >= 'a' && str[i] <= 'f') {
      digit = str[i] - 'a' + 10;
    } else {
      break;
    }

    value = value * 16 + digit;
  }

  return sign * value;
}


static inline int isValidDataChar(char c) {
  return c != '_' && c != ' ' && c != '-';
}

static inline int clampToInt8(int value) {
  if (value > CHIPNOMAD_PIT_MAX) return CHIPNOMAD_PIT_MAX;
  if (value < CHIPNOMAD_PIT_MIN) return CHIPNOMAD_PIT_MIN;
  return value;
}

static void setPitEffect(TableRow* row, int pitValue) {
  row->fx[FX_SLOT_PITCH][0] = fxPIT;
  row->fx[FX_SLOT_PITCH][1] = (uint8_t)pitValue;
}

static int findReferenceNote() {
  for (int i = 0; i < chipnomadState->project.pitchTable.length; i++) {
    if (strcmp(chipnomadState->project.pitchTable.noteNames[i], VTS_REFERENCE_NOTE) == 0) {
      return i;
    }
  }
  return chipnomadState->project.pitchTable.length / 2;
}

static int calculateSemitonesFromOffset(int offset, int referenceNoteIdx, int* outAccumulatedPeriod) {
  int semitones = 0;
  int accumulatedPeriod = 0;
  int currentNote = referenceNoteIdx;
  int direction = (offset > 0) ? -1 : 1;
  int absOffset = abs(offset);
  
  while (currentNote >= 0 && currentNote < chipnomadState->project.pitchTable.length - 1) {
    int nextNote = currentNote + direction;
    if (nextNote < 0 || nextNote >= chipnomadState->project.pitchTable.length) break;
    
    int stepSize = abs(chipnomadState->project.pitchTable.values[currentNote] -
      chipnomadState->project.pitchTable.values[nextNote]);
    
    if (accumulatedPeriod + stepSize > absOffset) break;
    
    accumulatedPeriod += stepSize;
    semitones++;
    currentNote = nextNote;
  }
  
  semitones *= direction;
  
  if (outAccumulatedPeriod != NULL) {
    *outAccumulatedPeriod = direction * accumulatedPeriod;
  }
  
  return semitones;
}

// Convert VTS pitch offsets to ChipNomad pitch commands for a table
static void convertVTSPitchOffsets(Table* table, int* vtsOffsets, int* vtsOffsetTypes, int rowCount, int referenceNoteIdx) {
  for (int i = 0; i < rowCount; i++) {
    int currentVTSOffset = vtsOffsets[i];
    int prevOffset = (i > 0) ? vtsOffsets[i-1] : 0;
    int delta = currentVTSOffset - prevOffset;

    if (vtsOffsetTypes[i] == 1) {
      // Accumulating PIT offset (^pitch) - use PIT directly
      if (delta != 0) {
        int pitValue = clampToInt8(-delta);
        setPitEffect(&table->rows[i], pitValue);
      }
    } else {
      // Regular semitone offset (+pitch) - convert to semitones + PIT
      if (abs(currentVTSOffset) < VTS_PITCH_THRESHOLD) {
        if (delta != 0) {
          int pitValue = clampToInt8(-delta);
          setPitEffect(&table->rows[i], pitValue);
        }
      } else {
        int currentAccumulated = 0;
        int semitones = calculateSemitonesFromOffset(currentVTSOffset, referenceNoteIdx, &currentAccumulated);

        int prevAccumulated = 0;
        calculateSemitonesFromOffset(prevOffset, referenceNoteIdx, &prevAccumulated);

        table->rows[i].pitchOffset = (int8_t)clampToInt8(semitones);

        int semitoneDelta = currentAccumulated - prevAccumulated;
        int pitDelta = delta - semitoneDelta;

        if (pitDelta != 0) {
          int pitValue = clampToInt8(-pitDelta);
          setPitEffect(&table->rows[i], pitValue);
        }
      }
    }
  }
}

static void parseVTSLine(const char* line, TableRow* row) {
  initTableRow(row);

  size_t len = strlen(line);
  if (len < VTS_COL_VOLUME + 1) return;

  int hasTone = (line[VTS_COL_TONE] == 'T' || line[VTS_COL_TONE] == 't');
  int hasNoise = (line[VTS_COL_NOISE] == 'N' || line[VTS_COL_NOISE] == 'n');
  int hasEnv = (line[VTS_COL_ENVELOPE] == 'E' || line[VTS_COL_ENVELOPE] == 'e');

  if (isValidDataChar(line[VTS_COL_VOLUME])) {
    row->volume = parseVTSHex(line[VTS_COL_VOLUME]);
  }

  if (len > VTS_COL_VOLUME + 1) {
    char volModifier = line[VTS_COL_VOLUME + 1];
    if (volModifier == '-') {
      row->fx[FX_SLOT_CONTROL][0] = fxVOL;
      row->fx[FX_SLOT_CONTROL][1] = 0xFF;
    } else if (volModifier == '+') {
      row->fx[FX_SLOT_CONTROL][0] = fxVOL;
      row->fx[FX_SLOT_CONTROL][1] = 0x01;
    }
  }

  if (len >= VTS_COL_NOISE_OFFSET + 4 &&
    (line[VTS_COL_NOISE_OFFSET] == '+' || line[VTS_COL_NOISE_OFFSET] == '-')) {
    int noiseOffset = parseVTSOffset(&line[VTS_COL_NOISE_OFFSET]);
    if (noiseOffset != 0) {
      row->fx[FX_SLOT_NOISE][0] = fxNOA;
      row->fx[FX_SLOT_NOISE][1] = (uint8_t)(int8_t)noiseOffset;
    }
  } else if (hasNoise && len > VTS_COL_NOISE_FREQ) {
    char noiseChar = line[VTS_COL_NOISE_FREQ];
    if (isValidDataChar(noiseChar) && noiseChar != 'L' && noiseChar != 'l') {
      row->fx[FX_SLOT_NOISE][0] = fxNOI;
      row->fx[FX_SLOT_NOISE][1] = parseVTSHex(noiseChar);
    }
  }

  int mixerMode = (hasTone ? AYM_TONE_BIT : 0) |
  (hasNoise ? AYM_NOISE_BIT : 0) |
  (hasEnv ? AYM_ENVELOPE_BIT : 0);

  row->fx[FX_SLOT_MIXER][0] = fxAYM;
  row->fx[FX_SLOT_MIXER][1] = mixerMode;
}

static void initAYInstrument(Instrument* inst) {
  inst->type = instAY;
  inst->tableSpeed = 1;
  inst->transposeEnabled = 1;

  inst->chip.ay.veA = 0;
  inst->chip.ay.veD = 0;
  inst->chip.ay.veS = 15;
  inst->chip.ay.veR = 0;
  inst->chip.ay.autoEnvN = 0;
  inst->chip.ay.autoEnvD = 0;
}

static void extractInstrumentName(const char* line, const char* path, char* outName, size_t maxLen) {
  if (line[0] == '[') {
    size_t len = strlen(line);
    size_t nameLen = 0;
    for (size_t i = 1; i < len && line[i] != ']' && nameLen < maxLen - 1; i++) {
      outName[nameLen++] = line[i];
    }
    outName[nameLen] = '\0';
  } else {
    const char* filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    strncpy(outName, filename, maxLen - 1);
    outName[maxLen - 1] = '\0';

    char* ext = strstr(outName, ".vts");
    if (ext) *ext = '\0';
  }
}

static int findLoopMarker(const char* line) {
  size_t len = strlen(line);
  for (size_t i = VTS_COL_LOOP_START; i < len && i < VTS_COL_LOOP_END; i++) {
    if (line[i] == 'L' || line[i] == 'l') {
      return 1;
    }
  }
  return 0;
}

static void parsePitchOffset(const char* line, size_t lineLen, int* offset, int* offsetType) {
  *offset = 0;
  *offsetType = 0;
  
  if (lineLen < VTS_COL_PITCH + 5) return;
  
  char pitchChar = line[VTS_COL_PITCH];
  if (pitchChar == '+' || pitchChar == '-') {
    *offset = parseVTSOffset(&line[VTS_COL_PITCH]);
    *offsetType = 0;
  } else if (pitchChar == '^') {
    *offset = parseVTSOffset(&line[VTS_COL_PITCH]);
    *offsetType = 1;
  }
}

static void processVTSLine(Table* table, const char* line, int rowIdx, 
                           int* loopRow, int* vtsOffsets, int* vtsOffsetTypes) {
  size_t lineLen = strlen(line);
  if (lineLen < 3) return;

  if (findLoopMarker(line)) {
    *loopRow = rowIdx;
  }

  parsePitchOffset(line, lineLen, &vtsOffsets[rowIdx], &vtsOffsetTypes[rowIdx]);
  parseVTSLine(line, &table->rows[rowIdx]);
}

static void finalizeVTSInstrument(Table* table, int rowCount, int loopRow, 
                                  int* vtsOffsets, int* vtsOffsetTypes) {
  int referenceNoteIdx = findReferenceNote();
  convertVTSPitchOffsets(table, vtsOffsets, vtsOffsetTypes, rowCount, referenceNoteIdx);

  if (loopRow >= 0 && rowCount < TABLE_ROW_COUNT) {
    initTableRow(&table->rows[rowCount]);
    table->rows[rowCount].fx[FX_SLOT_CONTROL][0] = fxTHO;
    table->rows[rowCount].fx[FX_SLOT_CONTROL][1] = loopRow;
    rowCount++;
  }

  for (int i = rowCount; i < TABLE_ROW_COUNT; i++) {
    initTableRow(&table->rows[i]);
  }
}

int instrumentLoadVTS(const char* path, int instrumentIdx) {
  if (instrumentIdx < 0 || instrumentIdx >= PROJECT_MAX_INSTRUMENTS) {
    return 1;
  }

  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    return 1;
  }

  Instrument* inst = &chipnomadState->project.instruments[instrumentIdx];
  Table* table = &chipnomadState->project.tables[instrumentIdx];

  initAYInstrument(inst);

  char* lpstr = fileReadString(fileId);
  if (lpstr == NULL) {
    fileClose(fileId);
    return 1;
  }

  extractInstrumentName(lpstr, path, inst->name, PROJECT_INSTRUMENT_NAME_LENGTH);

  int rowCount = 0;
  int loopRow = -1;
  int vtsOffsets[TABLE_ROW_COUNT] = {0};
  int vtsOffsetTypes[TABLE_ROW_COUNT] = {0};

  while (rowCount < TABLE_ROW_COUNT) {
    lpstr = fileReadString(fileId);
    if (lpstr == NULL || lpstr[0] == '[') break;

    processVTSLine(table, lpstr, rowCount, &loopRow, vtsOffsets, vtsOffsetTypes);
    rowCount++;
  }

  finalizeVTSInstrument(table, rowCount, loopRow, vtsOffsets, vtsOffsetTypes);

  fileClose(fileId);
  return 0;
}

int instrumentLoadVTSFromMemory(char** lines, int lineCount, int instrumentIdx, const char* instrumentName) {
  if (instrumentIdx < 0 || instrumentIdx >= PROJECT_MAX_INSTRUMENTS) {
    return 1;
  }

  Instrument* inst = &chipnomadState->project.instruments[instrumentIdx];
  Table* table = &chipnomadState->project.tables[instrumentIdx];

  initAYInstrument(inst);

  if (instrumentName != NULL) {
    strncpy(inst->name, instrumentName, PROJECT_INSTRUMENT_NAME_LENGTH);
    inst->name[PROJECT_INSTRUMENT_NAME_LENGTH] = '\0';
  } else {
    snprintf(inst->name, PROJECT_INSTRUMENT_NAME_LENGTH + 1, "Sample%02d", instrumentIdx + 1);
  }

  int rowCount = 0;
  int loopRow = -1;
  int vtsOffsets[TABLE_ROW_COUNT] = {0};
  int vtsOffsetTypes[TABLE_ROW_COUNT] = {0};

  for (int i = 0; i < lineCount && rowCount < TABLE_ROW_COUNT; i++) {
    const char* lpstr = lines[i];
    if (lpstr == NULL) break;

    processVTSLine(table, lpstr, rowCount, &loopRow, vtsOffsets, vtsOffsetTypes);
    rowCount++;
  }

  finalizeVTSInstrument(table, rowCount, loopRow, vtsOffsets, vtsOffsetTypes);

  return 0;
}
