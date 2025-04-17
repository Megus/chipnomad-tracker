#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdint.h>

#define PROJECT_MAX_TRACKS (10)
#define PROJECT_MAX_LENGTH (256)
#define PROJECT_MAX_CHAINS (255)
#define PROJECT_MAX_PHRASES (1024)
#define PROJECT_MAX_GROOVES (16)
#define PROJECT_MAX_INSTRUMENTS (128)
#define PROJECT_MAX_TABLES (128)
#define PROJECT_MAX_CHIPS (3)
#define PROJECT_MAX_PITCHES (254)

#define NOTE_OFF (254)
#define EMPTY_VALUE_8 (255)
#define EMPTY_VALUE_16 (32767)

///////////////////////////////////////////////////////////////////////////////
// Song data structures

// Chips

enum ChipType {
  chipAY = 0,
  chipTotalCount,
};

struct ChipSetupAY {
  int clock;
  int isYM;
  uint8_t panA;
  uint8_t panB;
  uint8_t panC;
};

union ChipSetup {
  struct ChipSetupAY ay;
};

// Tables

struct Table {
  uint8_t pitchFlags[16];
  int8_t pitchOffsets[16];
  int8_t volumeOffsets[16];
  uint8_t fx[16][4][2];
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
  uint8_t autoEnvOffset;
};

union InstrumentChipData {
  struct InstrumentAY ay;
};

struct Instrument {
  enum InstrumentType type;
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
  union InstrumentChipData chip;
};

// Grooves
struct Groove {
  uint8_t speed[16];
};

// Phrases

struct Phrase {
  int8_t hasNotes;
  uint8_t notes[16];
  uint8_t instruments[16];
  uint8_t volumes[16];
  uint8_t fx[16][3][2];
};

// Chains

struct Chain {
  int8_t hasNotes;
  uint16_t phrases[16];
  uint8_t transpose[16];
};

// Project

struct PitchTable {
  char title[32];
  uint16_t octaveSize;
  uint16_t length;
  uint16_t values[PROJECT_MAX_PITCHES];
  char names[PROJECT_MAX_PITCHES][4];
};

struct Project {
  char title[32];
  char author[32];

  float frameRate;
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

// Current song
extern struct Project project;

///////////////////////////////////////////////////////////////////////////////
// Project functions

// Initialize an empty project
void projectInit(struct Project* p);
// Load project from a file
int projectLoad(const char* path);
// Save project to a file
int projectSave(const char* path);

// Is chain empty?
int8_t chainIsEmpty(int chain);
// Does chain have notes?
int8_t chainHasNotes(int chain);
// Is phrase empty?
int8_t phraseIsEmpty(int phrase);
// Does phrase have notes?
int8_t phraseHasNotes(int phrase);
// Is instrument empty?
int8_t instrumentIsEmpty(int instrument);
// Is table empty?
int8_t tableIsEmpty(int table);
// Is groove empty?
int8_t grooveIsEmpty(int groove);
// FX name
char* fxName(uint8_t fx);

#endif