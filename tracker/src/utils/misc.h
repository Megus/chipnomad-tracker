#ifndef __MISC_H__
#define __MISC_H__

const char* getEnvelopeShapeASCII(int shape);

void trimString(char* str); // Trim leading and trailing spaces from string
void copyString(char *dest, const char* src, size_t maxLength); // Safe version of strncpy
const char* stringTail(const char* str, int maxLength); // Returns the tail of a long string (or the whole string if it's not too long)

#endif
