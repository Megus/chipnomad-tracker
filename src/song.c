#include <song.h>

struct Song song;

void songInit() {
  song.channelsCount = 3;

  // Clean song structure
  for (int c = 0; c < SONG_MAX_LENGTH * SONG_MAX_CHANNELS; c++) {
    song.song[c] = 0xff; // FF is empty
  }

}
