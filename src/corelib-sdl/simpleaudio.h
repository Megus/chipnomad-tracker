#ifndef SIMPLEAUDIO_H
#define SIMPLEAUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <simplegfx.h>
#include <ayumi.h>

extern double volume;

extern struct ayumi *chip;

int audio_setup(void);
void audio_cleanup(void);
void audio_callback(void * userdata, uint8_t * stream, int len);

#ifdef __cplusplus
}
#endif

#endif