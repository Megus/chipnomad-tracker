#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdint.h>

#define PROJECT_MAX_TRACKS (10)
#define PROJECT_MAX_LENGTH (256)
#define PROJECT_MAX_CHAINS (255)
#define PROJECT_MAX_PHRASES (1024)
#define PROJECT_MAX_GROOVES (32)
#define PROJECT_MAX_INSTRUMENTS (128)
#define PROJECT_MAX_TABLES (255)
#define PROJECT_MAX_CHIPS (3)
#define PROJECT_MAX_PITCHES (254)
#define PROJECT_INSTRUMENT_NAME_LENGTH (15)
#define PROJECT_TITLE_LENGTH (24)
#define PROJECT_PITCH_TABLE_TITLE_LENGTH (18)

#define NOTE_OFF (254)
#define EMPTY_VALUE_8 (255)
#define EMPTY_VALUE_16 (32767)

// Song data structures

// FX

enum FX {
  fxARP, // Arpeggio
  fxARC, // Arpeggio config
  fxPVB, // Pitch vibrato
  fxPBN, // Pitch bend
  fxPSL, // Pitch slide (portamento)
  fxPIT, // Pitch offset
  fxPRD, // Period offset
  fxVOL, // Volume (relative)
  fxRET, // Retrigger
  fxDEL, // Delay
  fxOFF, // Off
  fxKIL, // Kill note
  fxTIC, // Table speed
  fxTBL, // Set instrument table
  fxTBX, // Set aux table
  fxTHO, // Table hop
  fxTXH, // Aux table hop
  fxGRV, // Track groove
  fxGGR, // Global groove
  fxHOP, // Hop
  // AY-specific FX
  fxAYM, // AY Mixer settting
  fxERT, // Envelope retrigger
  fxNOI, // Noise (relative)
  fxNOA, // Noise (absolute)
  fxEAU, // Auto-env setting
  fxEVB, // Envelope vibrato
  fxEBN, // Envelope bend
  fxESL, // Envelope slide (portamento)
  fxENT, // Envelope note
  fxEPT, // Envelope pitch offset
  fxEPL, // Envelope period L
  fxEPH, // Envelope period H
  // Terminate
  fxTotalCount
};

typedef struct FXName {
  enum FX fx;
  char name[4];
} FXName;

extern FXName fxNames[256]; // All names
extern FXName fxNamesCommon[]; // Common FX names
extern int fxCommonCount;
extern FXName fxNamesAY[]; // AY FX names
extern int fxAYCount;

// Chips

enum ChipType {
  chipAY = 0,
  chipTotalCount,
};

enum StereoModeAY {
  ayStereoABC,
  ayStereoACB,
  ayStereoBAC,
};

typedef struct ChipSetupAY {
  int clock;
  uint8_t isYM;
  enum StereoModeAY stereoMode;
  uint8_t stereoSeparation;
} ChipSetupAY;

typedef union ChipSetup {
  ChipSetupAY ay;
} ChipSetup;

// Tables

typedef struct TableRow {
  uint8_t pitchFlag;
  uint8_t pitchOffset;
  uint8_t volume;
  uint8_t fx[4][2];
} TableRow;

typedef struct Table {
  TableRow rows[16];
} Table;

// Instruments

enum InstrumentType {
  instNone = 0,
  instAY = 1,
};

typedef struct InstrumentAY {
  uint8_t veA;
  uint8_t veD;
  uint8_t veS;
  uint8_t veR;
  uint8_t autoEnvN; // 0 - no auto-env
  uint8_t autoEnvD;
} InstrumentAY;

typedef union InstrumentChipData {
  InstrumentAY ay;
} InstrumentChipData;

typedef struct Instrument {
  uint8_t type; // enum InstrumentType
  char name[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
  InstrumentChipData chip;
} Instrument;

// Grooves

typedef struct Groove {
  uint8_t speed[16];
} Groove;

// Phrases

typedef struct PhraseRow {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t fx[3][2];
} PhraseRow;

typedef struct Phrase {
  PhraseRow rows[16];
} Phrase;

// Chains

typedef struct ChainRow {
  uint16_t phrase;
  uint8_t transpose;
} ChainRow;

typedef struct Chain {
  ChainRow rows[16];
} Chain;

// Project

typedef struct PitchTable {
  char name[PROJECT_PITCH_TABLE_TITLE_LENGTH + 1];
  uint16_t octaveSize;
  uint16_t length;
  uint16_t values[PROJECT_MAX_PITCHES];
  char noteNames[PROJECT_MAX_PITCHES][4];
} PitchTable;

typedef struct Project {
  char title[PROJECT_TITLE_LENGTH + 1];
  char author[PROJECT_TITLE_LENGTH + 1];

  float tickRate;
  enum ChipType chipType;
  ChipSetup chipSetup;
  int chipsCount;
  uint8_t linearPitch;

  int tracksCount;

  PitchTable pitchTable;

  uint16_t song[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
  Chain chains[PROJECT_MAX_CHAINS];
  Phrase phrases[PROJECT_MAX_PHRASES];
  Groove grooves[PROJECT_MAX_GROOVES];
  Instrument instruments[PROJECT_MAX_INSTRUMENTS];
  Table tables[PROJECT_MAX_TABLES];
} Project;

extern char projectFileError[41];

// Fill FX names (call this first before loading any projects)
void fillFXNames();
// Initialize an empty project
void projectInit(Project* p);
// Load project from a file
int projectLoad(Project* p, const char* path);
// Save project to a file
int projectSave(Project* p, const char* path);
// Save instrument to a file
int instrumentSave(Project* p, const char* path, int instrumentIdx);
// Load instrument from a file
int instrumentLoad(Project* p, const char* path, int instrumentIdx);

// Is chain empty?
int8_t chainIsEmpty(Project* p, int chain);
// Is phrase empty?
int8_t phraseIsEmpty(Project* p, int phrase);
// Is instrument empty?
int8_t instrumentIsEmpty(Project* p, int instrument);
// Is table empty?
int8_t tableIsEmpty(Project* p, int table);
// Is groove empty?
int8_t grooveIsEmpty(Project* p, int groove);
// Note name in phrase
char* noteName(Project* p, uint8_t note);
// Get number of tracks for a chip at index
int projectGetChipTracks(Project* p, int chipIndex);
// Get total number of tracks for the project
int projectGetTotalTracks(Project* p);
// Clear a single phrase with proper initialization
void phraseClear(Phrase* phrase);
// Clear a single chain with proper initialization
void chainClear(Chain* chain);
// Clear a single instrument with proper initialization
void instrumentClear(Instrument* instrument);
// Clear a single table with proper initialization
void tableClear(Table* table);

#endif