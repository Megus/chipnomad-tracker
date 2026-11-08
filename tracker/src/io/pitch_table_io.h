#ifndef __PITCH_TABLE_UTILS_H__
#define __PITCH_TABLE_UTILS_H__

#include "chipnomad_lib.h"

// Load pitch table from CSV file
// Returns 1 on success, 0 on error
int pitchTableLoadCSV(chipnomad::Project& p, const char* path);

// Save pitch table to CSV file in specified folder
// Returns 1 on success, 0 on error
int pitchTableSaveCSV(chipnomad::Project& p, const char* folderPath, const char* filename);

#endif
