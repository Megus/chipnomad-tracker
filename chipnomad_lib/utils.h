#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

const char* byteToHex(uint8_t byte);
const char* byteToHexOrEmpty(uint8_t byte);

int min(int a, int b);
int max(int a, int b);

// Convert cents value to frequency in Hz (with safeguards)
float centsToFrequency(int cents);

#endif