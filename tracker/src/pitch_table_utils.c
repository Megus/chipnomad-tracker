#include "pitch_table_utils.h"
#include "corelib/corelib_file.h"
#include "chipnomad_lib.h"
#include "playback_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int pitchTableLoadCSV(Project* project, const char* path) {
  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;

  char* line;
  int noteIndex = 0;
  int noteCol = -1, valueCol = -1;
  const char* expectedValueColumn = project->linearPitch ? "Cents" : "Period";

  // Read header line and find column positions
  line = fileReadString(fileId);
  if (!line) {
    fileClose(fileId);
    return 1;
  }

  // Parse header to find Note and value columns
  char* token = strtok(line, ",");
  int col = 0;
  while (token) {
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
    fileClose(fileId);
    return 1;
  }

  // Read data lines
  while ((line = fileReadString(fileId)) != NULL && noteIndex < PROJECT_MAX_PITCHES) {
    char noteName[4] = "";
    int value = 0;
    int foundNote = 0, foundValue = 0;

    // Parse CSV line
    token = strtok(line, ",");
    col = 0;
    while (token) {
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
      strncpy(project->pitchTable.noteNames[noteIndex], noteName, 3);
      project->pitchTable.noteNames[noteIndex][3] = 0;
      project->pitchTable.values[noteIndex] = value;
      noteIndex++;
    }
  }

  fileClose(fileId);

  if (noteIndex > 0) {
    project->pitchTable.length = noteIndex;

    // Calculate octave size (find first note name change)
    char firstOctave = project->pitchTable.noteNames[0][2];
    project->pitchTable.octaveSize = 12; // Default
    for (int i = 1; i < noteIndex; i++) {
      if (project->pitchTable.noteNames[i][2] != firstOctave) {
        project->pitchTable.octaveSize = i;
        break;
      }
    }

    return 0;
  }

  return 1;
}

int pitchTableSaveCSV(Project* project, const char* folderPath, const char* filename) {
  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s/%s.csv", folderPath, filename);

  int fileId = fileOpen(fullPath, 1);
  if (fileId == -1) return 1;

  // Write header with appropriate column name
  const char* valueColumn = project->linearPitch ? "Cents" : "Period";
  filePrintf(fileId, "Note,%s\n", valueColumn);

  // Write data
  for (int i = 0; i < project->pitchTable.length; i++) {
    filePrintf(fileId, "%s,%d\n", project->pitchTable.noteNames[i], project->pitchTable.values[i]);
  }

  fileClose(fileId);
  return 0;
}

// Create 12TET scale
void calculatePitchTableAY(Project* p) {
  static char noteStrings[12][4] = { "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1" };

  float clock = (float)(p->chipSetup.ay.clock);
  int octaves = 9;
  int startMidiNote = 12; // Start from C-0 (MIDI 12)

  sprintf(p->pitchTable.name, "12TET %dHz", p->chipSetup.ay.clock);
  p->pitchTable.length = octaves * 12;
  p->pitchTable.octaveSize = 12;

  for (int o = 0; o < octaves; o++) {
    for (int c = 0; c < 12; c++) {
      noteStrings[c][2] = 48 + o;

      int midiNote = startMidiNote + o * 12 + c;
      float freq = centsToFrequency(midiNote * 100);
      int period = frequencyToAYPeriod(freq, (int)clock);

      p->pitchTable.values[o * 12 + c] = period;
      strcpy(p->pitchTable.noteNames[o * 12 + c], noteStrings[c]);
    }
  }
}

// Create 12TET linear pitch table (values in cents)
void calculateLinearPitchTable12TET(Project* p) {
  static char noteStrings[12][4] = { "C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0" };
  
  strcpy(p->pitchTable.name, "12TET Linear");
  int octaves = 9;
  p->pitchTable.length = octaves * 12;
  p->pitchTable.octaveSize = 12;
  
  int startMidiNote = 12; // Start from C-0 (MIDI 12)
  
  for (int o = 0; o < octaves; o++) {
    for (int c = 0; c < 12; c++) {
      noteStrings[c][2] = 48 + o; // ASCII '0' + octave number
      
      int midiNote = startMidiNote + o * 12 + c;
      p->pitchTable.values[o * 12 + c] = midiNote * 100; // Value in cents
      strcpy(p->pitchTable.noteNames[o * 12 + c], noteStrings[c]);
    }
  }
}

// Reinitialize pitch table based on linear pitch setting
void reinitializePitchTable(Project* p) {
  if (p->linearPitch) {
    calculateLinearPitchTable12TET(p);
  } else {
    calculatePitchTableAY(p);
  }
}


