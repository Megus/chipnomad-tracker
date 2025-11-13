#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <corelib_file.h>
#include <utils.h>
#include <project.h>
#include <project_utils.h>
#include <screen_instrument.h>
#include <copy_paste.h>
#include <file_browser.h>
#include <string.h>

extern const struct AppScreen screenInstrumentPool;

int cInstrument = 0;
static int isCharEdit = 0;

static void getInstrumentFilename(char* filename, int size) {
  if (strlen(project.instruments[cInstrument].name) > 0) {
    snprintf(filename, size, "%s", project.instruments[cInstrument].name);
  } else {
    snprintf(filename, size, "instrument_%02X", cInstrument);
  }
}

static void onInstrumentLoaded(const char* path) {
  if (instrumentLoad(path, cInstrument) == 0) {
    // Save the directory path
    char* lastSeparator = strrchr(path, PATH_SEPARATOR);
    if (lastSeparator) {
      int pathLen = lastSeparator - path;
      if (pathLen < PATH_LENGTH) {
        strncpy(appSettings.instrumentPath, path, pathLen);
        appSettings.instrumentPath[pathLen] = 0;
      }
    }
  }
  screenSetup(&screenInstrument, cInstrument);
}

static void onInstrumentSaved(const char* folderPath) {
  // The file browser constructs the full path internally, but we need to recreate it
  // since the callback only gives us the folder path
  char fullPath[2048];
  char filename[32];
  getInstrumentFilename(filename, sizeof(filename));
  snprintf(fullPath, sizeof(fullPath), "%s%s%s.cni", folderPath, PATH_SEPARATOR_STR, filename);

  if (instrumentSave(fullPath, cInstrument) == 0) {
    strncpy(appSettings.instrumentPath, folderPath, PATH_LENGTH);
    appSettings.instrumentPath[PATH_LENGTH - 1] = 0;
  }
  screenSetup(&screenInstrument, cInstrument);
}

static void onInstrumentCancelled(void) {
  screenSetup(&screenInstrument, cInstrument);
}

static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);



static struct ScreenData screenInstrumentNone = {
  .rows = 1,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = instrumentCommonColumnCount,
  .drawStatic = instrumentCommonDrawStatic,
  .drawCursor = instrumentCommonDrawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = instrumentCommonDrawField,
  .onEdit = instrumentCommonOnEdit,
};

static struct ScreenData* instrumentScreen(void) {
  struct ScreenData* data = &screenInstrumentNone;
  if (project.instruments[cInstrument].type == instAY) {
    data = &screenInstrumentAY;
  }
  data->drawRowHeader = drawRowHeader;
  data->drawColHeader = drawColHeader;

  return data;
}

static void setup(int input) {
  isCharEdit = 0;
  if (input != -1) {
    cInstrument = input;
  }
}

static void fullRedraw(void) {
  struct ScreenData* screen = instrumentScreen();
  screenFullRedraw(screen);
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Common part of the form
//

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}

int instrumentCommonColumnCount(int row) {
  if (row == 0) {
    return 3; // Instrument type, load, save
  } else if (row == 1) {
    return 15; // Instrument name
  } else if (row == 2) {
    return 2; // Transpose on/off, Table tic speed
  }
  return 1; // Default value
}

void instrumentCommonDrawStatic(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "INSTRUMENT %02X", cInstrument);

  gfxSetFgColor(cs.textDefault);
  gfxPrint(0, 2, "Type");

  if (project.instruments[cInstrument].type == instNone) return;

  gfxPrint(0, 3, "Name");
  gfxPrint(0, 4, "Transp.");
  gfxPrint(17, 4, "Tbl. Tic");
}

void instrumentCommonDrawCursor(int col, int row) {
  if (row == 0 && col == 0) {
    // Type
    gfxCursor(8, 2, strlen(instrumentTypeName(project.instruments[cInstrument].type)));
  } else if (row == 0 && col == 1) {
    // Load
    gfxCursor(17, 2, 4);
  } else if (row == 0 && col == 2) {
    // Save
    gfxCursor(23, 2, 4);
  } else if (row == 1) {
    // Instrument name
    gfxCursor(8 + col, 3, 1);
  } else if (row == 2 && col == 0) {
    // Transpose on/off
    gfxCursor(8, 4, 3);
  } else if (row == 2 && col == 1) {
    // Table tic speed
    gfxCursor(26, 4, 2);
  }
}

void instrumentCommonDrawField(int col, int row, int state) {
  gfxSetFgColor(appSettings.colorScheme.textDefault);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 0 && col == 0) {
    // Instrument type
    gfxClearRect(8, 2, 8, 1);
    gfxPrint(8, 2, instrumentTypeName(project.instruments[cInstrument].type));
  } else if (row == 0 && col == 1) {
    // Load
    gfxPrint(17, 2, "Load");
  } else if (row == 0 && col == 2) {
    // Save
    gfxPrint(23, 2, "Save");
  } else if (row == 1) {
    // Instrument name
    gfxClearRect(8, 3, PROJECT_INSTRUMENT_NAME_LENGTH, 1);
    gfxPrintf(8, 3, "%s", project.instruments[cInstrument].name);
  } else if (row == 2 && col == 0) {
    // Transpose
    gfxPrintf(8, 4, project.instruments[cInstrument].transposeEnabled ? "On " : "Off");
  } else if (row == 2 && col == 1) {
    // Table tic speed
    gfxPrint(26, 4, byteToHex(project.instruments[cInstrument].tableSpeed));
  }
}

int instrumentCommonOnEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;
  if (row == 0 && col == 0) {
    // Instrument type
    uint8_t oldType = project.instruments[cInstrument].type;
    handled = edit8noLast(action, &project.instruments[cInstrument].type, 1, 0, 1);
    if (oldType != project.instruments[cInstrument].type) {
      switch (project.instruments[cInstrument].type) {
        case instNone:
          break;
        case instAY:
          initAYInstrument(cInstrument);
          break;
      }
      fullRedraw();
    }
  } else if (row == 0 && col == 1) {
    // Load instrument
    fileBrowserSetup("LOAD INSTRUMENT", ".cni", appSettings.instrumentPath, onInstrumentLoaded, onInstrumentCancelled);
    screenSetup(&screenFileBrowser, 0);
  } else if (row == 0 && col == 2) {
    // Save instrument
    if (instrumentIsEmpty(cInstrument)) {
      screenMessage(MESSAGE_TIME, "Cannot save empty instrument");
    } else {
      char filename[32];
      getInstrumentFilename(filename, sizeof(filename));
      fileBrowserSetupFolderMode("SAVE INSTRUMENT", appSettings.instrumentPath, filename, ".cni", onInstrumentSaved, onInstrumentCancelled);
      screenSetup(&screenFileBrowser, 0);
    }
  } else if (row == 1) {
    // Instrument name
    int res = editCharacter(action, project.instruments[cInstrument].name, col, PROJECT_INSTRUMENT_NAME_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
    } else if (res > 1) {
      handled = 1;
    }
  } else if (row == 2 && col == 0) {
    // Transpose
    handled = edit8noLast(action, &project.instruments[cInstrument].transposeEnabled, 1, 0, 1);
  } else if (row == 2 && col == 1) {
    // Table tic speed
    handled = edit8noLast(action, &project.instruments[cInstrument].tableSpeed, 16, 1, 255);
  }

  return handled;
}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    // To Table screen with the default instrument table
    screenSetup(&screenTable, cInstrument);
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    // To Phrase screen
    screenSetup(&screenPhrase, -1);
    return 1;
  } else if (keys == (keyDown | keyShift)) {
    // To Instrument Pool screen
    screenSetup(&screenInstrumentPool, cInstrument);
    return 1;
  } else if (keys == (keyOpt | keyLeft)) {
    // To the previous instrument
    if (cInstrument != 0) {
      cInstrument--;
      playbackStopPreview(&playback, *pSongTrack);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyOpt | keyRight)) {
    // To the next instrument
    if (cInstrument != PROJECT_MAX_INSTRUMENTS - 1) {
      cInstrument++;
      playbackStopPreview(&playback, *pSongTrack);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyOpt | keyUp)) {
    // +16 instruments
    cInstrument += 16;
    if (cInstrument >= PROJECT_MAX_INSTRUMENTS) cInstrument = PROJECT_MAX_INSTRUMENTS - 1;
    playbackStopPreview(&playback, *pSongTrack);
    fullRedraw();
    return 1;
  } else if (keys == (keyOpt | keyDown)) {
    // -16 instruments
    cInstrument -= 16;
    if (cInstrument < 0) cInstrument = 0;
    playbackStopPreview(&playback, *pSongTrack);
    fullRedraw();
    return 1;
  } else if (keys == (keyEdit | keyPlay)) {
    // Preview instrument
    if (!instrumentIsEmpty(cInstrument) && !playbackIsPlaying(&playback)) {
      uint8_t note = instrumentFirstNote(cInstrument);
      playbackPreviewNote(&playback, *pSongTrack, note, cInstrument);
    }
    return 1;
  } else if (keys == (keyShift | keyOpt)) {
    // Copy instrument
    copyInstrument(cInstrument);
    screenMessage(MESSAGE_TIME, "Copied instrument");
    return 1;
  } else if (keys == (keyShift | keyEdit)) {
    // Paste instrument
    pasteInstrument(cInstrument);
    fullRedraw();
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  // Stop preview when keys are released
  if (keys == 0) {
    playbackStopPreview(&playback, *pSongTrack);
  }

  if (isCharEdit) {
    struct ScreenData* screen = instrumentScreen();
    char result = charEditInput(keys, isDoubleTap, project.instruments[cInstrument].name, screen->cursorCol, PROJECT_INSTRUMENT_NAME_LENGTH);
    if (result) {
      isCharEdit = 0;
      if (screen->cursorCol < 15) screen->cursorCol++;
      fullRedraw();
    }
  } else {
    if (inputScreenNavigation(keys, isDoubleTap)) return;

    struct ScreenData* screen = instrumentScreen();
    if (screenInput(screen, keys, isDoubleTap)) return;
  }
}

const struct AppScreen screenInstrument = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
