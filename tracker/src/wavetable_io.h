#ifndef __WAVETABLE_IO_H__
#define __WAVETABLE_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Save wavetables to a file
// Parameters:
//   path: File path to save to
//   wavetables: Pointer to wavetable array (256 wavetables of 32 4-bit values each)
//   startIndex: Starting wavetable index (0-255)
//   count: Number of wavetables to save (1-256)
// Returns:
//   1 on success, 0 on failure
// Notes:
//   - Saves count wavetables starting from startIndex
//   - If startIndex + count > 256, only saves up to wavetable 255
//   - Format: plain text, one line per wavetable, 32 hex digits per line
int wavetableSave(const char* path, uint8_t wavetables[256][32], int startIndex, int count);

// Load wavetables from a file
// Parameters:
//   path: File path to load from
//   wavetables: Pointer to wavetable array (256 wavetables of 32 4-bit values each)
//   startIndex: Starting wavetable index to load into (0-255)
// Returns:
//   Number of wavetables loaded (0 if failed)
// Notes:
//   - Loads wavetables starting at startIndex
//   - If file contains more wavetables than fit (startIndex + file lines > 256), stops at 255
//   - Validates file format: each line must be exactly 32 hex digits (0-9, A-F, a-f)
//   - Ignores empty lines and lines starting with '#' (comments)
int wavetableLoad(const char* path, uint8_t wavetables[256][32], int startIndex);


#ifdef __cplusplus
}
#endif

#endif // __WAVETABLE_IO_H__
