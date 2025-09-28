#include <pitch_table_csv.h>
#include <corelib_file.h>
#include <project.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int pitchTableLoadCSV(const char* path) {
  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;

  char* line;
  int noteIndex = 0;
  int noteCol = -1, periodCol = -1;

  // Read header line and find column positions
  line = fileReadString(fileId);
  if (!line) {
    fileClose(fileId);
    return 1;
  }

  // Parse header to find Note and Period columns
  char* token = strtok(line, ",");
  int col = 0;
  while (token) {
    if (strcmp(token, "Note") == 0) {
      noteCol = col;
    } else if (strcmp(token, "Period") == 0) {
      periodCol = col;
    }
    token = strtok(NULL, ",");
    col++;
  }

  // Check if both required columns were found
  if (noteCol == -1 || periodCol == -1) {
    fileClose(fileId);
    return 1;
  }

  // Read data lines
  while ((line = fileReadString(fileId)) != NULL && noteIndex < PROJECT_MAX_PITCHES) {
    char noteName[4] = "";
    int period = 0;
    int foundNote = 0, foundPeriod = 0;

    // Parse CSV line
    token = strtok(line, ",");
    col = 0;
    while (token) {
      if (col == noteCol) {
        strncpy(noteName, token, 3);
        noteName[3] = 0;
        foundNote = 1;
      } else if (col == periodCol) {
        period = atoi(token);
        foundPeriod = 1;
      }
      token = strtok(NULL, ",");
      col++;
    }

    // Add entry if both values were found
    if (foundNote && foundPeriod && strlen(noteName) > 0) {
      strncpy(project.pitchTable.noteNames[noteIndex], noteName, 3);
      project.pitchTable.noteNames[noteIndex][3] = 0;
      project.pitchTable.values[noteIndex] = period;
      noteIndex++;
    }
  }

  fileClose(fileId);

  if (noteIndex > 0) {
    project.pitchTable.length = noteIndex;

    // Calculate octave size (find first note name change)
    char firstOctave = project.pitchTable.noteNames[0][2];
    project.pitchTable.octaveSize = 12; // Default
    for (int i = 1; i < noteIndex; i++) {
      if (project.pitchTable.noteNames[i][2] != firstOctave) {
        project.pitchTable.octaveSize = i;
        break;
      }
    }

    return 0;
  }

  return 1;
}

int pitchTableSaveCSV(const char* folderPath, const char* filename) {
  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s/%s.csv", folderPath, filename);

  int fileId = fileOpen(fullPath, 1);
  if (fileId == -1) return 1;

  // Write header
  filePrintf(fileId, "Note,Period\n");

  // Write data
  for (int i = 0; i < project.pitchTable.length; i++) {
    filePrintf(fileId, "%s,%d\n", project.pitchTable.noteNames[i], project.pitchTable.values[i]);
  }

  fileClose(fileId);
  return 0;
}