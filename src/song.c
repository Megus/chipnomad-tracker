#include <string.h>
#include <song.h>

struct Song song;

void songInit() {
  // Init for AY
  song.tracksCount = 3;

  // Clean song structure
  for (int c = 0; c < SONG_MAX_LENGTH; c++) {
    for (int d = 0; d < SONG_MAX_TRACKS; d++) {
      song.song[c][d] = 0xff; // FF is empty
    }
  }

  song.song[0][0] = 0x00;

}
