#include "pitch_table_io.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char lineBuffer[1024];

// Strip trailing whitespace (including newline) from a string
static void stripTrailingWhitespace(char* str) {
  if (!str) return;
  int len = strlen(str);
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' ' || str[len - 1] == '\t')) {
    str[len - 1] = '\0';
    len--;
  }
}

int pitchTableLoadCSV(chipnomad::Project& p, const char* path) {
  FILE* file = fopen(path, "r");
  if (file == NULL) return 0;

  int noteIndex = 0;
  int noteCol = -1, valueCol = -1;
  const char* expectedValueColumn = p.linearPitch ? "Cents" : "Period";

  // Read header line and find column positions
  if (fgets(lineBuffer, sizeof(lineBuffer), file) == NULL) {
    fclose(file);
    return 0;
  }

  // Parse header to find Note and value columns
  char* token = strtok(lineBuffer, ",");
  int col = 0;
  while (token) {
    stripTrailingWhitespace(token);
    if (strcmp(token, "Note") == 0) {
      noteCol = col;
    } else if (strcmp(token, expectedValueColumn) == 0) {
      valueCol = col;
    }
    token = strtok(NULL, ",");
    col++;
  }

  // Check if both required columns were found
  if (noteCol == -1 || valueCol == -1) {
    fclose(file);
    return 0;
  }

  // Read data lines
  while (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL && noteIndex < PROJECT_MAX_PITCHES) {
    char noteName[4] = "";
    int value = 0;
    int foundNote = 0, foundValue = 0;

    // Parse CSV line
    token = strtok(lineBuffer, ",");
    col = 0;
    while (token) {
      stripTrailingWhitespace(token);
      if (col == noteCol) {
        strncpy(noteName, token, 3);
        noteName[3] = 0;
        foundNote = 1;
      } else if (col == valueCol) {
        value = atoi(token);
        foundValue = 1;
      }
      token = strtok(NULL, ",");
      col++;
    }

    // Add entry if both values were found
    if (foundNote && foundValue && strlen(noteName) > 0) {
      strncpy(p.pitchTable.noteNames[noteIndex], noteName, 3);
      p.pitchTable.noteNames[noteIndex][3] = 0;
      p.pitchTable.values[noteIndex] = value;
      noteIndex++;
    }
  }

  fclose(file);

  if (noteIndex > 0) {
    p.pitchTable.length = noteIndex;

    // Calculate octave size (find first note name change)
    char firstOctave = p.pitchTable.noteNames[0][2];
    p.pitchTable.octaveSize = 12; // Default
    for (int i = 1; i < noteIndex; i++) {
      if (p.pitchTable.noteNames[i][2] != firstOctave) {
        p.pitchTable.octaveSize = i;
        break;
      }
    }

    return 1;
  }

  return 0;
}

int pitchTableSaveCSV(chipnomad::Project& p, const char* folderPath, const char* filename) {
  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s/%s.csv", folderPath, filename);

  FILE* file = fopen(fullPath, "w");
  if (file == NULL) return 0;

  // Write header with appropriate column name
  const char* valueColumn = p.linearPitch ? "Cents" : "Period";
  fprintf(file, "Note,%s\n", valueColumn);

  // Write data
  for (int i = 0; i < p.pitchTable.length; i++) {
    fprintf(file, "%s,%d\n", p.pitchTable.noteNames[i], p.pitchTable.values[i]);
  }

  fclose(file);
  return 1;
}
