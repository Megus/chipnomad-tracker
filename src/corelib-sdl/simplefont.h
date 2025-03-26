#ifndef SIMPLEFONT_H
#define SIMPLEFONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSTOP 1,2,3,4,5

typedef struct {
  uint8_t width;
  uint8_t height;
  uint8_t * data;
  uint16_t count;
} font_t;

extern font_t font5x7;

#ifdef __cplusplus
}
#endif

#endif