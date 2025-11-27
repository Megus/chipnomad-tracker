#ifndef __AUDIOMANAGER_H__
#define __AUDIOMANAGER_H__

#include <common.h>
#include <chipnomad_lib.h>

#define AUDIO_MAX_CHIPS (1)

typedef void FrameCallback(void* userdata);

struct AudioManager {
  float tickRate;

  int (*start)(int sampleRate, int audioBufferSize, float tickRate);
  void (*initChips)(void);
  void (*setFrameCallback)(FrameCallback *callback, void* userdata);
  void (*render)(float* buffer, int stereoSamples);
  void (*pause)(void);
  void (*resume)(void);
  void (*stop)();
};

// Singleton AudioManager struct
extern struct AudioManager audioManager;

#endif
