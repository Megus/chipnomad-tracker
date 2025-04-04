#ifndef __SONG_H__
#define __SONG_H__

#include <stdint.h>

#define SONG_MAX_TRACKS (10)
#define SONG_MAX_LENGTH (256)
#define SONG_MAX_CHAINS (255)
#define SONG_MAX_PHRASES (1024)
#define SONG_MAX_INSTRUMENTS (128)
#define SONG_MAX_TABLES (128)

///////////////////////////////////////////////////////////////////////////////
// Song data structures

enum InstrumentType {
  instAY,
};

// Tables
struct TableRow {
  uint8_t pitchOffset;
  uint8_t fx[3][2];
};

struct Table {
  struct TableRow rows[16];
};

// Instruments
struct InstrumentRowAY {
  uint8_t tone;
  uint8_t noise;
  uint8_t env;
  uint8_t volume;
  uint8_t volumeChange;
};

union InstrumentRowChipFeature {
  struct InstrumentRowAY ay;
};

struct InstrumentRow {
  uint8_t isAbsolutePitch;
  uint8_t pitch;
  union InstrumentRowChipFeature chip;
  uint8_t fx[3][2];
};

struct Instrument {
  enum InstrumentType type;
  uint8_t tableSpeed;
  struct InstrumentRow rows[16];
};

// Music
struct PhraseRow {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t fx[3][2];
};

struct Phrase {
  struct PhraseRow rows[16];
};

struct Chain {
  int phrases[16];
  int transpose[16];
};

struct Song {
  int tracksCount;
  // TODO: Chip setup, pitch table

  uint8_t song[SONG_MAX_LENGTH][SONG_MAX_TRACKS];
  struct Chain chains[SONG_MAX_CHAINS];
  struct Phrase phrases[SONG_MAX_PHRASES];
  struct Instrument instruments[SONG_MAX_INSTRUMENTS];
  struct Table tables[SONG_MAX_TABLES];
};

// Current song
extern struct Song song;

///////////////////////////////////////////////////////////////////////////////
// Song functions

// Initialize a new empty song
void songInit(void);


#endif