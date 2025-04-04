#ifndef __AUDIOMANAGER_H__
#define __AUDIOMANAGER_H__

#include <common.h>
#include <chips.h>

#define AUDIO_MAX_CHIPS (1)

typedef void FrameCallback(void* userdata);

struct AudioManager {
  float frameRate;
  struct SoundChip chips[AUDIO_MAX_CHIPS];

  int (*start)(int sampleRate, int audioBufferSize, float frameRate);
  void (*initChips)(void);
  void (*setFrameCallback)(FrameCallback *callback, void* userdata);
  void (*pause)(void);
  void (*resume)(void);
  void (*stop)();
};

// Singleton AudioManager struct
extern struct AudioManager audioManager;

#endif