#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

const char* byteToHex(uint8_t byte);
const char* byteToHexOrEmpty(uint8_t byte);

int min(int a, int b);
int max(int a, int b);

// Clamp value to a range [min, max]
int8_t clampInt8(int value, int8_t min, int8_t max);
uint8_t clampUInt8(int value, uint8_t min, uint8_t max);
int16_t clampInt16(int value, int16_t min, int16_t max);
uint16_t clampUInt16(int value, uint16_t min, uint16_t max);
int clampInt(int value, int min, int max);

// Convert cents value to frequency in Hz (with safeguards)
float centsToFrequency(int cents);

// Simple pseudo-random number generator. Returns a value in range [0, 65535]
uint16_t utilsRandom(void);

#endif