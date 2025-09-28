#ifndef __PITCH_TABLE_CSV_H__
#define __PITCH_TABLE_CSV_H__

// Load pitch table from CSV file
// Returns 0 on success, 1 on error
int pitchTableLoadCSV(const char* path);

// Save pitch table to CSV file in specified folder
// Returns 0 on success, 1 on error
int pitchTableSaveCSV(const char* folderPath, const char* filename);

#endif