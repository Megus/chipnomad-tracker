#ifndef __AUDIOMANAGER_H__
#define __AUDIOMANAGER_H__

#include <common.h>
#include <chipnomad_lib.h>

#define AUDIO_MAX_CHIPS (1)

typedef void FrameCallback(void* userdata);

typedef struct AudioManager {
  int (*start)(int sampleRate, int audioBufferSize);
  void (*pause)(void);
  void (*resume)(void);
  void (*stop)();
} AudioManager;

// Singleton AudioManager struct
extern AudioManager audioManager;

#endif
