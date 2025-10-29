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
  fxVOL, // Volume (relative)
  fxRET, // Retrigger
  fxDEL, // Delay
  fxOFF, // Off
  fxKIL, // Kill note
  fxTIC, // Table speed
  fxTBL, // Set instrument table
  fxTBX, // Set aux table
  fxTHO, // Table hop
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

struct FXName {
  enum FX fx;
  char name[4];
};

extern struct FXName fxNames[256]; // All names
extern struct FXName fxNamesCommon[]; // Common FX names
extern int fxCommonCount;
extern struct FXName fxNamesAY[]; // AY FX names
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

struct ChipSetupAY {
  int clock;
  uint8_t isYM;
  enum StereoModeAY stereoMode;
  uint8_t stereoSeparation;
};

union ChipSetup {
  struct ChipSetupAY ay;
};

// Tables

struct TableRow {
  uint8_t pitchFlag;
  uint8_t pitchOffset;
  uint8_t volume;
  uint8_t fx[4][2];
};

struct Table {
  struct TableRow rows[16];
};

// Instruments

enum InstrumentType {
  instNone = 0,
  instAY = 1,
};

struct InstrumentAY {
  uint8_t veA;
  uint8_t veD;
  uint8_t veS;
  uint8_t veR;
  uint8_t autoEnvN; // 0 - no auto-env
  uint8_t autoEnvD;
};

union InstrumentChipData {
  struct InstrumentAY ay;
};

struct Instrument {
  uint8_t type; // enum InstrumentType
  char name[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
  union InstrumentChipData chip;
};

// Grooves

struct Groove {
  uint8_t speed[16];
};

// Phrases

struct PhraseRow {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t fx[3][2];
};

struct Phrase {
  struct PhraseRow rows[16];
};

// Chains

struct ChainRow {
  uint16_t phrase;
  uint8_t transpose;
};

struct Chain {
  struct ChainRow rows[16];
};

// Project

struct PitchTable {
  char name[PROJECT_PITCH_TABLE_TITLE_LENGTH + 1];
  uint16_t octaveSize;
  uint16_t length;
  uint16_t values[PROJECT_MAX_PITCHES];
  char noteNames[PROJECT_MAX_PITCHES][4];
};

struct Project {
  char title[PROJECT_TITLE_LENGTH + 1];
  char author[PROJECT_TITLE_LENGTH + 1];

  float tickRate;
  enum ChipType chipType;
  union ChipSetup chipSetup;
  int chipsCount;

  int tracksCount;

  struct PitchTable pitchTable;

  uint16_t song[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
  struct Chain chains[PROJECT_MAX_CHAINS];
  struct Phrase phrases[PROJECT_MAX_PHRASES];
  struct Groove grooves[PROJECT_MAX_GROOVES];
  struct Instrument instruments[PROJECT_MAX_INSTRUMENTS];
  struct Table tables[PROJECT_MAX_TABLES];
};

// Current project
extern struct Project project;

extern char projectFileError[41];

// Fill FX names (call this first before loading any projects)
void fillFXNames();
// Initialize an empty project
void projectInit(struct Project* p);
// Load project from a file
int projectLoad(const char* path);
// Save project to a file
int projectSave(const char* path);

// Is chain empty?
int8_t chainIsEmpty(int chain);
// Is phrase empty?
int8_t phraseIsEmpty(int phrase);
// Is instrument empty?
int8_t instrumentIsEmpty(int instrument);
// Is table empty?
int8_t tableIsEmpty(int table);
// Is groove empty?
int8_t grooveIsEmpty(int groove);
// Note name in phrase
char* noteName(uint8_t note);

#endif