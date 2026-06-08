#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "waveform_display.h"
#include "utils.h"
#include "misc.h"

// Wavetable preview bitmap for P1 field (2 chars wide, 1 char high)
static Bitmap* wavetablePreviewBitmap = NULL;

// Screen layout:
// y 6:  Tone   [On ]       Envelope [Off   ]
// y 7:  Pitch  [+0  ]      Mode     [Osc   ]
// y 8:  Fine   [+0  ]      Pitch/N  [+0  ]  (depends on mode)
// y 9:                     Fine/D   [+0  ]  (depends on mode)
// y 10: (spacing)
// y 11: Noise  [Off]       Soft osc [Off  ]
// y 12: Period [00 ]       Pitch    [+0  ]
// y 13:                    Fine     [+0  ]
// y 14:                    FM depth [00  ]
// y 15:                    P1       [00  ]  (depends on type)
// y 16:                    P2       [00  ]  (depends on type)
//
// Logical rows:
// 0-2: common (type, name, transpose/tic)
// 3: Tone on/off | Env shape        (y 6)
// 4: Tone pitch  | Env mode         (y 7)
// 5: Tone fine   | Env pitch/N      (y 8)
// 6:             | Env fine/D       (y 9)
// 7: Noise on/off | Soft osc type   (y 11)
// 8: Noise period | Soft osc pitch  (y 12)
// 9: (dead left)  | Soft osc fine   (y 13)
// 10: (dead left) | Soft osc FM depth (y 14)
// 11: (dead left) | Soft osc P1     (y 15)
// 12: (dead left) | Soft osc P2     (y 16)

#define COL_LEFT_X    0
#define COL_LEFT_VAL  8
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 26

#define ROW_TOTAL 13

static int rowToY(int row) {
  switch (row) {
    case 3: return 6;
    case 4: return 7;
    case 5: return 8;
    case 6: return 9;
    case 7: return 11;
    case 8: return 12;
    case 9: return 13;
    case 10: return 14;
    case 11: return 15;
    case 12: return 16;
    default: return 0;
  }
}

static const char* softwareOscTypeName(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscNone:           return "Off     ";
    case aySoftwareOscPulse:          return "Pulse   ";
    case aySoftwareOscSyncTone:       return "SyncTone";
    case aySoftwareOscSyncEnvelope:   return "SyncEnv ";
    case aySoftwareOscWavetable:      return "Wavetbl ";
    case aySoftwareOscToneFM:         return "ToneFM  ";
    case aySoftwareOscEnvFM:          return "EnvFM   ";
    default:                          return "?       ";
  }
}

static const char* softwareOscP1Name(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscPulse:          return "Width  ";
    case aySoftwareOscWavetable:      return "WaveIdx";
    default:                          return "P1     ";
  }
}

static const char* softwareOscP2Name(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscPulse:          return "PulseLow";
    default:                          return "P2      ";
  }
}

static int softwareOscHasP1(enum AYSoftwareOscType type) {
  return type == aySoftwareOscPulse ||
         type == aySoftwareOscWavetable;
}

static int softwareOscHasP2(enum AYSoftwareOscType type) {
  return type == aySoftwareOscPulse;
}

static uint8_t* softwareOscP1Ptr(InstrumentAYOscSoftware* osc) {
  switch (osc->type) {
    case aySoftwareOscPulse:          return &osc->pulseWidth;
    case aySoftwareOscWavetable:      return &osc->wavetableIndex;
    default:                          return NULL;
  }
}

static uint8_t* softwareOscP2Ptr(InstrumentAYOscSoftware* osc) {
  switch (osc->type) {
    case aySoftwareOscPulse:          return &osc->pulseLow;
    default:                          return NULL;
  }
}

static uint8_t softwareOscP1Max(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscPulse:          return 255;  // Pulse width 0-255
    case aySoftwareOscWavetable:      return 255; // Wavetable index
    default:                          return 255;
  }
}

static uint8_t softwareOscP2Max(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscPulse:          return 15;  // Low level 0-15
    default:                          return 255;
  }
}

static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  if (row >= 3 && row <= 12) return 2;
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  // Create wavetable preview bitmap if needed
  if (!wavetablePreviewBitmap) {
    wavetablePreviewBitmap = gfxBitmapCreate(2, 1);
  }

  const ColorScheme cs = appSettings.colorScheme;
  InstrumentAY2* ay2 = &chipnomadState->project.instruments[cInstrument].chip.ay2;

  // Top block headers (y 6)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 6, "Tone");
  gfxPrint(COL_RIGHT_X, 6, "Envelope");

  // Top block labels (y 7-9)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 7, "Pitch");
  gfxPrint(COL_LEFT_X, 8, "Fine");
  gfxPrint(COL_RIGHT_X, 7, "Mode");
  // Row 8-9 labels are dynamic (Pitch/N and Fine/D or N and D)

  // Bottom block headers (y 11)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 11, "Noise");
  gfxPrint(COL_RIGHT_X, 11, "Soft osc");

  // Bottom block labels (y 12-14)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 12, "Period");
  gfxPrint(COL_RIGHT_X, 12, "Pitch");
  gfxPrint(COL_RIGHT_X, 13, "Fine");
  gfxPrint(COL_RIGHT_X, 14, "FM depth");

  // P1/P2 labels (y 15-16) - conditional based on osc type
  // Clear the areas first to remove any stale labels
  gfxClearRect(COL_RIGHT_X, 15, 14, 1);
  gfxClearRect(COL_RIGHT_X, 16, 14, 1);

  if (softwareOscHasP1(ay2->oscSoftware.type)) {
    gfxPrint(COL_RIGHT_X, 15, softwareOscP1Name(ay2->oscSoftware.type));
  }
  if (softwareOscHasP2(ay2->oscSoftware.type)) {
    gfxPrint(COL_RIGHT_X, 16, softwareOscP2Name(ay2->oscSoftware.type));
  }
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  int y = rowToY(row);

  if (col == 0) {
    switch (row) {
      case 3: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Tone on/off
      case 4: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Tone pitch (hex)
      case 5: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Tone fine (hex)
      case 7: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Noise on/off
      case 8: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Noise period
    }
  } else if (col == 1) {
    switch (row) {
      case 3: gfxCursor(COL_RIGHT_VAL, y, 1); break;  // Env shape (single hex digit)
      case 4: gfxCursor(COL_RIGHT_VAL, y, 6); break;  // Env mode
      case 5: // Env pitch/N
        gfxCursor(COL_RIGHT_VAL, y, 2);  // Always 2 chars for hex
        break;
      case 6: // Env fine/D
        gfxCursor(COL_RIGHT_VAL, y, 2);  // Always 2 chars for hex
        break;
      case 7: gfxCursor(COL_RIGHT_VAL, y, 8); break;  // Soft osc type
      case 8: gfxCursor(COL_RIGHT_VAL, y, 2); break;  // Soft osc pitch (hex)
      case 9: gfxCursor(COL_RIGHT_VAL, y, 2); break;  // Soft osc fine (hex)
      case 10: gfxCursor(COL_RIGHT_VAL, y, 2); break; // FM depth (hex)
      case 11: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Soft osc P1 (hex)
      case 12: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Soft osc P2 (hex)
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  InstrumentAY2* ay2 = &chipnomadState->project.instruments[cInstrument].chip.ay2;
  int y = rowToY(row);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (col == 0) {
    switch (row) {
      case 3: // Tone on/off
        gfxPrintf(COL_LEFT_VAL, y, ay2->oscTone.isOn ? "On " : "Off");
        break;
      case 4: // Tone pitch
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)ay2->oscTone.pitchOffset));
        break;
      case 5: // Tone fine
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)ay2->oscTone.fineTune));
        break;
      case 7: // Noise on/off
        gfxPrintf(COL_LEFT_VAL, y, ay2->oscNoise.isOn ? "On " : "Off");
        break;
      case 8: // Noise period
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex(ay2->oscNoise.noisePeriod));
        break;
    }
  } else if (col == 1) {
    int isAutoEnv = ay2->oscEnvelope.autoEnvN != 0;

    switch (row) {
      case 3: // Envelope shape
        gfxClearRect(COL_RIGHT_VAL, y, 6, 1);
        if (ay2->oscEnvelope.shape == 0) {
          gfxPrint(COL_RIGHT_VAL, y, "0 Off ");
        } else {
          gfxPrintf(COL_RIGHT_VAL, y, "%X %s", ay2->oscEnvelope.shape, getEnvelopeShapeASCII(ay2->oscEnvelope.shape));
        }
        break;
      case 4: // Envelope mode
        gfxClearRect(COL_RIGHT_VAL, y, 7, 1);
        gfxPrint(COL_RIGHT_VAL, y, isAutoEnv ? "AutoEnv" : "Osc   ");
        break;
      case 5: // Envelope pitch/N
        // Draw the label for this row
        gfxSetFgColor(appSettings.colorScheme.textDefault);
        gfxPrint(COL_RIGHT_X, y, isAutoEnv ? "N    " : "Pitch");
        // Draw the value
        gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        if (isAutoEnv) {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex(ay2->oscEnvelope.autoEnvN));
        } else {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscEnvelope.pitchOffset));
        }
        break;
      case 6: // Envelope fine/D
        // Draw the label for this row
        gfxSetFgColor(appSettings.colorScheme.textDefault);
        gfxPrint(COL_RIGHT_X, y, isAutoEnv ? "D    " : "Fine ");
        // Draw the value
        gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        if (isAutoEnv) {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex(ay2->oscEnvelope.autoEnvD));
        } else {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscEnvelope.fineTune));
        }
        break;
      case 7: // Software osc type
        gfxPrint(COL_RIGHT_VAL, y, softwareOscTypeName(ay2->oscSoftware.type));
        break;
      case 8: // Software osc pitch
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscSoftware.pitchOffset));
        break;
      case 9: // Software osc fine
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscSoftware.fineTune));
        break;
      case 10: // FM depth (visible for all soft osc types)
        // Draw the value
        gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex(ay2->oscSoftware.fmDepth));
        break;
      case 11: // Software osc P1
        if (softwareOscHasP1(ay2->oscSoftware.type)) {
          // Draw the label for this row
          gfxSetFgColor(appSettings.colorScheme.textDefault);
          gfxPrint(COL_RIGHT_X, y, softwareOscP1Name(ay2->oscSoftware.type));
          // Draw the value
          gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
          gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
          uint8_t* p1 = softwareOscP1Ptr(&ay2->oscSoftware);
          if (p1) gfxPrint(COL_RIGHT_VAL, y, byteToHex(*p1));

          // Draw wavetable preview for WavTb type
          if (ay2->oscSoftware.type == aySoftwareOscWavetable) {
            if (p1 && wavetablePreviewBitmap) {
              // Render preview
              Project* p = &chipnomadState->project;
              int isYM = p->chipSetup.ay.isYM;
              renderWavetablePreview(wavetablePreviewBitmap, p->ayWavetables[*p1], isYM);

              // Draw preview after the value (at x=29, 2 chars after value at x=26-27)
              gfxSetFgColor(appSettings.colorScheme.textInfo);
              gfxDrawBitmap(wavetablePreviewBitmap, 29, y);
            }
          } else {
            // Clear preview area for non-wavetable types
            gfxClearRect(29, y, 2, 1);
          }
        } else {
          // Clear entire P1 area when not available
          gfxClearRect(COL_RIGHT_X, y, 14, 1);
        }
        break;
      case 12: // Software osc P2
        if (softwareOscHasP2(ay2->oscSoftware.type)) {
          // Draw the label for this row
          gfxSetFgColor(appSettings.colorScheme.textDefault);
          gfxPrint(COL_RIGHT_X, y, softwareOscP2Name(ay2->oscSoftware.type));
          // Draw the value
          gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
          gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
          uint8_t* p2 = softwareOscP2Ptr(&ay2->oscSoftware);
          if (p2) gfxPrint(COL_RIGHT_VAL, y, byteToHex(*p2));
        } else {
          // Clear entire P2 area when not available
          gfxClearRect(COL_RIGHT_X, y, 14, 1);
        }
        break;
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;
  InstrumentAY2* ay2 = &chipnomadState->project.instruments[cInstrument].chip.ay2;

  if (col == 0) {
    switch (row) {
      case 3: // Tone on/off
        handled = edit8noLast(action, &ay2->oscTone.isOn, 1, 0, 1);
        break;
      case 4: // Tone pitch
        handled = editSigned8(action, &ay2->oscTone.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Tone pitch offset %hhd", ay2->oscTone.pitchOffset);
        }
        break;
      case 5: // Tone fine
        handled = editSigned8(action, &ay2->oscTone.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Tone fine tune %hhd", ay2->oscTone.fineTune);
        }
        break;
      case 7: // Noise on/off
        handled = edit8noLast(action, &ay2->oscNoise.isOn, 1, 0, 1);
        break;
      case 8: // Noise period
        handled = edit8noLast(action, &ay2->oscNoise.noisePeriod, 8, 0, 31);
        break;
    }
  } else if (col == 1) {
    int isAutoEnv = ay2->oscEnvelope.autoEnvN != 0;

    switch (row) {
      case 3: // Envelope shape
        handled = edit8noLast(action, &ay2->oscEnvelope.shape, 8, 0, 15);
        break;
      case 4: // Envelope mode (toggle between Osc and AutoEnv)
        {
          uint8_t modeValue = isAutoEnv ? 1 : 0;
          uint8_t oldValue = modeValue;
          handled = edit8noLast(action, &modeValue, 1, 0, 1);

          if (handled && oldValue != modeValue) {
            if (modeValue == 1) {
              // Switching to AutoEnv mode
              ay2->oscEnvelope.autoEnvN = 1;
              ay2->oscEnvelope.autoEnvD = 1;
            } else {
              // Switching to Osc mode
              ay2->oscEnvelope.autoEnvN = 0;
              ay2->oscEnvelope.autoEnvD = 0;
            }
            // Redraw the fields (which will also redraw labels)
            drawField(1, 5, 0);
            drawField(1, 6, 0);
          }
        }
        break;
      case 5: // Envelope pitch/N
        if (isAutoEnv) {
          handled = edit8noLast(action, &ay2->oscEnvelope.autoEnvN, 16, 1, 15);
          if (handled) {
            screenMessage(0, "Envelope auto N %hhu", ay2->oscEnvelope.autoEnvN);
          }
        } else {
          handled = editSigned8(action, &ay2->oscEnvelope.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
          if (handled) {
            screenMessage(0, "Envelope pitch offset %hhd", ay2->oscEnvelope.pitchOffset);
          }
        }
        break;
      case 6: // Envelope fine/D
        if (isAutoEnv) {
          handled = edit8noLast(action, &ay2->oscEnvelope.autoEnvD, 16, 1, 15);
          if (handled) {
            screenMessage(0, "Envelope auto D %hhu", ay2->oscEnvelope.autoEnvD);
          }
        } else {
          handled = editSigned8(action, &ay2->oscEnvelope.fineTune, 16, -128, 127);
          if (handled) {
            screenMessage(0, "Envelope fine tune %hhd", ay2->oscEnvelope.fineTune);
          }
        }
        break;
      case 7: // Software osc type
        {
          uint8_t oldType = (uint8_t)ay2->oscSoftware.type;
          uint8_t type = oldType;
          handled = edit8noLast(action, &type, 1, 0, aySoftwareOscSample - 1);
          ay2->oscSoftware.type = (enum AYSoftwareOscType)type;

          // If type changed, clear and redraw rows that depend on type
          if (handled && oldType != type) {
            // Clear P1 and P2 display areas (including labels, values, and wavetable preview)
            int y11 = rowToY(11);
            int y12 = rowToY(12);
            gfxClearRect(COL_RIGHT_X, y11, 14, 1);  // Clear label + value area for P1 (row 11)
            gfxClearRect(29, y11, 2, 1);            // Clear wavetable preview area for P1
            gfxClearRect(COL_RIGHT_X, y12, 14, 1);  // Clear label + value area for P2 (row 12)

            // Redraw all affected rows
            drawField(1, 10, 0);  // FM depth (always visible, might need refresh)
            drawField(1, 11, 0);  // P1 (conditional)
            drawField(1, 12, 0);  // P2 (conditional)
          }
        }
        break;
      case 8: // Software osc pitch
        handled = editSigned8(action, &ay2->oscSoftware.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Soft osc pitch offset %hhd", ay2->oscSoftware.pitchOffset);
        }
        break;
      case 9: // Software osc fine
        handled = editSigned8(action, &ay2->oscSoftware.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Soft osc fine tune %hhd", ay2->oscSoftware.fineTune);
        }
        break;
      case 10: // FM depth (available for all soft osc types)
        handled = edit8noLast(action, &ay2->oscSoftware.fmDepth, 16, 0, 255);
        if (handled) {
          screenMessage(0, "Soft osc FM depth %hhu", ay2->oscSoftware.fmDepth);
        }
        break;
      case 11: // Software osc P1
        if (softwareOscHasP1(ay2->oscSoftware.type)) {
          uint8_t* p1 = softwareOscP1Ptr(&ay2->oscSoftware);
          if (p1) {
            // Special handling for Clear action on pulse width - reset to 0x80 (50% duty cycle)
            if (action == editClear && ay2->oscSoftware.type == aySoftwareOscPulse) {
              *p1 = 0x80;
              handled = 1;
              screenMessage(0, "Pulse width %hhu", *p1);
            } else {
              uint8_t maxVal = softwareOscP1Max(ay2->oscSoftware.type);
              handled = edit8noLast(action, p1, 16, 0, maxVal);
              if (handled) {
                // Type-specific messages
                if (ay2->oscSoftware.type == aySoftwareOscPulse) {
                  screenMessage(0, "Pulse width %hhu", *p1);
                } else if (ay2->oscSoftware.type == aySoftwareOscWavetable) {
                  screenMessage(0, "Wavetable index %hhu", *p1);
                } else {
                  screenMessage(0, "Soft osc P1 %hhu", *p1);
                }

                // Update wavetable preview immediately for WavTb type
                if (ay2->oscSoftware.type == aySoftwareOscWavetable) {
                  if (wavetablePreviewBitmap) {
                    Project* p = &chipnomadState->project;
                    int isYM = p->chipSetup.ay.isYM;
                    renderWavetablePreview(wavetablePreviewBitmap, p->ayWavetables[*p1], isYM);

                    int y = rowToY(11);
                    gfxSetFgColor(appSettings.colorScheme.textInfo);
                    gfxDrawBitmap(wavetablePreviewBitmap, 29, y);
                  }
                }
              }
            }
          }
        }
        break;
      case 12: // Software osc P2
        if (softwareOscHasP2(ay2->oscSoftware.type)) {
          uint8_t* p2 = softwareOscP2Ptr(&ay2->oscSoftware);
          if (p2) {
            uint8_t maxVal = softwareOscP2Max(ay2->oscSoftware.type);
            handled = edit8noLast(action, p2, 16, 0, maxVal);
            if (handled) {
              // Type-specific messages
              if (ay2->oscSoftware.type == aySoftwareOscPulse) {
                screenMessage(0, "Pulse low level %hhu", *p2);
              } else {
                screenMessage(0, "Soft osc P2 %hhu", *p2);
              }
            }
          }
        }
        break;
    }
  }

  if (handled) projectModified = 1;
  return handled;
}

static int isCellValid(int col, int row) {
  // Row 6, 9, 10, 11, 12 col 0 are dead cells (tone/noise blocks have no corresponding rows)
  if ((row == 6 || row == 9 || row == 10 || row == 11 || row == 12) && col == 0) return 0;

  // Row 10 (FM depth) is always valid for col 1
  // Row 11 (P1) and row 12 (P2) col 1 are only valid if the osc type supports them
  if (col == 1) {
    InstrumentAY2* ay2 = &chipnomadState->project.instruments[cInstrument].chip.ay2;
    if (row == 11 && !softwareOscHasP1(ay2->oscSoftware.type)) return 0;
    if (row == 12 && !softwareOscHasP2(ay2->oscSoftware.type)) return 0;
  }

  return 1;
}

ScreenData screenInstrumentAY2 = {
  .rows = ROW_TOTAL,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
  .isCellValid = isCellValid,
};
