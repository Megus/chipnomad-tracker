#include "screen_instrument.h"
#include "waveform_display.h"
#include "sample_utils.h"
#include "corelib_gfx.h"
#include "corelib/corelib_file.h"
#include "utils.h"
#include "file_browser.h"
#include "import/import_wav.h"
#include <string.h>

// Preview configuration
#define PREVIEW_ROW 16
#define PREVIEW_COL 0
#define PREVIEW_WIDTH_CHARS 32
#define PREVIEW_HEIGHT_CHARS 3

// Screen layout:
// y 0: INSTRUMENT 00
// y 2: Type  AY Sample  Load Save
// y 3: Name  _______________
// y 4: Transp. On       Tbl. Tic 01
// y 5: (spacing)
// y 6: Sample  Load  Lift
// y 7: sample_filename.wav
// y 8: Rate  [8000 ]    Tone   [On ]
// y 9: Start [0000]     Pitch  [+0  ]
// y 10: Len   [0000]     Fine   [+0  ]
// y 11: LpSt  [0000]
// y 12: LpEnd [0000]
// y 13: Pitch [+0  ]    Noise  [Off]
// y 14: Fine  [+0  ]    Period [00]
// y 15: (spacing)
// y 16-18: (sample preview)
//
// Logical rows:
// 0-2: common (type, name, transpose/tic)
// 3: Load button (col 0) + Lift button (col 1)
// 4: Rate         | Tone on/off
// 5: Start        | Tone pitch
// 6: Length       | Tone fine
// 7: Loop Start   | (empty)
// 8: Pitch        | Noise on/off
// 9: Fine         | Noise period

#define COL_LEFT_X    0
#define COL_LEFT_VAL  8
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 26

#define ROW_TOTAL 10

Bitmap* samplePreviewBitmap = NULL;  // Non-static so instrument screen can access it
int isLoopPreview = 0;  // Flag to track if we're showing loop preview (non-static so instrument screen can access it)

static int rowToY(int row) {
  switch (row) {
    case 3: return 6;
    case 4: return 8;
    case 5: return 9;
    case 6: return 10;
    case 7: return 11;
    case 8: return 12;
    case 9: return 13;
    default: return 0;
  }
}

void updateSamplePreview(void) {
  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;

  if (!samplePreviewBitmap) {
    samplePreviewBitmap = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, PREVIEW_HEIGHT_CHARS);
  }

  if (smp->sampleData && smp->sampleLength > 0) {
    uint16_t previewStart = smp->sampleStart;
    uint16_t previewEnd = smp->sampleStart + smp->sampleLength;
    if (previewEnd > smp->fileLength) previewEnd = smp->fileLength;

    renderSamplePreview(samplePreviewBitmap, smp->sampleData, previewStart, previewEnd);
  } else {
    // Clear preview if no sample
    if (samplePreviewBitmap) {
      memset(samplePreviewBitmap->data, 0, samplePreviewBitmap->widthPixels * samplePreviewBitmap->heightPixels);
    }
  }
}

static void onSampleLoaded(const char* path) {
  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;

  // Load the WAV file
  uint16_t sampleLength;
  uint16_t sampleRate;
  WavLoadResult result;
  uint8_t* sampleData = loadWavFile(path, PROJECT_MAX_SAMPLE_SIZE, &sampleLength, &sampleRate, &result);

  if (sampleData == NULL) {
    // Show error message
    screenMessage(MESSAGE_TIME, "Error: %s", getWavLoadErrorMessage(result));
    screenSetup(&screenInstrument, cInstrument);
    return;
  }

  // Copy sample data to instrument
  if (smp->sampleData != NULL) {
    free(smp->sampleData);
  }
  smp->sampleData = sampleData;
  smp->fileLength = sampleLength;      // Size of data in file
  smp->sampleLength = sampleLength;    // Playback length (initially same as file length)
  smp->sampleRate = sampleRate;

  // Reset sample playback parameters to sensible defaults
  smp->sampleStart = 0;
  smp->sampleLoopStart = sampleLength;  // No loop (loop start at end)

  // Extract filename for display
  char* lastSeparator = strrchr(path, PATH_SEPARATOR);
  const char* filename = lastSeparator ? lastSeparator + 1 : path;

  // Strip extension and copy name
  const char* dot = strrchr(filename, '.');
  int nameLen = dot ? (int)(dot - filename) : (int)strlen(filename);
  if (nameLen > PROJECT_INSTRUMENT_NAME_LENGTH) nameLen = PROJECT_INSTRUMENT_NAME_LENGTH;
  strncpy(smp->sampleName, filename, nameLen);
  smp->sampleName[nameLen] = 0;

  // Save the directory path
  if (lastSeparator) {
    int pathLen = lastSeparator - path;
    if (pathLen < PATH_LENGTH) {
      strncpy(appSettings.samplePath, path, pathLen);
      appSettings.samplePath[pathLen] = 0;
    }
  }

  projectModified = 1;
  screenMessage(MESSAGE_TIME, "Sample loaded: %d bytes @ %d Hz", sampleLength, sampleRate);
  updateSamplePreview();
  screenSetup(&screenInstrument, cInstrument);
}

static void onSampleCancelled(void) {
  screenSetup(&screenInstrument, cInstrument);
}

static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  if (row == 3) return 2; // Load button + Lift button
  // Rows 4-6: col 0 = sample params, col 1 = tone
  if (row >= 4 && row <= 6) return 2;
  // Row 7: col 0 = loop start only
  if (row == 7) return 1;
  // Rows 8-9: col 0 = sample pitch/fine, col 1 = noise
  if (row == 8 || row == 9) return 2;
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  const ColorScheme cs = appSettings.colorScheme;
  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;

  // Sample label (y 6) - use title color like "Tone" and "Noise"
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 6, "Sample");

  // Sample filename under "Sample" label (y 7) - dimmer color, not editable
  gfxSetFgColor(cs.textInfo);
  if (strlen(smp->sampleName) > 0) {
    gfxPrint(COL_LEFT_X, 7, smp->sampleName);
  }

  // Sample params labels (y 8-13)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 8, "Rate");
  gfxPrint(COL_LEFT_X, 9, "Start");
  gfxPrint(COL_LEFT_X, 10, "Length");
  gfxPrint(COL_LEFT_X, 11, "LoopSt");
  gfxPrint(COL_LEFT_X, 12, "Pitch");
  gfxPrint(COL_LEFT_X, 13, "Fine");

  // Tone header (y 8)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_RIGHT_X, 8, "Tone");

  // Tone labels (y 9-10)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_RIGHT_X, 9, "Pitch");
  gfxPrint(COL_RIGHT_X, 10, "Fine");

  // Noise header and on/off label (y 12)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_RIGHT_X, 12, "Noise");

  // Noise period label (y 13)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_RIGHT_X, 13, "Period");

  // Update and draw sample preview
  updateSamplePreview();
  if (samplePreviewBitmap) {
    gfxSetFgColor(cs.textInfo);
    gfxDrawBitmap(samplePreviewBitmap, PREVIEW_COL, PREVIEW_ROW);
  }
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  int y = rowToY(row);

  if (row == 3) {
    if (col == 0) {
      // Load button
      gfxCursor(COL_LEFT_VAL, y, 4);
    } else if (col == 1) {
      // Lift button
      gfxCursor(COL_LEFT_VAL + 6, y, 4);
    }
    return;
  }

  if (col == 0) {
    switch (row) {
      case 4: gfxCursor(COL_LEFT_VAL, y, 5); break;  // Rate (decimal, up to 5 digits)
      case 5: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Start
      case 6: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Length
      case 7: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Loop Start
      case 8: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Pitch (hex)
      case 9: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Fine (hex)
    }
  } else if (col == 1) {
    switch (row) {
      case 4: gfxCursor(COL_RIGHT_VAL, y, 3); break; // Tone on/off
      case 5: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Tone pitch (hex)
      case 6: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Tone fine (hex)
      case 8: gfxCursor(COL_RIGHT_VAL, y, 3); break; // Noise on/off
      case 9: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Noise period
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;
  int y = rowToY(row);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 3) {
    if (col == 0) {
      // Load button
      gfxPrint(COL_LEFT_VAL, y, "Load");
    } else if (col == 1) {
      // Lift button
      gfxPrint(COL_LEFT_VAL + 6, y, "Lift");
    }
    return;
  }

  if (col == 0) {
    switch (row) {
      case 4: // Rate
        gfxClearRect(COL_LEFT_VAL, y, 5, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%d", smp->sampleRate);
        break;
      case 5: // Start
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleStart);
        break;
      case 6: // Length
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleLength);
        break;
      case 7: // Loop Start
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleLoopStart);
        break;
      case 8: // Pitch
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)smp->pitchOffset));
        break;
      case 9: // Fine
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)smp->fineTune));
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 4: // Tone on/off
        gfxPrintf(COL_RIGHT_VAL, y, smp->oscTone.isOn ? "On " : "Off");
        break;
      case 5: // Tone pitch
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)smp->oscTone.pitchOffset));
        break;
      case 6: // Tone fine
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)smp->oscTone.fineTune));
        break;
      case 8: // Noise on/off
        gfxPrintf(COL_RIGHT_VAL, y, smp->oscNoise.isOn ? "On " : "Off");
        break;
      case 9: // Noise period
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex(smp->oscNoise.noisePeriod));
        break;
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;
  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;

  if (row == 3) {
    if (col == 0) {
      // Load sample
      fileBrowserSetup("LOAD SAMPLE", ".wav", appSettings.samplePath, onSampleLoaded, onSampleCancelled);
      screenSetup(&screenFileBrowser, 0);
    } else if (col == 1) {
      // Lift sample to zero
      if (smp->sampleData && smp->sampleLength > 0) {
        sampleLiftToZero(smp->sampleData, smp->fileLength, smp->sampleRate);
        projectModified = 1;
        screenMessage(MESSAGE_TIME, "Sample lifted to zero");

        // Update preview to show the modified sample
        updateSamplePreview();
        if (samplePreviewBitmap) {
          gfxSetFgColor(appSettings.colorScheme.textInfo);
          gfxDrawBitmap(samplePreviewBitmap, PREVIEW_COL, PREVIEW_ROW);
        }
      } else {
        screenMessage(MESSAGE_TIME, "No sample loaded");
      }
    }
    return 0;
  }

  if (col == 0) {
    switch (row) {
      case 4: // Rate
        if (action == editClear) {
          smp->sampleRate = 8000;  // Reasonable default rate
          handled = 1;
        } else {
          handled = edit16withMinMax(action, &smp->sampleRate, 100, 500, 44100);
        }
        break;
      case 5: // Start
        {
          if (action == editClear) {
            smp->sampleStart = 0;  // Start from beginning
            handled = 1;
          } else {
            uint16_t oldStart = smp->sampleStart;
            uint16_t oldLoopStart = smp->sampleLoopStart;
            handled = edit16withMinMax(action, &smp->sampleStart, 256, 0, smp->sampleLength > 0 ? smp->sampleLength - 1 : 0);

            // If start increased beyond loop start, adjust loop start
            if (handled && smp->sampleStart > oldStart) {
              if (smp->sampleLoopStart < smp->sampleStart) {
                smp->sampleLoopStart = smp->sampleStart;
              }

              // Redraw loop start if it changed
              if (smp->sampleLoopStart != oldLoopStart) {
                drawField(0, 7, 0);  // Loop Start
              }
            }
          }
        }
        break;
      case 6: // Length
        {
          if (action == editClear) {
            smp->sampleLength = smp->fileLength;  // Use full sample
            handled = 1;
          } else {
            uint16_t oldLength = smp->sampleLength;
            uint16_t oldStart = smp->sampleStart;
            uint16_t oldLoopStart = smp->sampleLoopStart;
            // Length can't exceed fileLength
            uint16_t maxLength = smp->fileLength;
            handled = edit16withMinMax(action, &smp->sampleLength, 256, 1, maxLength);

            // If length decreased, clamp dependent values
            if (handled && smp->sampleLength < oldLength) {
              if (smp->sampleStart >= smp->sampleLength) {
                smp->sampleStart = smp->sampleLength > 0 ? smp->sampleLength - 1 : 0;
              }
              if (smp->sampleLoopStart > smp->sampleLength) {
                smp->sampleLoopStart = smp->sampleLength;
              }

              // Redraw affected fields
              if (smp->sampleStart != oldStart) {
                drawField(0, 5, 0);  // Start
              }
              if (smp->sampleLoopStart != oldLoopStart) {
                drawField(0, 7, 0);  // Loop Start
              }
            }
          }
        }
        break;
      case 7: // Loop Start
        {
          if (action == editClear) {
            // Set to end of sample (no loop)
            smp->sampleLoopStart = smp->sampleLength;
            handled = 1;
          } else {
            // Loop start can be up to length (inclusive) for "no loop"
            uint16_t maxLoopStart = smp->sampleLength;
            handled = edit16withMinMax(action, &smp->sampleLoopStart, 256, smp->sampleStart, maxLoopStart);
          }
          // Update loop preview if it's active
          if (handled && isLoopPreview) {
            if (smp->sampleData && smp->sampleLength > 0) {
              if (!samplePreviewBitmap) {
                samplePreviewBitmap = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, PREVIEW_HEIGHT_CHARS);
              }
              renderSamplePreview(samplePreviewBitmap, smp->sampleData, smp->sampleLoopStart, smp->sampleLength);
              gfxSetFgColor(appSettings.colorScheme.textInfo);
              gfxDrawBitmap(samplePreviewBitmap, PREVIEW_COL, PREVIEW_ROW);
            }
          }
        }
        break;
      case 8: // Pitch
        handled = editSigned8(action, &smp->pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Sample pitch offset %hhd", smp->pitchOffset);
        }
        break;
      case 9: // Fine
        handled = editSigned8(action, &smp->fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Sample fine tune %hhd", smp->fineTune);
        }
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 4: // Tone on/off
        handled = edit8noLast(action, &smp->oscTone.isOn, 1, 0, 1);
        break;
      case 5: // Tone pitch
        handled = editSigned8(action, &smp->oscTone.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Tone pitch offset %hhd", smp->oscTone.pitchOffset);
        }
        break;
      case 6: // Tone fine
        handled = editSigned8(action, &smp->oscTone.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Tone fine tune %hhd", smp->oscTone.fineTune);
        }
        break;
      case 8: // Noise on/off
        handled = edit8noLast(action, &smp->oscNoise.isOn, 1, 0, 1);
        break;
      case 9: // Noise period
        handled = edit8noLast(action, &smp->oscNoise.noisePeriod, 8, 0, 31);
        break;
    }
  }

  if (handled) {
    projectModified = 1;

    // Update preview when start or length changes
    if (col == 0 && (row == 5 || row == 6)) {
      updateSamplePreview();
      if (samplePreviewBitmap) {
        gfxSetFgColor(appSettings.colorScheme.textInfo);
        gfxDrawBitmap(samplePreviewBitmap, PREVIEW_COL, PREVIEW_ROW);
      }
    }
  }

  return handled;
}

static int isCellValid(int col, int row) {
  // Row 7 only has left column (loop start)
  if (row == 7 && col == 1) return 0;
  return 1;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  // Handle loop preview: show loop range while EDIT is held on loop fields
  if (keys == 0) {
    // All keys released - restore normal preview if loop preview was active
    if (isLoopPreview) {
      isLoopPreview = 0;
      updateSamplePreview();
      if (samplePreviewBitmap) {
        gfxSetFgColor(appSettings.colorScheme.textInfo);
        gfxDrawBitmap(samplePreviewBitmap, PREVIEW_COL, PREVIEW_ROW);
      }
    }
  } else if (keys & keyEdit) {
    // EDIT is held - check if we're on loop start field
    if (screenInstrumentAYSample.cursorCol == 0 &&
        screenInstrumentAYSample.cursorRow == 7) {
      InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;

      // Show loop preview if not already showing, we have valid loop data, and loop is not at the end
      if (!isLoopPreview && smp->sampleData && smp->sampleLength > 0 && smp->sampleLoopStart < smp->sampleLength) {
        if (!samplePreviewBitmap) {
          samplePreviewBitmap = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, PREVIEW_HEIGHT_CHARS);
        }
        renderSamplePreview(samplePreviewBitmap, smp->sampleData,
                          smp->sampleLoopStart, smp->sampleLength);
        gfxSetFgColor(appSettings.colorScheme.textInfo);
        gfxDrawBitmap(samplePreviewBitmap, PREVIEW_COL, PREVIEW_ROW);
        isLoopPreview = 1;
      }
    }
  }

  return 0;  // Don't block standard input processing
}

ScreenData screenInstrumentAYSample = {
  .rows = ROW_TOTAL,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
  .onInput = onInput,
  .isCellValid = isCellValid,
};
