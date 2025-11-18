#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <project.h>
#include <corelib_file.h>

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
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
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

static void initTableRow(struct TableRow* row) {
  row->pitchFlag = 0;
  row->pitchOffset = 0;
  row->volume = EMPTY_VALUE_8;
  for (int i = 0; i < 4; i++) {
    row->fx[i][0] = EMPTY_VALUE_8;
    row->fx[i][1] = 0;
  }
}

static inline int isValidDataChar(char c) {
  return c != '_' && c != ' ' && c != '-';
}

static inline int clampToInt8(int value) {
  if (value > CHIPNOMAD_PIT_MAX) return CHIPNOMAD_PIT_MAX;
  if (value < CHIPNOMAD_PIT_MIN) return CHIPNOMAD_PIT_MIN;
  return value;
}

static void setPitEffect(struct TableRow* row, int pitValue) {
  row->fx[FX_SLOT_PITCH][0] = fxPIT;
  row->fx[FX_SLOT_PITCH][1] = (uint8_t)pitValue;
}

static int findReferenceNote() {
  // Find Ref note in the pitch table
  // This will be used as anchor for absolute pitch mode
  for (int i = 0; i < project.pitchTable.length; i++) {
    if (strcmp(project.pitchTable.noteNames[i], VTS_REFERENCE_NOTE) == 0) {
      return i;
    }
  }
  return project.pitchTable.length / 2;
}

static int getReferenceNotePeriod(int referenceNoteIdx) {
  if (referenceNoteIdx >= 0 && referenceNoteIdx < project.pitchTable.length) {
    return project.pitchTable.values[referenceNoteIdx];
  }
  return 1000;
}

static void parseVTSLine(const char* line, struct TableRow* row) {
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

static void initAYInstrument(struct Instrument* inst) {
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

int instrumentLoadVTS(const char* path, int instrumentIdx) {
  if (instrumentIdx < 0 || instrumentIdx >= PROJECT_MAX_INSTRUMENTS) {
    return 1;
  }
  
  int fileId = fileOpen(path, 0);
  if (fileId == -1) {
    return 1;
  }
  
  struct Instrument* inst = &project.instruments[instrumentIdx];
  struct Table* table = &project.tables[instrumentIdx];
  
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
  
  while (rowCount < TABLE_ROW_COUNT) {
    lpstr = fileReadString(fileId);
    if (lpstr == NULL || lpstr[0] == '[') break;
    
    size_t lineLen = strlen(lpstr);
    if (lineLen < 3) continue;
    
    if (findLoopMarker(lpstr)) {
      loopRow = rowCount;
    }
    
    if (lineLen >= VTS_COL_PITCH + 5 && 
        (lpstr[VTS_COL_PITCH] == '+' || lpstr[VTS_COL_PITCH] == '-')) {
      vtsOffsets[rowCount] = parseVTSOffset(&lpstr[VTS_COL_PITCH]);
    }
    
    parseVTSLine(lpstr, &table->rows[rowCount]);
    rowCount++;
  }
  
  /* Convert VTS pitch offsets to ChipNomad pitch commands
     Strategy:
     - Below threshold: Use PIT command (relative pitch adjustment, 128 range)
     - Above threshold: Use absolute note (C-4 reference) + or - PIT for fine-tuning

  Note: VTS offsets are cumulative/absolute (relative to 0), but we convert to deltas*/ 
  
  int referenceNoteIdx = findReferenceNote();
  int referencePeriod = getReferenceNotePeriod(referenceNoteIdx);
  
  for (int i = 0; i < rowCount; i++) {
    int currentVTSOffset = vtsOffsets[i];
    int prevOffset = (i > 0) ? vtsOffsets[i-1] : 0;
    int delta = currentVTSOffset - prevOffset;
    
    if (abs(currentVTSOffset) < VTS_PITCH_THRESHOLD) {
      if (delta != 0) {
        int pitValue = clampToInt8(-delta);
        setPitEffect(&table->rows[i], pitValue);
      }
    } else {
      
      int semitones = 0;
      int accumulatedPeriod = 0;
      int currentNote = referenceNoteIdx;
      int direction = (currentVTSOffset > 0) ? -1 : 1;
      int absOffset = abs(currentVTSOffset);
      
      // Walk pitch table to find how many whole semitones fit in the absolute VTS offset
      while (currentNote >= 0 && currentNote < project.pitchTable.length - 1) {
        int nextNote = currentNote + direction;
        if (nextNote < 0 || nextNote >= project.pitchTable.length) break;
        
        int stepSize = abs(project.pitchTable.values[currentNote] - 
                          project.pitchTable.values[nextNote]);
        
        if (accumulatedPeriod + stepSize > absOffset) break;
        
        accumulatedPeriod += stepSize;
        semitones++;
        currentNote = nextNote;
      }
      
      semitones *= direction;
      
      int prevSemitones = 0;
      int prevAccumulated = 0;
      int prevNote = referenceNoteIdx;
      int prevDirection = (prevOffset > 0) ? -1 : 1;
      int absPrevOffset = abs(prevOffset);
      
      while (prevNote >= 0 && prevNote < project.pitchTable.length - 1) {
        int nextNote = prevNote + prevDirection;
        if (nextNote < 0 || nextNote >= project.pitchTable.length) break;
        
        int stepSize = abs(project.pitchTable.values[prevNote] - 
                          project.pitchTable.values[nextNote]);
        
        if (prevAccumulated + stepSize > absPrevOffset) break;
        
        prevAccumulated += stepSize;
        prevSemitones++;
        prevNote = nextNote;
      }
      
      prevSemitones *= prevDirection;
      
      table->rows[i].pitchOffset = (int8_t)clampToInt8(semitones);
      
      int semitoneDelta = (direction * accumulatedPeriod) - (prevDirection * prevAccumulated);
      int pitDelta = delta - semitoneDelta;
      
      if (pitDelta != 0) {
        int pitValue = clampToInt8(-pitDelta);
        setPitEffect(&table->rows[i], pitValue);
      }
    }
  }
  
  if (loopRow >= 0 && rowCount < TABLE_ROW_COUNT) {
    initTableRow(&table->rows[rowCount]);
    table->rows[rowCount].fx[FX_SLOT_CONTROL][0] = fxTHO;
    table->rows[rowCount].fx[FX_SLOT_CONTROL][1] = loopRow;
    rowCount++;
  }
  
  for (int i = rowCount; i < TABLE_ROW_COUNT; i++) {
    initTableRow(&table->rows[i]);
  }
  
  fileClose(fileId);
  return 0;
}

