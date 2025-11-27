#include <pitch_table_utils.h>
#include <corelib_file.h>
#include <chipnomad_lib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

// Create 12TET scale
void calculatePitchTableAY(struct Project* p) {
  static char noteStrings[12][4] = { "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1" };

  float clock = (float)(p->chipSetup.ay.clock);
  int octaves = 9;
  float cfreq = 16.35159783; // C-0 frequency for A4 = 440Hz. It's too low for 1.75MHz AY, but we'll keep it
  float freq = cfreq;
  float semitone = powf(2., 1. / 12.);

  sprintf(p->pitchTable.name, "12TET %dHz", p->chipSetup.ay.clock);
  p->pitchTable.length = octaves * 12;
  p->pitchTable.octaveSize = 12;

  for (int o = 0; o < octaves; o++) {
    for (int c = 0; c < 12; c++) {
      noteStrings[c][2] = 48 + o;

      float periodf = clock / 16. / freq;

      float freqL = clock / 16. / floorf(periodf);
      float freqH = clock / 16. / ceilf(periodf);

      int period = (fabsf(freqL - freq) < fabsf(freqH - freq)) ? floorf(periodf) : ceilf(periodf);
      if (period > 4095) period = 4095; // AY only has 12 bits for period

      p->pitchTable.values[o * 12 + c] = period;
      strcpy(p->pitchTable.noteNames[o * 12 + c], noteStrings[c]);

      freq *= semitone;
    }

    // Reset frequency calculation on each octave to minimize rounding errors
    cfreq *= 2.;
    freq = cfreq;
  }
}
