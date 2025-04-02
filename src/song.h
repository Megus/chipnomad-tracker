#ifndef __SONG_H__
#define __SONG_H__

#include <stdint.h>

#define SONG_MAX_TRACKS (10)
#define SONG_MAX_LENGTH (256)
#define SONG_MAX_CHAINS (255)
#define SONG_MAX_PHRASES (1024)
#define SONG_MAX_INSTRUMENTS (256)
#define SONG_MAX_TABLES (256)
#define SONG_PHRASE_LENGTH (16)

// Song data structure

struct Chain {

};

struct PhraseRow {

};

struct Phrase {

  struct PhraseRow rows[SONG_PHRASE_LENGTH];
};

struct Instrument {

};

struct Table {

};

struct Song {
  int tracksCount;

  uint8_t song[SONG_MAX_LENGTH][SONG_MAX_TRACKS];
  struct Chain chains[SONG_MAX_CHAINS];
  struct Phrase phrases[SONG_MAX_PHRASES];
  struct Instrument instruments[SONG_MAX_INSTRUMENTS];
  struct Table tables[SONG_MAX_TABLES];
};

// Current song
extern struct Song song;

// Song functions

// Initialize a new empty song
void songInit(void);


#endif