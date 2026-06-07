#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "waveform_display.h"
#include <string.h>

// Wavetable preview dimensions (in characters)
#define PREVIEW_WIDTH_CHARS 32
#define PREVIEW_HEIGHT_CHARS 3

// Screen layout:
// y 0:  WAVETABLES
// y 1:  (empty)
// y 2:  Load Save          <- Row 0 (buttons)
// y 3:  (empty)
// y 4:     wavetable -2 (preview)
// y 5:  LL wavetable -2 (preview)
// y 6:     wavetable -1 (preview)
// y 7:  MM wavetable -1 (preview)
// y 8:     Current wavetable preview row 1
// y 9:     Current wavetable preview row 2
// y 10:    Current wavetable preview row 3
// y 11: NN 0123456789ABCDEF0123456789ABCDEF  <- Row 1 (editor - 32 columns)
// y 12:    wavetable +1 (preview)
// y 13: OO wavetable +1 (preview)
// y 14:    wavetable +2 (preview)
// y 15: PP wavetable +2 (preview)
// y 16:    wavetable +3 (preview)
// y 17: QQ wavetable +3 (preview)
//
// Logical rows:
// Row 0: Buttons (Load, Save) - 2 columns
// Row 1: Wavetable editor - 32 columns (starts at x=2, directly after the 2-char index)

static int wavetableIdx = 0;
static Bitmap* previewBitmap = NULL;  // Bitmap for current wavetable preview
static Bitmap* previewBitmapPrev1 = NULL;  // Preview for wavetable -1
static Bitmap* previewBitmapPrev2 = NULL;  // Preview for wavetable -2
static Bitmap* previewBitmapNext1 = NULL;  // Preview for wavetable +1
static Bitmap* previewBitmapNext2 = NULL;  // Preview for wavetable +2
static Bitmap* previewBitmapNext3 = NULL;  // Preview for wavetable +3

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static void drawSelection(int col1, int row1, int col2, int row2);
static int onEdit(int col, int row, enum CellEditAction action);
static int inputScreenNavigation(int keys, int tapCount);
static enum ScreenPlaybackLevel getPlaybackLevel(void);

static ScreenData screen = {
  .rows = 2,  // Row 0: buttons, Row 1: wavetable editor
  .cursorRow = 1,  // Start on the editor row
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = -1,  // Selection disabled
  .selectStartRow = 0,
  .selectStartCol = 0,
  .selectAnchorRow = 0,
  .selectAnchorCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawSelection = drawSelection,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

static void init(void) {
  screen.cursorRow = 1;  // Start on the editor row
  screen.cursorCol = 0;
  screen.topRow = 0;
  wavetableIdx = 0;

  // Create preview bitmaps
  if (!previewBitmap) previewBitmap = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, PREVIEW_HEIGHT_CHARS);
  if (!previewBitmapPrev1) previewBitmapPrev1 = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, 1);
  if (!previewBitmapPrev2) previewBitmapPrev2 = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, 1);
  if (!previewBitmapNext1) previewBitmapNext1 = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, 1);
  if (!previewBitmapNext2) previewBitmapNext2 = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, 1);
  if (!previewBitmapNext3) previewBitmapNext3 = gfxBitmapCreate(PREVIEW_WIDTH_CHARS, 1);
}

static void updatePreviews(void) {
  Project* p = &chipnomadState->project;
  int isYM = p->chipSetup.ay.isYM;

  // Update current wavetable preview (3 lines tall)
  if (previewBitmap) {
    renderWavetablePreview(previewBitmap, p->ayWavetables[wavetableIdx], isYM);
  }

  // Update adjacent wavetable previews (1 line tall each)
  if (wavetableIdx >= 2 && previewBitmapPrev2) {
    renderWavetablePreview(previewBitmapPrev2, p->ayWavetables[wavetableIdx - 2], isYM);
  }
  if (wavetableIdx >= 1 && previewBitmapPrev1) {
    renderWavetablePreview(previewBitmapPrev1, p->ayWavetables[wavetableIdx - 1], isYM);
  }
  if (wavetableIdx <= 254 && previewBitmapNext1) {
    renderWavetablePreview(previewBitmapNext1, p->ayWavetables[wavetableIdx + 1], isYM);
  }
  if (wavetableIdx <= 253 && previewBitmapNext2) {
    renderWavetablePreview(previewBitmapNext2, p->ayWavetables[wavetableIdx + 2], isYM);
  }
  if (wavetableIdx <= 252 && previewBitmapNext3) {
    renderWavetablePreview(previewBitmapNext3, p->ayWavetables[wavetableIdx + 3], isYM);
  }
}

static void setup(int input) {
  wavetableIdx = input & 0xff;
  screen.selectMode = -1;
  screen.cursorRow = 1;  // Always start on the editor row when switching wavetables
}

static void fullRedraw(void) {
  drawStatic();
  screenFullRedraw(&screen);
}

static void draw(void) {
  screenDrawOverlays(&screen);
}

static int getColumnCount(int row) {
  if (row == 0) return 2;   // Buttons: Load, Save
  if (row == 1) return 32;  // Wavetable editor: 32 steps
  return 1;
}

static void drawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  // Title
  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "WAVETABLES");

  // Button labels (row 0 is at y=2)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(0, 2, "Load");
  gfxPrint(5, 2, "Save");

  // Update all previews
  updatePreviews();

  // Clear all preview areas first (35 chars wide: 2 for index + 32 for data + 1 for safety)
  gfxSetBgColor(cs.background);
  gfxClearRect(0, 4, 35, 2);  // wavetable -2
  gfxClearRect(0, 6, 35, 2);  // wavetable -1
  gfxClearRect(0, 8, 35, 3);  // current wavetable preview (3 lines)
  gfxClearRect(0, 12, 35, 2); // wavetable +1
  gfxClearRect(0, 14, 35, 2); // wavetable +2
  gfxClearRect(0, 16, 35, 2); // wavetable +3

  // Wavetable numbers on the left (2 characters wide) - using textInfo color
  gfxSetFgColor(cs.textInfo);

  // Calculate wavetable indices (no wrapping - stop at boundaries)
  int prevIdx2 = wavetableIdx - 2;
  int prevIdx1 = wavetableIdx - 1;
  int nextIdx1 = wavetableIdx + 1;
  int nextIdx2 = wavetableIdx + 2;
  int nextIdx3 = wavetableIdx + 3;

  // Draw wavetable indices only if they're valid (within 0-255)
  if (prevIdx2 >= 0) {
    gfxPrintf(0, 5, "%02X", prevIdx2);
  }
  if (prevIdx1 >= 0) {
    gfxPrintf(0, 7, "%02X", prevIdx1);
  }
  // Current wavetable index is always shown
  gfxPrintf(0, 11, "%02X", wavetableIdx);

  if (nextIdx1 <= 255) {
    gfxPrintf(0, 13, "%02X", nextIdx1);
  }
  if (nextIdx2 <= 255) {
    gfxPrintf(0, 15, "%02X", nextIdx2);
  }
  if (nextIdx3 <= 255) {
    gfxPrintf(0, 17, "%02X", nextIdx3);
  }

  // Draw wavetable previews
  gfxSetFgColor(cs.textInfo);

  if (prevIdx2 >= 0 && previewBitmapPrev2) {
    gfxDrawBitmap(previewBitmapPrev2, 2, 4);
  }
  if (prevIdx1 >= 0 && previewBitmapPrev1) {
    gfxDrawBitmap(previewBitmapPrev1, 2, 6);
  }
  // Current wavetable preview (3 lines tall)
  if (previewBitmap) {
    gfxDrawBitmap(previewBitmap, 2, 8);
  }
  if (nextIdx1 <= 255 && previewBitmapNext1) {
    gfxDrawBitmap(previewBitmapNext1, 2, 12);
  }
  if (nextIdx2 <= 255 && previewBitmapNext2) {
    gfxDrawBitmap(previewBitmapNext2, 2, 14);
  }
  if (nextIdx3 <= 255 && previewBitmapNext3) {
    gfxDrawBitmap(previewBitmapNext3, 2, 16);
  }
}

static void drawRowHeader(int row, int state) {
  // No row headers needed for this screen
  (void)row;
  (void)state;
}

static void drawColHeader(int col, int state) {
  // No column headers needed for this screen
  (void)col;
  (void)state;
}

static void drawSelection(int col1, int row1, int col2, int row2) {
  // Selection is disabled for this screen (selectMode = -1)
  (void)col1;
  (void)row1;
  (void)col2;
  (void)row2;
}

static void drawCursor(int col, int row) {
  if (row == 0) {
    // Buttons row at y=2
    int x = (col == 0) ? 0 : 5;
    gfxCursor(x, 2, 4);
  } else if (row == 1) {
    // Wavetable editor row at y=11, starts at x=2 (directly after 2-char index)
    gfxCursor(2 + col, 11, 1);
  }
}

static void drawField(int col, int row, int state) {
  const ColorScheme cs = appSettings.colorScheme;

  if (row == 0) {
    // Buttons row - need to redraw to clear cursor
    gfxSetFgColor(state == stateFocus ? cs.textValue : cs.textDefault);
    if (col == 0) {
      gfxClearRect(0, 2, 4, 1);  // Clear area for "Load" button
      gfxPrint(0, 2, "Load");
    } else if (col == 1) {
      gfxClearRect(5, 2, 4, 1);  // Clear area for "Save" button
      gfxPrint(5, 2, "Save");
    }
  } else if (row == 1) {
    // Wavetable editor row at y=11, starts at x=2
    uint8_t* wavetable = chipnomadState->project.ayWavetables[wavetableIdx];
    uint8_t value = wavetable[col];

    gfxSetFgColor(state == stateFocus ? cs.textValue : cs.textDefault);

    // Each wavetable value is 4-bit (0-15), display as single hex digit
    char hex[2];
    hex[0] = "0123456789ABCDEF"[value & 0x0F];
    hex[1] = '\0';
    gfxPrint(2 + col, 11, hex);
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row == 0) {
    // Buttons row
    if (action == editTap) {
      if (col == 0) {
        // Load button - TODO: implement load functionality
        screenMessage(MESSAGE_TIME, "Load not implemented yet");
        return 1;
      } else if (col == 1) {
        // Save button - TODO: implement save functionality
        screenMessage(MESSAGE_TIME, "Save not implemented yet");
        return 1;
      }
    }
    return 0;
  } else if (row == 1) {
    // Wavetable editor row
    uint8_t* wavetable = chipnomadState->project.ayWavetables[wavetableIdx];

    int handled = edit8noLast(action, &wavetable[col], 8, 0, 15);
    if (handled) {
      projectModified = 1;

      // Update preview immediately after edit
      Project* p = &chipnomadState->project;
      int isYM = p->chipSetup.ay.isYM;
      if (previewBitmap) {
        renderWavetablePreview(previewBitmap, p->ayWavetables[wavetableIdx], isYM);
        gfxSetFgColor(appSettings.colorScheme.textInfo);
        gfxDrawBitmap(previewBitmap, 2, 8);
      }
    }
    return handled;
  }

  return 0;
}

static int inputScreenNavigation(int keys, int tapCount) {
  if (keys == (keyUp | keyShift)) {
    // Go back to Table screen
    screenSetup(&screenTable, 0);
    return 1;
  } else if (keys == (keyUp | keyOpt)) {
    // Previous wavetable (by 1) - stop at 0
    if (wavetableIdx > 0) {
      wavetableIdx--;
      setup(wavetableIdx);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyDown | keyOpt)) {
    // Next wavetable (by 1) - stop at 255
    if (wavetableIdx < 255) {
      wavetableIdx++;
      setup(wavetableIdx);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyLeft | keyOpt)) {
    // Previous wavetable (by 16) - stop at 0
    if (wavetableIdx >= 16) {
      wavetableIdx -= 16;
    } else {
      wavetableIdx = 0;
    }
    setup(wavetableIdx);
    fullRedraw();
    return 1;
  } else if (keys == (keyRight | keyOpt)) {
    // Next wavetable (by 16) - stop at 255
    if (wavetableIdx <= 239) {
      wavetableIdx += 16;
    } else {
      wavetableIdx = 255;
    }
    setup(wavetableIdx);
    fullRedraw();
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (!isKeyDown) return 0;

  // Handle screen navigation first
  if (inputScreenNavigation(keys, tapCount)) {
    return 1;
  }

  // Handle spreadsheet input
  return screenInput(&screen, isKeyDown, keys, tapCount);
}

static enum ScreenPlaybackLevel getPlaybackLevel(void) {
  return screenPlaybackPhrase;
}

const AppScreen screenWavetable = {
  .init = init,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .getPlaybackLevel = getPlaybackLevel,
};
