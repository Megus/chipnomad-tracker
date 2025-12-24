#ifndef __AUDIOMANAGER_H__
#define __AUDIOMANAGER_H__

#include "common.h"
#include "chipnomad_lib.h"

typedef void FrameCallback(void* userdata);

typedef struct AudioManager {
  int (*start)(int sampleRate, int audioBufferSize);
  void (*pause)(void);
  void (*resume)(void);
  void (*stop)();
  void (*toggleTrackMute)(int trackIdx);
  void (*toggleTrackSolo)(int trackIdx);
  uint8_t trackStates[PROJECT_MAX_TRACKS];
} AudioManager;

// Singleton AudioManager struct
extern AudioManager audioManager;

// Track state constants
#define TRACK_NORMAL 0
#define TRACK_SOLO 1
#define TRACK_MUTED 2

#endif
