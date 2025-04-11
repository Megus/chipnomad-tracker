#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdint.h>

#define PROJECT_MAX_TRACKS (10)
#define PROJECT_MAX_LENGTH (256)
#define PROJECT_MAX_CHAINS (255)
#define PROJECT_MAX_PHRASES (1024)
#define PROJECT_MAX_INSTRUMENTS (128)
#define PROJECT_MAX_TABLES (128)

#define EMPTY_VALUE_8 (255)
#define EMPTY_VALUE_16 (32767)

///////////////////////////////////////////////////////////////////////////////
// Song data structures

enum InstrumentType {
  instNone,
  instAY,
};

// Tables
struct TableRow {
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t volumeOffset;
  uint8_t fx[4][2];
};

struct Table {
  struct TableRow rows[16];
};

struct Instrument {
  enum InstrumentType type;
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
};

// Music
struct PhraseRow {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t fx[3][2];
};

struct Phrase {
  uint8_t hasNoNotes;
  struct PhraseRow rows[16];
};

struct Chain {
  uint8_t hasNoNotes;
  uint16_t phrases[16];
  int8_t transpose[16];
};

struct Project {
  int tracksCount;
  // TODO: Chip setup, pitch table

  uint16_t song[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
  struct Chain chains[PROJECT_MAX_CHAINS];
  struct Phrase phrases[PROJECT_MAX_PHRASES];
  struct Instrument instruments[PROJECT_MAX_INSTRUMENTS];
  struct Table tables[PROJECT_MAX_TABLES];
};

// Current song
extern struct Project project;

///////////////////////////////////////////////////////////////////////////////
// Project functions

// Initialize an empty project
void projectInit(void);
// Load project from a file
int projectLoad(const char* path);
// Save project to a file
int projectSave(const char* path);

int isChainEmpty(int chain);
int isPhraseEmpty(int phrase);


#endif