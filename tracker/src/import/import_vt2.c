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

#define VT2_MAX_PATTERNS 256
#define VT2_MAX_ORNAMENTS 32
#define VT2_MAX_SAMPLES 32
#define VT2_PATTERN_LENGTH 255  // Maximum pattern length (VT2 has no fixed limit, but we need a reasonable max)
#define VT2_CHANNELS 3

typedef struct InstrumentEnvelopeUsage {
  uint8_t envelopeTypes[16];
  uint8_t usesEnvelope;
} InstrumentEnvelopeUsage;

typedef struct InstrumentCloneMap {
  int8_t clonedInstrumentIdx[VT2_MAX_SAMPLES][16];
} InstrumentCloneMap;

static int importVT2Samples(const char* path, Project* project, int sampleCount);
static void initEmptyPhraseRow(PhraseRow* row);
static void trimLineEndings(char* line);
static void createMultiPhraseChain(Project* project, int chainIdx, const int* phraseIndices, int phraseCount);

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
  int playOrder[256];
  int playOrderLength;
  VT2Pattern patterns[VT2_MAX_PATTERNS];
  VT2Ornament ornaments[VT2_MAX_ORNAMENTS];
  int patternCount;
  int ornamentCount;
  int sampleCount;
} VT2Module;

static int parseHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
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

static void initEmptyPhraseRow(PhraseRow* row) {
  row->note = EMPTY_VALUE_8;
  row->instrument = EMPTY_VALUE_8;
  row->volume = EMPTY_VALUE_8;
  for (int fx = 0; fx < 3; fx++) {
    row->fx[fx][0] = EMPTY_VALUE_8;
    row->fx[fx][1] = 0;
  }
}

static void createMultiPhraseChain(Project* project, int chainIdx, const int* phraseIndices, int phraseCount) {
  Chain* chain = &project->chains[chainIdx];
  
  for (int i = 0; i < phraseCount && i < 16; i++) {
    chain->rows[i].phrase = phraseIndices[i];
    chain->rows[i].transpose = 0;
  }
  
  for (int i = phraseCount; i < 16; i++) {
    chain->rows[i].phrase = EMPTY_VALUE_16;
    chain->rows[i].transpose = 0;
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
      row->channels[ch].instrument = parseHex(*p);
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
    
    // Skip spaces before effect
    while (*p == ' ') p++;
    
    // Parse effect: BXYZ where B=command, X=delay (ignore), YZ=params
    if (*p && *p != '.' && *p != ' ') {
      char cmdChar = *p;
      p++;
      
      // Skip delay byte (X) - always skip one character (even if it's '.')
      if (*p && *p != ' ') {
        p++;  // Skip delay byte unconditionally
      }
      
      // Parse YZ parameter bytes ('.' means 0)
      uint8_t param = 0;
      if (*p && *p != ' ') {
        if (*p != '.') {
          param = (parseHex(*p) << 4);
        }
        p++;
        if (*p && *p != ' ') {
          if (*p != '.') {
            param |= parseHex(*p);
          }
        }
      }
      
      // Store command
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

// Load VT2 module structure
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
    
    // Section headers
    if (line[0] == '[') {
      if (strncmp(line, "[Module]", 8) == 0) {
        currentSection = SECTION_MODULE;
      } else if (strncmp(line, "[Ornament", 9) == 0) {
        currentSection = SECTION_ORNAMENT;
        sscanf(line, "[Ornament%d]", &currentOrnament);
        currentOrnament--; // 0-indexed
        if (currentOrnament >= 0 && currentOrnament < VT2_MAX_ORNAMENTS) {
          module->ornamentCount = (currentOrnament + 1 > module->ornamentCount) ? currentOrnament + 1 : module->ornamentCount;
        }
      } else if (strncmp(line, "[Sample", 7) == 0) {
        currentSection = SECTION_SAMPLE;
        sscanf(line, "[Sample%d]", &currentSample);
        currentSample--; // 0-indexed
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
    
    // Parse section content
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
        } else if (strncmp(line, "PlayOrder=", 10) == 0) {
          const char* p = line + 10;
          module->playOrderLength = 0;
          
          while (*p && module->playOrderLength < 256) {
            while (*p && (*p == ' ' || *p == ',' || *p == '\t')) p++;
            if (!*p) break;
            
            // Skip loop marker 'L'
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
  // First, look for an existing groove with this speed in all rows
  for (int g = 0; g < PROJECT_MAX_GROOVES; g++) {
    if (project->grooves[g].speed[0] == speed && project->grooves[g].speed[1] == EMPTY_VALUE_8) {
      return g;
    }
  }
  
  // Not found, create a new constant-speed groove in first empty slot
  for (int g = 1; g < PROJECT_MAX_GROOVES; g++) {
    if (project->grooves[g].speed[0] == EMPTY_VALUE_8) {
      project->grooves[g].speed[0] = speed;
      for (int i = 1; i < 16; i++) {
        project->grooves[g].speed[i] = EMPTY_VALUE_8;
      }
      return g;
    }
  }
  
  return -1; // No free slots
}

// VT2 ornaments -> ChipNomad aux tables with semitone offsets + THO loop
static void convertOrnamentToTable(const VT2Ornament* orn, Table* table, int ornIdx) {
  for (int i = 0; i < 16; i++) {
    table->rows[i].pitchFlag = 0;
    table->rows[i].pitchOffset = 0;
    table->rows[i].volume = EMPTY_VALUE_8;
    for (int j = 0; j < 4; j++) {
      table->rows[i].fx[j][0] = EMPTY_VALUE_8;
      table->rows[i].fx[j][1] = 0;
    }
  }
  
  if (orn->length == 0) return;
  
  int rowCount = (orn->length < 15) ? orn->length : 15;
  
  for (int i = 0; i < rowCount; i++) {
    table->rows[i].pitchFlag = 0;
    table->rows[i].pitchOffset = orn->offsets[i];
    table->rows[i].volume = EMPTY_VALUE_8;
  }
  
  // Add THO to loop ornament
  if (rowCount < 16) {
    table->rows[rowCount].fx[0][0] = fxTHO;
    table->rows[rowCount].fx[0][1] = (orn->loopPoint >= 0) ? (uint8_t)orn->loopPoint : 0;
  }
}

// VT2 patterns (dynamic length) -> ChipNomad phrases (16 rows each)
// Simple approach:
// - B command -> GGR with constant-speed groove (length 1)
// - Pattern ends (first empty row) -> GGR 00 (skip groove with speed 0)
// - Next pattern (first row of first phrase) -> GGR with last used speed (restored)
// lastUsedGroove is passed in/out to track the last speed groove used
static int convertPatternToPhrases(const VT2Pattern* pattern, Project* project, 
                                     int* phraseIndices, int maxPhrases,
                                     int ornamentBaseIdx, InstrumentCloneMap* cloneMap,
                                     uint8_t currentInstrument[VT2_CHANNELS],
                                     uint8_t currentOrnament[VT2_CHANNELS],
                                     int needsInstrumentAfterOff[VT2_CHANNELS],
                                     int isFirstPattern, int startSpeedGroove,
                                     int* lastUsedGroove) {
  if (pattern->rowCount == 0) return 0;
  
  int phrasesNeeded = (pattern->rowCount + 15) / 16;
  if (phrasesNeeded > maxPhrases) {
    phrasesNeeded = maxPhrases;
  }
  
  for (int phraseNum = 0; phraseNum < phrasesNeeded; phraseNum++) {
    int startRow = phraseNum * 16;
    int endRow = startRow + 16;
    if (endRow > pattern->rowCount) {
      endRow = pattern->rowCount;
    }
    int rowsInPhrase = endRow - startRow;
    
    int phraseA = phraseIndices[phraseNum * 3 + 0];
    int phraseB = phraseIndices[phraseNum * 3 + 1];
    int phraseC = phraseIndices[phraseNum * 3 + 2];
    
    Phrase* phrases[3] = {
      &project->phrases[phraseA],
      &project->phrases[phraseB],
      &project->phrases[phraseC]
    };
    
    // ChipNomad: instruments don't persist between phrases
    int needsInstrumentSet[VT2_CHANNELS] = {0, 0, 0};
    if (phraseNum > 0) {
      for (int ch = 0; ch < VT2_CHANNELS; ch++) {
        if (currentInstrument[ch] > 0 || needsInstrumentAfterOff[ch]) {
          needsInstrumentSet[ch] = 1;
        }
      }
    }
    
    for (int row = 0; row < rowsInPhrase; row++) {
      const VT2PatternRow* vt2Row = &pattern->rows[startRow + row];
      
      for (int ch = 0; ch < VT2_CHANNELS; ch++) {
        PhraseRow* phraseRow = &phrases[ch]->rows[row];
        const VT2ChannelData* chData = &vt2Row->channels[ch];
        
        if (chData->hasNote) {
          phraseRow->note = chData->note;
        } else {
          phraseRow->note = EMPTY_VALUE_8;
        }
        
        if (chData->instrument != 0xFF && chData->instrument > 0) {
          currentInstrument[ch] = chData->instrument;
        }
        
        if (chData->ornament != 0xFF) {
          currentOrnament[ch] = chData->ornament;
        } else if (chData->envelopeType == 0xF) {
          currentOrnament[ch] = 0;
        }
        
        if (chData->hasNote && chData->note == NOTE_OFF) {
          needsInstrumentAfterOff[ch] = 1;
        }
        
        if (chData->hasNote && chData->note != NOTE_OFF) {
          int shouldSetInstrument = (chData->instrument != 0xFF && chData->instrument > 0) || needsInstrumentSet[ch] || needsInstrumentAfterOff[ch];
          
          // If no instrument has been set yet, default to instrument 1
          if (currentInstrument[ch] == 0) {
            currentInstrument[ch] = 1;
            shouldSetInstrument = 1;
          }
          
          if (shouldSetInstrument && currentInstrument[ch] > 0) {
            int baseInstrument = currentInstrument[ch] - 1;
            int finalInstrument = baseInstrument;
            
            int envType = (chData->envelopeType != 0xFF) ? chData->envelopeType : 0xF;
            
            if (cloneMap) {
              int clonedIdx = cloneMap->clonedInstrumentIdx[currentInstrument[ch]][envType];
              if (clonedIdx >= 0) {
                finalInstrument = clonedIdx;
              }
            }
            
            phraseRow->instrument = finalInstrument;
            needsInstrumentSet[ch] = 0;
            needsInstrumentAfterOff[ch] = 0;
          } else {
            phraseRow->instrument = EMPTY_VALUE_8;
          }
        } else {
          phraseRow->instrument = EMPTY_VALUE_8;
        }
        
        if (chData->volume != 0xFF) {
          phraseRow->volume = chData->volume;
        } else {
          phraseRow->volume = EMPTY_VALUE_8;
        }
        
        int fxSlot = 0;
        
        if (chData->hasNote && currentOrnament[ch] > 0) {
          int ornTableIdx = ornamentBaseIdx + currentOrnament[ch] - 1;
          if (ornTableIdx < PROJECT_MAX_TABLES && fxSlot < 3) {
            phraseRow->fx[fxSlot][0] = fxTBX;
            phraseRow->fx[fxSlot][1] = (uint8_t)ornTableIdx;
            fxSlot++;
          }
        }
        
        if (chData->envelopeType != 0xFF && chData->envelopeType != 0xF) {
          if (fxSlot < 3) {
            phraseRow->fx[fxSlot][0] = fxERT;
            phraseRow->fx[fxSlot][1] = 0;
            fxSlot++;
          }
        }
        
        if (vt2Row->envelopePeriod != 0xFFFF && fxSlot < 3) {
          phraseRow->fx[fxSlot][0] = fxEPL;
          phraseRow->fx[fxSlot][1] = vt2Row->envelopePeriod & 0xFF;
          fxSlot++;
          
          uint8_t highByte = (vt2Row->envelopePeriod >> 8) & 0xFF;
          if (highByte > 0 && fxSlot < 3) {
            phraseRow->fx[fxSlot][0] = fxEPH;
            phraseRow->fx[fxSlot][1] = highByte;
            fxSlot++;
          }
        }
        
        // B command = speed -> GGR with constant-speed groove
        if (chData->commandCount > 0 && fxSlot < 3) {
          uint8_t cmd = chData->commands[0][0];
          uint8_t param = chData->commands[0][1];
          
          if (cmd == 0xB && param > 0) {
            int grooveIdx = getOrCreateGroove(project, param);
            if (grooveIdx >= 0) {
              phraseRow->fx[fxSlot][0] = fxGGR;
              phraseRow->fx[fxSlot][1] = (uint8_t)grooveIdx;
              fxSlot++;
              *lastUsedGroove = grooveIdx;
              if (isFirstPattern && phraseNum == 0 && row < 2) {
                printf("  B cmd speed %d -> GGR %d (ch %d, row %d)\n", param, grooveIdx, ch, row);
              }
            }
          }
        }
        
        
        // Apply start speed groove on first row of first pattern in playOrder (if no B command yet)
        if (isFirstPattern && phraseNum == 0 && row == 0 && *lastUsedGroove < 0 && startSpeedGroove >= 0 && fxSlot < 3) {
          phraseRow->fx[fxSlot][0] = fxGGR;
          phraseRow->fx[fxSlot][1] = (uint8_t)startSpeedGroove;
          fxSlot++;
          *lastUsedGroove = startSpeedGroove;
          printf("  Applied start speed GGR %d on first pattern (ch %d)\n", startSpeedGroove, ch);
        }
        
        while (fxSlot < 3) {
          phraseRow->fx[fxSlot][0] = EMPTY_VALUE_8;
          phraseRow->fx[fxSlot][1] = 0;
          fxSlot++;
        }
      }
    }
    
    // Initialize remaining rows as empty
    for (int ch = 0; ch < VT2_CHANNELS; ch++) {
      Phrase* phrase = phrases[ch];
      for (int row = rowsInPhrase; row < 16; row++) {
        initEmptyPhraseRow(&phrase->rows[row]);
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
// Returns the next available instrument index
// Clone instruments for each envelope type used (VT2 'E' in mixer + specific envelope types)
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
    
    // Clear base instrument (only clones are used)
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

// Main import function
int projectLoadVT2(const char* path) {
  if (!chipnomadState) {
    return 1;
  }
  
  VT2Module module;
  if (loadVT2Module(path, &module) != 0) {
    return 1;
  }
  
  // Load into temporary project first, then copy to main project
  Project p;
  projectInitAY(&p);
  
  Project* project = &p;
  
  // Set title and author
  strncpy(project->title, module.title, PROJECT_TITLE_LENGTH);
  project->title[PROJECT_TITLE_LENGTH] = '\0';
  strncpy(project->author, module.author, PROJECT_TITLE_LENGTH);
  project->author[PROJECT_TITLE_LENGTH] = '\0';
  
  // Set up 3 tracks for the 3 AY channels
  project->tracksCount = 3;
  
  // Groove 00 = VT2 start speed (default groove used by playback)
  int startSpeedGroove = 0;
  if (module.speed > 0) {
    project->grooves[0].speed[0] = module.speed;
    for (int i = 1; i < 16; i++) {
      project->grooves[0].speed[i] = EMPTY_VALUE_8;
    }
  }
  
  // Convert ornaments to aux tables (starting at 0x80 = 128)
  int ornamentBaseIdx = 0x80;  // Start at 128 for aux tables
  for (int i = 0; i < module.ornamentCount && i < VT2_MAX_ORNAMENTS; i++) {
    if (ornamentBaseIdx + i < PROJECT_MAX_TABLES) {
      convertOrnamentToTable(&module.ornaments[i], &project->tables[ornamentBaseIdx + i], i);
    }
  }
  
  // Scan patterns to find which envelope types are used with which instruments
  InstrumentEnvelopeUsage envelopeUsage[VT2_MAX_SAMPLES];
  scanInstrumentEnvelopeUsage(&module, envelopeUsage);
  
  // Import samples into the temporary project FIRST (before pattern conversion)
  // We need to temporarily swap the project pointer so VTS importer writes to temp project
  Project* savedProject = &chipnomadState->project;
  chipnomadState->project = p;
  
  int sampleImportResult = importVT2Samples(path, &chipnomadState->project, module.sampleCount);
  
  // Clone instruments for different envelope types
  // This must happen AFTER importing samples but BEFORE pattern conversion
  InstrumentCloneMap cloneMap;
  if (sampleImportResult == 0) {
    cloneInstrumentsForEnvelopes(&chipnomadState->project, envelopeUsage, module.sampleCount, &cloneMap);
  }
  
  // Restore the original project pointer
  Project tempWithSamples = chipnomadState->project;
  chipnomadState->project = *savedProject;
  
  if (sampleImportResult != 0) {
    // Sample import failed - don't overwrite user's project
    return 1;
  }
  
  // Now tempWithSamples has instruments and clones, cloneMap is populated
  // Continue working with tempWithSamples
  project = &tempWithSamples;
  
  // Convert patterns to phrases
  // Patterns can have any length - use grooves with speed 0 to skip unused rows
  // Structure: patternToPhrases[pattern][channel][phraseNum] = phraseIdx
  int patternToPhrases[VT2_MAX_PATTERNS][3][16]; // Max 16 phrases per pattern (256 rows)
  int patternPhraseCounts[VT2_MAX_PATTERNS];
  
  int phraseIdx = 0;
  
  // VT2: instruments/ornaments persist across patterns (not just within)
  uint8_t globalCurrentInstrument[VT2_CHANNELS] = {0, 0, 0};
  uint8_t globalCurrentOrnament[VT2_CHANNELS] = {0, 0, 0};
  int globalNeedsInstrumentAfterOff[VT2_CHANNELS] = {0, 0, 0};
  
  // Track last used groove across patterns
  int lastUsedGroove = -1; // -1 means not set yet
  
  // Find the first pattern in playOrder (this is where we apply start speed groove)
  int firstPatternInPlayOrder = -1;
  if (module.playOrderLength > 0) {
    firstPatternInPlayOrder = module.playOrder[0];
  }
  
  printf("VT2 Import Debug:\n");
  printf("  Start speed: %d\n", module.speed);
  printf("  Groove 0 speed[0]: %d (default groove)\n", project->grooves[0].speed[0]);
  printf("  fxGGR=%02X\n", fxGGR);
  printf("  Play order length: %d\n", module.playOrderLength);
  printf("  First pattern in play order: %d\n", firstPatternInPlayOrder);
  printf("  Play order: ");
  for (int i = 0; i < module.playOrderLength && i < 10; i++) {
    printf("%d ", module.playOrder[i]);
  }
  printf("\n");
  
  for (int patIdx = 0; patIdx < module.patternCount && patIdx < VT2_MAX_PATTERNS; patIdx++) {
    const VT2Pattern* pattern = &module.patterns[patIdx];
    
    if (pattern->rowCount == 0) {
      patternPhraseCounts[patIdx] = 0;
      continue;
    }
    
    // Calculate phrases needed (simple division, no packing)
    int phrasesNeeded = (pattern->rowCount + 15) / 16;
    if (phrasesNeeded > 16) phrasesNeeded = 16;
    
    // Allocate phrase indices (groups of 3: one per channel)
    int phraseIndices[48]; // Max 16 phrases * 3 channels
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
    
    // Convert pattern - uses GGR only
    int isFirstPattern = (patIdx == firstPatternInPlayOrder);
    int phrasesUsed = convertPatternToPhrases(pattern, project, phraseIndices, phrasesAllocated, 
                                               ornamentBaseIdx, &cloneMap,
                                               globalCurrentInstrument, globalCurrentOrnament, 
                                               globalNeedsInstrumentAfterOff,
                                               isFirstPattern, startSpeedGroove, &lastUsedGroove);
    
    // Track which phrases belong to this pattern
    patternPhraseCounts[patIdx] = phrasesUsed;
    
    // Map phrases to pattern
    for (int ch = 0; ch < 3; ch++) {
      for (int ph = 0; ph < phrasesUsed && ph < 16; ph++) {
        patternToPhrases[patIdx][ch][ph] = phraseIndices[ph * 3 + ch];
      }
    }
  }
  
  // Build song structure from play order
  // Simple approach: create chains for all patterns, empty or not
  int chainIdx = 0;
  
  printf("  Building song structure...\n");
  
  for (int i = 0; i < module.playOrderLength && i < PROJECT_MAX_LENGTH; i++) {
    int patNum = module.playOrder[i];
    
    if (patNum >= 0 && patNum < module.patternCount && patternPhraseCounts[patNum] > 0) {
      int phrasesNeeded = patternPhraseCounts[patNum];
      
      // Create chains for all 3 channels
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
      
      if (i < 3) {
        printf("  Song pos %d: pattern %d (%d phrases) -> chains %d,%d,%d\n", 
               i, patNum, phrasesNeeded, chainA, chainB, chainC);
        // Show first phrase in chain A
        int pIdx = project->chains[chainA].rows[0].phrase;
        PhraseRow* pr = &project->phrases[pIdx].rows[0];
        printf("    Chain %d phrase %d row0: note=%02X inst=%02X fx=(%02X/%02X, %02X/%02X, %02X/%02X)\n",
               chainA, pIdx, pr->note, pr->instrument,
               pr->fx[0][0], pr->fx[0][1],
               pr->fx[1][0], pr->fx[1][1],
               pr->fx[2][0], pr->fx[2][1]);
      }
    } else {
      printf("  Song pos %d: pattern %d SKIPPED (rowCount=%d, phraseCount=%d)\n", 
             i, patNum, 
             (patNum >= 0 && patNum < module.patternCount) ? module.patterns[patNum].rowCount : -1,
             (patNum >= 0 && patNum < VT2_MAX_PATTERNS) ? patternPhraseCounts[patNum] : -1);
    }
  }
  
  printf("  Total chains created: %d\n", chainIdx);
  
  // The temporary project now has everything: patterns, chains, instruments, clones
  // Copy it to the main project
  chipnomadState->project = *project;
  
  return 0;
}

// Import VT2 samples (instruments) by extracting them and using VTS importer
static int importVT2Samples(const char* path, Project* project, int sampleCount) {
  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;
  
  char* line;
  int currentSample = -1;
  int inSampleSection = 0;
  char* sampleLines[64]; // Store pointers to up to 64 lines per sample
  int sampleLineCount = 0;
  
  // Allocate memory for sample lines
  for (int i = 0; i < 64; i++) {
    sampleLines[i] = (char*)malloc(256);
    if (sampleLines[i] == NULL) {
      // Cleanup on allocation failure
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
    
    // Check for sample section
    if (line[0] == '[') {
      // If we were in a sample section, import it now
      if (inSampleSection && currentSample >= 0 && currentSample < VT2_MAX_SAMPLES && currentSample < PROJECT_MAX_INSTRUMENTS) {
        char sampleName[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
        snprintf(sampleName, PROJECT_INSTRUMENT_NAME_LENGTH + 1, "Sample%02d", currentSample + 1);
        instrumentLoadVTSFromMemory(sampleLines, sampleLineCount, currentSample, sampleName);
        sampleLineCount = 0;
      }
      
      if (strncmp(line, "[Sample", 7) == 0) {
        sscanf(line, "[Sample%d]", &currentSample);
        currentSample--; // 0-indexed
        inSampleSection = 1;
        sampleLineCount = 0;
      } else {
        inSampleSection = 0;
      }
      continue;
    }
    
    // Collect sample lines
    if (inSampleSection && sampleLineCount < 64) {
      strncpy(sampleLines[sampleLineCount], line, 255);
      sampleLines[sampleLineCount][255] = '\0';
      sampleLineCount++;
    }
  }
  
  // Handle last sample if file ended while in sample section
  if (inSampleSection && currentSample >= 0 && currentSample < VT2_MAX_SAMPLES && currentSample < PROJECT_MAX_INSTRUMENTS) {
    char sampleName[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
    snprintf(sampleName, PROJECT_INSTRUMENT_NAME_LENGTH + 1, "Sample%02d", currentSample + 1);
    instrumentLoadVTSFromMemory(sampleLines, sampleLineCount, currentSample, sampleName);
  }
  
  // Cleanup
  for (int i = 0; i < 64; i++) {
    free(sampleLines[i]);
  }
  
  fileClose(fileId);
  return 0;
}


