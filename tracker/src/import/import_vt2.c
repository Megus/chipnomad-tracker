#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <chipnomad_lib.h>
#include <corelib/corelib_file.h>
#include <common.h>
#include <project_utils.h>
#include "import_vt2.h"
#include "import_vts.h"
#include "pt3_tables.h"
#include "import_common.h"

#define VT2_MAX_PATTERNS 256
#define VT2_MAX_ORNAMENTS 32
#define VT2_MAX_SAMPLES 32
#define VT2_PATTERN_LENGTH 255
#define VT2_CHANNELS 3
#define PHRASE_ROWS 16
#define PHRASES_PER_PATTERN 16
#define CHAIN_MAX_ROWS 16

typedef struct InstrumentEnvelopeUsage {
  uint8_t envelopeTypes[16];
  uint8_t usesEnvelope;
} InstrumentEnvelopeUsage;

typedef struct InstrumentCloneMap {
  int8_t clonedInstrumentIdx[VT2_MAX_SAMPLES][16];
} InstrumentCloneMap;

static int importVT2Samples(const char* path, Project* project, int sampleCount);
static void trimLineEndings(char* line);
static void createMultiPhraseChain(Project* project, int chainIdx, const int* phraseIndices, int phraseCount);
static int getOrCreateSkipGroove(Project* project, uint8_t baseSpeed, int rowsToSkip);
static void setFxCommand(PhraseRow* phraseRow, int* fxSlot, uint8_t command, uint8_t param);
static void setGgrCommand(PhraseRow* phraseRow, int* fxSlot, int grooveIdx);
static void setEnvelopePeriodCommands(PhraseRow* phraseRow, int* fxSlot, uint16_t envelopePeriod);
static void getPhraseIndicesForPattern(int phraseNum, const int* phraseIndices, int* phraseA, int* phraseB, int* phraseC);
static void setupPitchTableFromVT2(Project* project, int noteTableIndex);

typedef struct VT2ChannelData {
  uint8_t hasNote;
  uint8_t note;
  uint8_t instrument;
  uint8_t ornament;
  uint8_t volume;
  uint8_t envelopeType;
  uint8_t commands[4][2];
  uint8_t commandCount;
} VT2ChannelData;

typedef struct VT2PatternRow {
  uint16_t envelopePeriod;
  uint8_t noiseMode;
  VT2ChannelData channels[VT2_CHANNELS];
} VT2PatternRow;

typedef struct VT2Pattern {
  VT2PatternRow rows[VT2_PATTERN_LENGTH];
  int rowCount;
} VT2Pattern;

typedef struct VT2Ornament {
  int8_t offsets[64];
  int length;
  int loopPoint;
} VT2Ornament;

typedef struct VT2Module {
  char title[PROJECT_TITLE_LENGTH + 1];
  char author[PROJECT_TITLE_LENGTH + 1];
  int speed;
  int noteTable;
  int playOrder[256];
  int playOrderLength;
  VT2Pattern patterns[VT2_MAX_PATTERNS];
  VT2Ornament ornaments[VT2_MAX_ORNAMENTS];
  int patternCount;
  int ornamentCount;
  int sampleCount;
} VT2Module;

static int parseHex(char c) {
  return parseHexChar(c);
}

static int parseBase26(char c) {
  return parseBase26Char(c);
}

static int getCurrentSpeed(Project* project, int* realLastGroove) {
  int currentSpeed = DEFAULT_SPEED;
  if (*realLastGroove >= 0 && *realLastGroove < PROJECT_MAX_GROOVES) {
    currentSpeed = project->grooves[*realLastGroove].speed[0];
    if (currentSpeed == EMPTY_VALUE_8) currentSpeed = DEFAULT_SPEED;
  }
  return currentSpeed;
}

static int parseNoteName(const char* noteStr) {
  if (noteStr[0] == '-' || noteStr[0] == 'R') {
    return -1;
  }
  
  int note = 0;
  char noteName = noteStr[0];
  char accidental = noteStr[1];
  char octave = noteStr[2];
  
  switch (noteName) {
    case 'C': note = 0; break;
    case 'D': note = 2; break;
    case 'E': note = 4; break;
    case 'F': note = 5; break;
    case 'G': note = 7; break;
    case 'A': note = 9; break;
    case 'B': note = 11; break;
    default: return -1;
  }
  
  if (accidental == '#') note += 1;
  
  int octaveNum = octave - '0';
  note += octaveNum * 12;
  
  if (note < 0) note = 0;
  if (note > 127) note = 127;
  
  return note;
}

static void trimLineEndings(char* line) {
  size_t len = strlen(line);
  while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
    line[--len] = '\0';
  }
}


static void createMultiPhraseChain(Project* project, int chainIdx, const int* phraseIndices, int phraseCount) {
  Chain* chain = &project->chains[chainIdx];

  for (int i = 0; i < phraseCount && i < CHAIN_MAX_ROWS; i++) {
    chain->rows[i].phrase = phraseIndices[i];
    chain->rows[i].transpose = 0;
  }

  for (int i = phraseCount; i < CHAIN_MAX_ROWS; i++) {
    chain->rows[i].phrase = EMPTY_VALUE_16;
    chain->rows[i].transpose = 0;
  }
}

static void setFxCommand(PhraseRow* phraseRow, int* fxSlot, uint8_t command, uint8_t param) {
  if (*fxSlot < MAX_FX_SLOTS) {
    phraseRow->fx[*fxSlot][0] = command;
    phraseRow->fx[*fxSlot][1] = param;
    (*fxSlot)++;
  }
}

static void setGgrCommand(PhraseRow* phraseRow, int* fxSlot, int grooveIdx) {
  for (int fx = 0; fx < MAX_FX_SLOTS; fx++) {
    if (phraseRow->fx[fx][0] == fxGGR) {
      phraseRow->fx[fx][1] = (uint8_t)grooveIdx;
      return;
    }
  }
  if (*fxSlot < MAX_FX_SLOTS) {
    phraseRow->fx[*fxSlot][0] = fxGGR;
    phraseRow->fx[*fxSlot][1] = (uint8_t)grooveIdx;
    (*fxSlot)++;
  }
}

static void setEnvelopePeriodCommands(PhraseRow* phraseRow, int* fxSlot, uint16_t envelopePeriod) {
  if (*fxSlot >= MAX_FX_SLOTS) return;
  
  phraseRow->fx[*fxSlot][0] = fxEPL;
  phraseRow->fx[*fxSlot][1] = envelopePeriod & 0xFF;
  (*fxSlot)++;

  uint8_t highByte = (envelopePeriod >> 8) & 0xFF;
  if (highByte > 0 && *fxSlot < MAX_FX_SLOTS) {
    phraseRow->fx[*fxSlot][0] = fxEPH;
    phraseRow->fx[*fxSlot][1] = highByte;
    (*fxSlot)++;
  }
}

static int getOrCreateGroove(Project* project, uint8_t speed);

static void getPhraseIndicesForPattern(int phraseNum, const int* phraseIndices, int* phraseA, int* phraseB, int* phraseC) {
  int baseIdx = phraseNum * VT2_CHANNELS;
  *phraseA = phraseIndices[baseIdx + 0];
  *phraseB = phraseIndices[baseIdx + 1];
  *phraseC = phraseIndices[baseIdx + 2];
}

static void updateChannelState(const VT2ChannelData* chData, uint8_t* currentInstrument, 
                               uint8_t* currentOrnament, uint8_t* currentVolume, 
                               int* needsInstrumentAfterOff) {
  if (chData->instrument != 0xFF && chData->instrument > 0) {
    *currentInstrument = chData->instrument;
  }
  
  if (chData->ornament != 0xFF) {
    *currentOrnament = chData->ornament;
  } else if (chData->envelopeType == 0xF) {
    *currentOrnament = 0;
  }
  
  if (chData->volume != 0xFF) {
    *currentVolume = chData->volume;
  }
  
  if (chData->hasNote && chData->note == NOTE_OFF) {
    *needsInstrumentAfterOff = 1;
  }
}

static int getFinalInstrument(int baseInstrument, int envType, const InstrumentCloneMap* cloneMap) {
  if (!cloneMap) return baseInstrument;
  
  int clonedIdx = cloneMap->clonedInstrumentIdx[baseInstrument + 1][envType];
  return (clonedIdx >= 0) ? clonedIdx : baseInstrument;
}

static void processVT2Commands(Project* project, PhraseRow* phraseRow, int* fxSlot,
                               const VT2ChannelData* chData, int* lastUsedGroove, 
                               int* realLastGroove, int startSpeedGroove,
                               int isFirstPattern, int phraseNum, int row) {
  for (int cmdIdx = 0; cmdIdx < chData->commandCount && cmdIdx < 4; cmdIdx++) {
    uint8_t cmd = chData->commands[cmdIdx][0];
    uint8_t param = chData->commands[cmdIdx][1];

    if (cmd == 0xB && param > 0) {
      int grooveIdx = getOrCreateGroove(project, param);
      if (grooveIdx >= 0) {
        setGgrCommand(phraseRow, fxSlot, grooveIdx);
        *lastUsedGroove = grooveIdx;
        *realLastGroove = grooveIdx;
      }
    } else if ((cmd == 1 || cmd == 2 || cmd == 3) && param > 0) {
      int delay = (param >> 8) & 0xFF;
      int parameter = param & 0xFF;
      int currentSpeed = getCurrentSpeed(project, realLastGroove);
      int effectiveSpeed = (delay > 0) ? parameter / delay : parameter;
      int slideValue = effectiveSpeed * currentSpeed;

      if (cmd == 1) {
        int8_t signedValue = (int8_t)(-slideValue & 0xFF);
        setFxCommand(phraseRow, fxSlot, fxPBN, (uint8_t)signedValue);
      } else if (cmd == 2) {
        setFxCommand(phraseRow, fxSlot, fxPBN, (uint8_t)(slideValue & 0x7F));
      } else if (cmd == 3) {
        setFxCommand(phraseRow, fxSlot, fxPSL, (uint8_t)parameter);
      }
    }
  }
  
  if (isFirstPattern && phraseNum == 0 && row == 0 && *lastUsedGroove < 0 && startSpeedGroove >= 0) {
    setFxCommand(phraseRow, fxSlot, fxGGR, (uint8_t)startSpeedGroove);
    *lastUsedGroove = startSpeedGroove;
    *realLastGroove = startSpeedGroove;
  }
}

static void processChannelRow(Project* project, PhraseRow* phraseRow, 
                              const VT2ChannelData* chData, const VT2PatternRow* vt2Row,
                              uint8_t* currentInstrument, uint8_t* currentOrnament, 
                              uint8_t* currentVolume, int* needsInstrumentAfterOff,
                              int ornamentBaseIdx, const InstrumentCloneMap* cloneMap,
                              int isFirstPattern, int phraseNum, int row,
                              int* lastUsedGroove, int* realLastGroove, int startSpeedGroove) {
  initEmptyPhraseRow(phraseRow);
  
  phraseRow->note = chData->hasNote ? chData->note : EMPTY_VALUE_8;
  
  updateChannelState(chData, currentInstrument, currentOrnament, currentVolume, needsInstrumentAfterOff);
  
  if (chData->hasNote && chData->note != NOTE_OFF) {
    if (*currentInstrument == 0) {
      *currentInstrument = 1;
    }
    
    int baseInstrument = *currentInstrument - 1;
    int envType = (chData->envelopeType != 0xFF) ? chData->envelopeType : 0xF;
    int finalInstrument = getFinalInstrument(baseInstrument, envType, cloneMap);
    
    phraseRow->instrument = finalInstrument;
    phraseRow->volume = *currentVolume;
    *needsInstrumentAfterOff = 0;
  } else {
    phraseRow->instrument = EMPTY_VALUE_8;
    if (chData->volume != 0xFF) {
      phraseRow->volume = chData->volume;
    } else {
      phraseRow->volume = EMPTY_VALUE_8;
    }
  }
  
  int fxSlot = 0;
  
  if (chData->hasNote && *currentOrnament > 0) {
    int ornTableIdx = ornamentBaseIdx + *currentOrnament - 1;
    if (ornTableIdx < PROJECT_MAX_TABLES) {
      setFxCommand(phraseRow, &fxSlot, fxTBX, (uint8_t)ornTableIdx);
    }
  }
  
  if (vt2Row->envelopePeriod != 0xFFFF) {
    setEnvelopePeriodCommands(phraseRow, &fxSlot, vt2Row->envelopePeriod);
  }

  if (row == 0 && !(isFirstPattern && phraseNum == 0)) {
    int grooveToSet = (*realLastGroove >= 0) ? *realLastGroove : 0;
    setGgrCommand(phraseRow, &fxSlot, grooveToSet);
  }

  processVT2Commands(project, phraseRow, &fxSlot, chData, lastUsedGroove, 
                     realLastGroove, startSpeedGroove, isFirstPattern, phraseNum, row);
  
  while (fxSlot < MAX_FX_SLOTS) {
    phraseRow->fx[fxSlot][0] = EMPTY_VALUE_8;
    phraseRow->fx[fxSlot][1] = 0;
    fxSlot++;
  }
}


static void setupPitchTableFromVT2(Project* project, int noteTableIndex) {
  if (noteTableIndex < 0 || noteTableIndex > 3) {
    noteTableIndex = 2;
  }

  const uint16_t* table = PT3ToneTables[noteTableIndex];

  strncpy(project->pitchTable.name, PT3TableNames[noteTableIndex], PROJECT_PITCH_TABLE_TITLE_LENGTH);
  project->pitchTable.name[PROJECT_PITCH_TABLE_TITLE_LENGTH] = '\0';

  project->pitchTable.octaveSize = OCTAVE_SIZE;
  project->pitchTable.length = PT3_TABLE_NOTES;

  for (int i = 0; i < PT3_TABLE_NOTES; i++) {
    project->pitchTable.values[i] = table[i] * 2;
  }

  const char* noteNames[NOTES_PER_OCTAVE] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
  for (int octave = 0; octave < PT3_TABLE_OCTAVES; octave++) {
    for (int note = 0; note < NOTES_PER_OCTAVE; note++) {
      int index = octave * NOTES_PER_OCTAVE + note;
      if (index < PROJECT_MAX_PITCHES) {
        snprintf(project->pitchTable.noteNames[index], 4, "%s%d", noteNames[note], octave);
      }
    }
  }
}

static void parseOrnamentLine(const char* line, VT2Ornament* orn) {
  orn->length = 0;
  orn->loopPoint = -1;
  
  const char* p = line;
  int idx = 0;
  
  while (*p && idx < 64) {
    while (*p && (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (!*p) break;
    
    if (*p == 'L' || *p == 'l') {
      orn->loopPoint = idx;
      p++;
      continue;
    }
    
    int sign = 1;
    if (*p == '-') {
      sign = -1;
      p++;
    } else if (*p == '+') {
      p++;
    }
    
    int value = 0;
    while (*p >= '0' && *p <= '9') {
      value = value * 10 + (*p - '0');
      p++;
    }
    
    orn->offsets[idx++] = (int8_t)(sign * value);
    orn->length++;
  }
  
  if (orn->loopPoint == -1) {
    orn->loopPoint = 0;
  }
}

static int parseVT2PatternRow(const char* line, VT2PatternRow* row) {
  size_t len = strlen(line);
  if (len < 5) return 0;
  
  row->envelopePeriod = 0xFFFF;
  row->noiseMode = 0xFF;
  
  for (int ch = 0; ch < VT2_CHANNELS; ch++) {
    row->channels[ch].hasNote = 0;
    row->channels[ch].note = 0;
    row->channels[ch].instrument = 0xFF;
    row->channels[ch].ornament = 0xFF;
    row->channels[ch].volume = 0xFF;
    row->channels[ch].envelopeType = 0xFF;
    row->channels[ch].commandCount = 0;
  }
  
  // VT2 envelope period format: HHLL (hex, high byte first)
  if (len >= 4 && (line[0] != '.' || line[1] != '.' || line[2] != '.' || line[3] != '.')) {
    uint8_t highByte = (parseHex(line[0]) << 4) | parseHex(line[1]);
    uint8_t lowByte = (parseHex(line[2]) << 4) | parseHex(line[3]);
    row->envelopePeriod = (highByte << 8) | lowByte;
  }
  
  if (len >= 5 && line[4] != '.' && line[4] != ' ') {
    row->noiseMode = parseHex(line[4]);
  }
  
  // VT2 format: "..27|..|F-4 1.FF B..C|F-1 3E.. ....|F-4 5F1C ...."
  const char* channelStart[3];
  int sepCount = 0;
  for (size_t i = 0; i < len && sepCount < 4; i++) {
    if (line[i] == '|') {
      if (sepCount == 1) channelStart[0] = &line[i+1];
      if (sepCount == 2) channelStart[1] = &line[i+1];
      if (sepCount == 3) channelStart[2] = &line[i+1];
      sepCount++;
    }
  }
  
  if (sepCount < 4) return 0;
  
  for (int ch = 0; ch < VT2_CHANNELS; ch++) {
    const char* p = channelStart[ch];
    if (!p) continue;
    
    if (p[0] != '-' && p[0] != '.' && p[0] != ' ') {
      char noteStr[4] = {0};
      noteStr[0] = p[0];
      noteStr[1] = p[1];
      noteStr[2] = p[2];
      
      int note = parseNoteName(noteStr);
      if (note >= 0) {
        row->channels[ch].hasNote = 1;
        row->channels[ch].note = note;
      } else if (p[0] == 'R') {
        row->channels[ch].hasNote = 1;
        row->channels[ch].note = NOTE_OFF;
      }
    }
    
    p += 4;
    
    if (*p && *p != '.' && *p != ' ') {
      row->channels[ch].instrument = parseBase26(*p);
    }
    p++;
    
    if (*p && *p != '.' && *p != ' ') {
      row->channels[ch].envelopeType = parseHex(*p);
    }
    p++;
    
    if (*p && *p != '.' && *p != ' ') {
      row->channels[ch].ornament = parseHex(*p);
    }
    p++;
    
    if (*p && *p != '.' && *p != ' ') {
      row->channels[ch].volume = parseHex(*p);
    }
    p++;
    
    while (*p == ' ') p++;
    
    if (*p && *p != '.' && *p != ' ') {
      char cmdChar = *p;
      p++;
      
      while (*p && *p == '.') p++;

      uint8_t param = 0;
      if (*p && *p != ' ') {
        if (cmdChar == '1' || cmdChar == '2') {
          if (*p && isxdigit(*p)) {
            int delay = parseHex(*p);
            p++;

            char paramStr[4] = {0};
            int paramLen = 0;
            while (*p && *p != ' ' && paramLen < 3) {
              if (isxdigit(*p)) {
                paramStr[paramLen++] = *p;
              }
              p++;
            }

            int parameter = 0;
            if (paramLen > 0) {
              parameter = (int)strtol(paramStr, NULL, 16);
            }

            // Store as delay in high 8 bits, parameter in low 8 bits
            param = (delay << 8) | (parameter & 0xFF);
          }
        } else {
          param = parseHex(*p);
          p++;
        }
      }
      
      if (cmdChar >= '0' && cmdChar <= '9') {
        row->channels[ch].commands[0][0] = cmdChar - '0';
        row->channels[ch].commands[0][1] = param;
        row->channels[ch].commandCount = 1;
      } else if (cmdChar >= 'A' && cmdChar <= 'F') {
        row->channels[ch].commands[0][0] = 10 + (cmdChar - 'A');
        row->channels[ch].commands[0][1] = param;
        row->channels[ch].commandCount = 1;
      } else if (cmdChar >= 'a' && cmdChar <= 'f') {
        row->channels[ch].commands[0][0] = 10 + (cmdChar - 'a');
        row->channels[ch].commands[0][1] = param;
        row->channels[ch].commandCount = 1;
      }
    }
  }
  
  return 1;
}

static int loadVT2Module(const char* path, VT2Module* module) {
  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;
  
  memset(module, 0, sizeof(VT2Module));
  
  char* line;
  enum {
    SECTION_NONE,
    SECTION_MODULE,
    SECTION_ORNAMENT,
    SECTION_SAMPLE,
    SECTION_PATTERN
  } currentSection = SECTION_NONE;
  
  int currentOrnament = -1;
  int currentSample = -1;
  int currentPattern = -1;
  
  while ((line = fileReadString(fileId)) != NULL) {
    trimLineEndings(line);
    
    if (strlen(line) == 0) continue;
    
    if (line[0] == '[') {
      if (strncmp(line, "[Module]", 8) == 0) {
        currentSection = SECTION_MODULE;
      } else if (strncmp(line, "[Ornament", 9) == 0) {
        currentSection = SECTION_ORNAMENT;
        sscanf(line, "[Ornament%d]", &currentOrnament);
        currentOrnament--;
        if (currentOrnament >= 0 && currentOrnament < VT2_MAX_ORNAMENTS) {
          module->ornamentCount = (currentOrnament + 1 > module->ornamentCount) ? currentOrnament + 1 : module->ornamentCount;
        }
      } else if (strncmp(line, "[Sample", 7) == 0) {
        currentSection = SECTION_SAMPLE;
        sscanf(line, "[Sample%d]", &currentSample);
        currentSample--;
        if (currentSample >= 0 && currentSample < VT2_MAX_SAMPLES) {
          module->sampleCount = (currentSample + 1 > module->sampleCount) ? currentSample + 1 : module->sampleCount;
        }
      } else if (strncmp(line, "[Pattern", 8) == 0) {
        currentSection = SECTION_PATTERN;
        sscanf(line, "[Pattern%d]", &currentPattern);
        if (currentPattern >= 0 && currentPattern < VT2_MAX_PATTERNS) {
          module->patterns[currentPattern].rowCount = 0;
          module->patternCount = (currentPattern + 1 > module->patternCount) ? currentPattern + 1 : module->patternCount;
        }
      } else {
        currentSection = SECTION_NONE;
      }
      continue;
    }
    
    switch (currentSection) {
      case SECTION_MODULE:
        if (strncmp(line, "Title=", 6) == 0) {
          strncpy(module->title, line + 6, PROJECT_TITLE_LENGTH);
          module->title[PROJECT_TITLE_LENGTH] = '\0';
        } else if (strncmp(line, "Author=", 7) == 0) {
          strncpy(module->author, line + 7, PROJECT_TITLE_LENGTH);
          module->author[PROJECT_TITLE_LENGTH] = '\0';
        } else if (strncmp(line, "Speed=", 6) == 0) {
          module->speed = atoi(line + 6);
        } else if (strncmp(line, "NoteTable=", 10) == 0) {
          module->noteTable = atoi(line + 10);
        } else if (strncmp(line, "PlayOrder=", 10) == 0) {
          const char* p = line + 10;
          module->playOrderLength = 0;
          
          while (*p && module->playOrderLength < 256) {
            while (*p && (*p == ' ' || *p == ',' || *p == '\t')) p++;
            if (!*p) break;
            
            if (*p == 'L' || *p == 'l') {
              p++;
            }
            
            int patNum = 0;
            while (*p >= '0' && *p <= '9') {
              patNum = patNum * 10 + (*p - '0');
              p++;
            }
            
            module->playOrder[module->playOrderLength++] = patNum;
          }
        }
        break;
        
      case SECTION_ORNAMENT:
        if (currentOrnament >= 0 && currentOrnament < VT2_MAX_ORNAMENTS) {
          parseOrnamentLine(line, &module->ornaments[currentOrnament]);
        }
        break;
        
      case SECTION_SAMPLE:
        break;
        
      case SECTION_PATTERN:
        if (currentPattern >= 0 && currentPattern < VT2_MAX_PATTERNS) {
          VT2Pattern* pat = &module->patterns[currentPattern];
          if (pat->rowCount < VT2_PATTERN_LENGTH) {
            if (parseVT2PatternRow(line, &pat->rows[pat->rowCount])) {
              pat->rowCount++;
            }
          }
        }
        break;
        
      default:
        break;
    }
  }
  
  fileClose(fileId);
  return 0;
}

// Groove 00 = start speed (default, all 16 rows same speed)
// Groove 01+ = other grooves (constant-speed or pattern-length)
// Find or create a constant-speed groove (same speed for all 16 rows)
static int getOrCreateGroove(Project* project, uint8_t speed) {
  for (int g = 0; g < PROJECT_MAX_GROOVES; g++) {
    if (project->grooves[g].speed[0] == speed && project->grooves[g].speed[1] == EMPTY_VALUE_8) {
      return g;
    }
  }

  for (int g = 1; g < PROJECT_MAX_GROOVES; g++) {
    if (project->grooves[g].speed[0] == EMPTY_VALUE_8) {
      project->grooves[g].speed[0] = speed;
      for (int i = 1; i < 16; i++) {
        project->grooves[g].speed[i] = EMPTY_VALUE_8;
      }
      return g;
    }
  }
  
  return -1;
}

// Create/find a groove that keeps the current speed for the triggering row,
// then skips a given number of rows with zeros.
// The next phrase will start with its own groove table.
// This is used to trim VT2 patterns whose length is not a multiple of 16.
static int getOrCreateSkipGroove(Project* project, uint8_t baseSpeed, int rowsToSkip) {
  if (rowsToSkip <= 0 || rowsToSkip >= 16 || baseSpeed == EMPTY_VALUE_8) {
    return -1;
  }

  uint8_t layout[16];
  for (int i = 0; i < 16; i++) {
    layout[i] = EMPTY_VALUE_8;
  }

  layout[0] = baseSpeed;

  for (int i = 1; i <= rowsToSkip && i < 16; i++) {
    layout[i] = 0;
  }
  // Don't resume speed at the end - next phrase will set its own groove

  for (int g = 0; g < PROJECT_MAX_GROOVES; g++) {
    int match = 1;
    for (int r = 0; r < 16; r++) {
      if (project->grooves[g].speed[r] != layout[r]) {
        match = 0;
        break;
      }
    }
    if (match) {
      return g;
    }
  }

  for (int g = 1; g < PROJECT_MAX_GROOVES; g++) {
    if (project->grooves[g].speed[0] == EMPTY_VALUE_8) {
      for (int r = 0; r < 16; r++) {
        project->grooves[g].speed[r] = layout[r];
      }
      return g;
    }
  }

  return -1;
}

static void convertOrnamentToTable(const VT2Ornament* orn, Table* table, int ornIdx) {
  initEmptyTable(table);
  
  if (orn->length == 0) return;
  
  int rowCount = (orn->length < 15) ? orn->length : 15;
  
  for (int i = 0; i < rowCount; i++) {
    table->rows[i].pitchOffset = orn->offsets[i];
  }
  
  if (rowCount < 16) {
    table->rows[rowCount].fx[0][0] = fxTHO;
    table->rows[rowCount].fx[0][1] = (orn->loopPoint >= 0) ? (uint8_t)orn->loopPoint : 0;
  }
}

static int convertPatternToPhrases(const VT2Pattern* pattern, Project* project,
                                     int* phraseIndices, int maxPhrases,
                                     int ornamentBaseIdx, InstrumentCloneMap* cloneMap,
                                     uint8_t currentInstrument[VT2_CHANNELS],
                                     uint8_t currentOrnament[VT2_CHANNELS],
                                     uint8_t currentVolume[VT2_CHANNELS],
                                     int needsInstrumentAfterOff[VT2_CHANNELS],
                                     int isFirstPattern, int startSpeedGroove,
                                     int* lastUsedGroove, int* realLastGroove, int patIdx) {
  if (pattern->rowCount == 0) return 0;

  int phrasesNeeded = (pattern->rowCount + PHRASE_ROWS - 1) / PHRASE_ROWS;
  if (phrasesNeeded > maxPhrases) {
    phrasesNeeded = maxPhrases;
  }
  if (phrasesNeeded > PHRASES_PER_PATTERN) {
    phrasesNeeded = PHRASES_PER_PATTERN;
  }
  
  for (int phraseNum = 0; phraseNum < phrasesNeeded; phraseNum++) {
    int startRow = phraseNum * PHRASE_ROWS;
    int endRow = startRow + PHRASE_ROWS;
    if (endRow > pattern->rowCount) {
      endRow = pattern->rowCount;
    }
    int rowsInPhrase = endRow - startRow;
    
    int phraseA, phraseB, phraseC;
    getPhraseIndicesForPattern(phraseNum, phraseIndices, &phraseA, &phraseB, &phraseC);
    
    Phrase* phrases[3] = {
      &project->phrases[phraseA],
      &project->phrases[phraseB],
      &project->phrases[phraseC]
    };
    
    for (int row = 0; row < rowsInPhrase; row++) {
      const VT2PatternRow* vt2Row = &pattern->rows[startRow + row];

      for (int ch = 0; ch < VT2_CHANNELS; ch++) {
        PhraseRow* phraseRow = &phrases[ch]->rows[row];
        const VT2ChannelData* chData = &vt2Row->channels[ch];
        
        processChannelRow(project, phraseRow, chData, vt2Row,
                         &currentInstrument[ch], &currentOrnament[ch], 
                         &currentVolume[ch], &needsInstrumentAfterOff[ch],
                         ornamentBaseIdx, cloneMap, isFirstPattern, phraseNum, row,
                         lastUsedGroove, realLastGroove, startSpeedGroove);
      }
    }
    
    for (int ch = 0; ch < VT2_CHANNELS; ch++) {
      Phrase* phrase = phrases[ch];
      for (int row = rowsInPhrase; row < 16; row++) {
        initEmptyPhraseRow(&phrase->rows[row]);
      }
    }
    
    // If this is the last phrase and it's shorter than 16 rows, switch to a
    // skipping groove so the leftover rows are skipped instantly.
    if (phraseNum == phrasesNeeded - 1 && rowsInPhrase < 16) {
      int rowsToSkip = 16 - rowsInPhrase;

      int baseGrooveIdx = (*lastUsedGroove >= 0) ? *lastUsedGroove : startSpeedGroove;
      if (baseGrooveIdx < 0) baseGrooveIdx = 0;

      uint8_t baseSpeed = project->grooves[baseGrooveIdx].speed[0];
      if (baseSpeed == EMPTY_VALUE_8 || baseSpeed == 0) {
        baseSpeed = 1; // Fallback to a sane non-zero speed
      }


      int skipGrooveIdx = getOrCreateSkipGroove(project, baseSpeed, rowsToSkip);
      if (skipGrooveIdx >= 0) {
        int lastRow = rowsInPhrase - 1;
        int inserted = 0;

        for (int ch = 0; ch < VT2_CHANNELS && !inserted; ch++) {
          PhraseRow* phraseRow = &phrases[ch]->rows[lastRow];
          for (int fx = 0; fx < MAX_FX_SLOTS; fx++) {
            if (phraseRow->fx[fx][0] == EMPTY_VALUE_8) {
              phraseRow->fx[fx][0] = fxGGR;
              phraseRow->fx[fx][1] = (uint8_t)skipGrooveIdx;
              inserted = 1;
              break;
            }
          }
        }

        if (inserted) {
          *lastUsedGroove = skipGrooveIdx;
        } else {
        }
      }
    }
  }
  
  return phrasesNeeded;
}

static void scanInstrumentEnvelopeUsage(const VT2Module* module, InstrumentEnvelopeUsage usage[VT2_MAX_SAMPLES]) {
  for (int i = 0; i < VT2_MAX_SAMPLES; i++) {
    usage[i].usesEnvelope = 0;
    for (int e = 0; e < 16; e++) {
      usage[i].envelopeTypes[e] = 0;
    }
  }
  
  for (int patIdx = 0; patIdx < module->patternCount; patIdx++) {
    const VT2Pattern* pattern = &module->patterns[patIdx];
    uint8_t currentInstrument[VT2_CHANNELS] = {0, 0, 0};
    
    for (int row = 0; row < pattern->rowCount; row++) {
      const VT2PatternRow* vt2Row = &pattern->rows[row];
      
      for (int ch = 0; ch < VT2_CHANNELS; ch++) {
        const VT2ChannelData* chData = &vt2Row->channels[ch];
        
        if (chData->instrument != 0xFF && chData->instrument > 0 && chData->instrument <= VT2_MAX_SAMPLES) {
          currentInstrument[ch] = chData->instrument;
        }
        
        if (currentInstrument[ch] > 0 && currentInstrument[ch] <= VT2_MAX_SAMPLES) {
          if (chData->envelopeType != 0xFF) {
            usage[currentInstrument[ch]].envelopeTypes[chData->envelopeType] = 1;
          } else {
            usage[currentInstrument[ch]].envelopeTypes[0xF] = 1;
          }
        }
      }
    }
  }
}

// Clone instruments that have envelope enabled in their mixer settings
// for each envelope type used (VT2 'E' in mixer + specific envelope types)
static int cloneInstrumentsForEnvelopes(Project* project, const InstrumentEnvelopeUsage usage[VT2_MAX_SAMPLES], 
                                          int sampleCount, InstrumentCloneMap* cloneMap) {
  for (int i = 0; i < VT2_MAX_SAMPLES; i++) {
    for (int e = 0; e < 16; e++) {
      cloneMap->clonedInstrumentIdx[i][e] = -1;
    }
  }
  
  int nextInstrumentIdx = sampleCount;
  
  #define AYM_ENVELOPE_BITS 0xC0
  
  for (int instNum = 1; instNum <= sampleCount && instNum <= VT2_MAX_SAMPLES; instNum++) {
    int instIdx = instNum - 1;
    
    int hasEnvelopeUsage = 0;
    for (int e = 0; e < 16; e++) {
      if (usage[instNum].envelopeTypes[e]) {
        hasEnvelopeUsage = 1;
        break;
      }
    }
    
    if (!hasEnvelopeUsage) continue;
    
    int hasEnvelopeInTable = 0;
    for (int row = 0; row < 16; row++) {
      for (int fx = 0; fx < 4; fx++) {
        if (project->tables[instIdx].rows[row].fx[fx][0] == fxAYM) {
          uint8_t mixerValue = project->tables[instIdx].rows[row].fx[fx][1];
          if (mixerValue & AYM_ENVELOPE_BITS) {
            hasEnvelopeInTable = 1;
            break;
          }
        }
      }
      if (hasEnvelopeInTable) break;
    }
    
    if (!hasEnvelopeInTable) continue;
    
    Instrument originalInst = project->instruments[instIdx];
    Table originalTable = project->tables[instIdx];
    int clonesCreated = 0;
    
    for (int envType = 0; envType < 16; envType++) {
      if (!usage[instNum].envelopeTypes[envType]) continue;
      
      if (nextInstrumentIdx >= PROJECT_MAX_INSTRUMENTS) {
        return nextInstrumentIdx;
      }
      
      project->instruments[nextInstrumentIdx] = originalInst;
      project->tables[nextInstrumentIdx] = originalTable;
      
      cloneMap->clonedInstrumentIdx[instNum][envType] = nextInstrumentIdx;
      clonesCreated++;
      
      // Set envelope type in AYM high nibble (0 = disabled, 1-E = specific envelope)
      for (int row = 0; row < 16; row++) {
        for (int fx = 0; fx < 4; fx++) {
          if (project->tables[nextInstrumentIdx].rows[row].fx[fx][0] == fxAYM) {
            uint8_t mixerValue = project->tables[nextInstrumentIdx].rows[row].fx[fx][1];
            if (mixerValue & AYM_ENVELOPE_BITS) {
              // envType F = disable envelope (set high nibble to 0)
              uint8_t envNibble = (envType == 0xF) ? 0 : envType;
              mixerValue = (mixerValue & 0x0F) | (envNibble << 4);
              project->tables[nextInstrumentIdx].rows[row].fx[fx][1] = mixerValue;
            }
          }
        }
      }
      
      char envChar = (envType < 10) ? ('0' + envType) : ('A' + (envType - 10));
      snprintf(project->instruments[nextInstrumentIdx].name, PROJECT_INSTRUMENT_NAME_LENGTH + 1, 
               "%.13s-E%c", originalInst.name, envChar);
      
      nextInstrumentIdx++;
    }
    
    if (clonesCreated > 0) {
      Instrument emptyInst = {0};
      emptyInst.type = instNone;
      emptyInst.name[0] = '\0';
      project->instruments[instIdx] = emptyInst;
      
      Table emptyTable = {0};
      for (int row = 0; row < 16; row++) {
        emptyTable.rows[row].pitchFlag = 0;
        emptyTable.rows[row].pitchOffset = 0;
        emptyTable.rows[row].volume = EMPTY_VALUE_8;
        for (int fx = 0; fx < 4; fx++) {
          emptyTable.rows[row].fx[fx][0] = EMPTY_VALUE_8;
          emptyTable.rows[row].fx[fx][1] = 0;
        }
      }
      project->tables[instIdx] = emptyTable;
    }
  }
  
  return nextInstrumentIdx;
}

int projectLoadVT2(const char* path) {
  if (!chipnomadState) {
    return 1;
  }

  
  VT2Module module;
  if (loadVT2Module(path, &module) != 0) {
    return 1;
  }
  
  Project p;
  projectInitAY(&p);
  
  Project* project = &p;

  setupPitchTableFromVT2(project, module.noteTable);

  strncpy(project->title, module.title, PROJECT_TITLE_LENGTH);
  project->title[PROJECT_TITLE_LENGTH] = '\0';
  strncpy(project->author, module.author, PROJECT_TITLE_LENGTH);
  project->author[PROJECT_TITLE_LENGTH] = '\0';
  
  project->tracksCount = 3;
  
  int startSpeedGroove = 0;
  if (module.speed > 0) {
    project->grooves[0].speed[0] = module.speed;
    for (int i = 1; i < 16; i++) {
      project->grooves[0].speed[i] = EMPTY_VALUE_8;
    }
  }
  
  int ornamentBaseIdx = 0x80;
  for (int i = 0; i < module.ornamentCount && i < VT2_MAX_ORNAMENTS; i++) {
    if (ornamentBaseIdx + i < PROJECT_MAX_TABLES) {
      convertOrnamentToTable(&module.ornaments[i], &project->tables[ornamentBaseIdx + i], i);
    }
  }
  
  InstrumentEnvelopeUsage envelopeUsage[VT2_MAX_SAMPLES];
  scanInstrumentEnvelopeUsage(&module, envelopeUsage);
  
  Project* savedProject = &chipnomadState->project;
  chipnomadState->project = p;
  
  int sampleImportResult = importVT2Samples(path, &chipnomadState->project, module.sampleCount);
  
  InstrumentCloneMap cloneMap;
  if (sampleImportResult == 0) {
    cloneInstrumentsForEnvelopes(&chipnomadState->project, envelopeUsage, module.sampleCount, &cloneMap);
  }
  
  Project tempWithSamples = chipnomadState->project;
  chipnomadState->project = *savedProject;
  
  if (sampleImportResult != 0) {
    return 1;
  }
  
  project = &tempWithSamples;
  
  int patternToPhrases[VT2_MAX_PATTERNS][3][16];
  int patternPhraseCounts[VT2_MAX_PATTERNS];
  
  int phraseIdx = 0;
  
  uint8_t globalCurrentInstrument[VT2_CHANNELS] = {0, 0, 0};
  uint8_t globalCurrentOrnament[VT2_CHANNELS] = {0, 0, 0};
  uint8_t globalCurrentVolume[VT2_CHANNELS] = {DEFAULT_VOLUME, DEFAULT_VOLUME, DEFAULT_VOLUME};
  int globalNeedsInstrumentAfterOff[VT2_CHANNELS] = {0, 0, 0};
  
  int lastUsedGroove = -1;
  int realLastGroove = -1; //the actual speed groove, not skip grooves
  
  int firstPatternInPlayOrder = -1;
  if (module.playOrderLength > 0) {
    firstPatternInPlayOrder = module.playOrder[0];
  }
  
  for (int patIdx = 0; patIdx < module.patternCount && patIdx < VT2_MAX_PATTERNS; patIdx++) {
    const VT2Pattern* pattern = &module.patterns[patIdx];
    
    if (pattern->rowCount == 0) {
      patternPhraseCounts[patIdx] = 0;
      continue;
    }
    
    int phrasesNeeded = (pattern->rowCount + PHRASE_ROWS - 1) / PHRASE_ROWS;
    
    int phraseIndices[48];
    int phrasesAllocated = 0;
    
    for (int i = 0; i < phrasesNeeded * 3; i++) {
      if (phraseIdx >= PROJECT_MAX_PHRASES) break;
      phraseIndices[i] = phraseIdx++;
      phrasesAllocated++;
    }
    
    if (phrasesAllocated < 3) {
      patternPhraseCounts[patIdx] = 0;
      break;
    }
    
    int isFirstPattern = (patIdx == firstPatternInPlayOrder);
    int phrasesUsed = convertPatternToPhrases(pattern, project, phraseIndices, phrasesAllocated,
                                               ornamentBaseIdx, &cloneMap,
                                               globalCurrentInstrument, globalCurrentOrnament,
                                               globalCurrentVolume,
                                               globalNeedsInstrumentAfterOff,
                                               isFirstPattern, startSpeedGroove, &lastUsedGroove, &realLastGroove, patIdx);
    
    patternPhraseCounts[patIdx] = phrasesUsed;
    
    for (int ch = 0; ch < 3; ch++) {
      for (int ph = 0; ph < phrasesUsed && ph < 16; ph++) {
        patternToPhrases[patIdx][ch][ph] = phraseIndices[ph * 3 + ch];
      }
    }
  }
  
  int chainIdx = 0;

  for (int i = 0; i < module.playOrderLength && i < PROJECT_MAX_LENGTH; i++) {
    int patNum = module.playOrder[i];
    
    if (patNum >= 0 && patNum < module.patternCount && patternPhraseCounts[patNum] > 0) {
      int phrasesNeeded = patternPhraseCounts[patNum];
      
      if (chainIdx + 3 > PROJECT_MAX_CHAINS) break;
      
      int chainA = chainIdx++;
      int chainB = chainIdx++;
      int chainC = chainIdx++;
      
      createMultiPhraseChain(project, chainA, patternToPhrases[patNum][0], phrasesNeeded);
      createMultiPhraseChain(project, chainB, patternToPhrases[patNum][1], phrasesNeeded);
      createMultiPhraseChain(project, chainC, patternToPhrases[patNum][2], phrasesNeeded);
      
      project->song[i][0] = chainA;
      project->song[i][1] = chainB;
      project->song[i][2] = chainC;
      
    }
  }

  chipnomadState->project = *project;


  return 0;
}

static int importVT2Samples(const char* path, Project* project, int sampleCount) {
  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;
  
  char* line;
  int currentSample = -1;
  int inSampleSection = 0;
  char* sampleLines[64];
  int sampleLineCount = 0;
  
  for (int i = 0; i < 64; i++) {
    sampleLines[i] = (char*)malloc(256);
    if (sampleLines[i] == NULL) {
      for (int j = 0; j < i; j++) {
        free(sampleLines[j]);
      }
      fileClose(fileId);
      return 1;
    }
  }
  
  while ((line = fileReadString(fileId)) != NULL) {
    trimLineEndings(line);
    
    if (strlen(line) == 0) continue;
    
    if (line[0] == '[') {
      if (inSampleSection && currentSample >= 0 && currentSample < VT2_MAX_SAMPLES && currentSample < PROJECT_MAX_INSTRUMENTS) {
        char sampleName[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
        snprintf(sampleName, PROJECT_INSTRUMENT_NAME_LENGTH + 1, "Sample%02d", currentSample + 1);
        instrumentLoadVTSFromMemory(sampleLines, sampleLineCount, currentSample, sampleName);
        sampleLineCount = 0;
      }
      
      if (strncmp(line, "[Sample", 7) == 0) {
        sscanf(line, "[Sample%d]", &currentSample);
        currentSample--;
        inSampleSection = 1;
        sampleLineCount = 0;
      } else {
        inSampleSection = 0;
      }
      continue;
    }
    
    if (inSampleSection && sampleLineCount < 64) {
      strncpy(sampleLines[sampleLineCount], line, 255);
      sampleLines[sampleLineCount][255] = '\0';
      sampleLineCount++;
    }
  }
  
  if (inSampleSection && currentSample >= 0 && currentSample < VT2_MAX_SAMPLES && currentSample < PROJECT_MAX_INSTRUMENTS) {
    char sampleName[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
    snprintf(sampleName, PROJECT_INSTRUMENT_NAME_LENGTH + 1, "Sample%02d", currentSample + 1);
    instrumentLoadVTSFromMemory(sampleLines, sampleLineCount, currentSample, sampleName);
  }
  
  for (int i = 0; i < 64; i++) {
    free(sampleLines[i]);
  }
  
  fileClose(fileId);
  return 0;
}
