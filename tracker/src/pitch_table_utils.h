#ifndef __PITCH_TABLE_UTILS_H__
#define __PITCH_TABLE_UTILS_H__

#include "chipnomad_lib.h"

// Load pitch table from CSV file
// Returns 0 on success, 1 on error
int pitchTableLoadCSV(Project* p, const char* path);

// Save pitch table to CSV file in specified folder
// Returns 0 on success, 1 on error
int pitchTableSaveCSV(Project* p, const char* folderPath, const char* filename);

// Calculate 12TET pitch table for AY chip
void calculatePitchTableAY(Project* p);

// Calculate 12TET linear pitch table (values in cents, C-4 = 60)
void calculateLinearPitchTable12TET(Project* p);

// Reinitialize pitch table based on linear pitch setting
void reinitializePitchTable(Project* p);

#endif