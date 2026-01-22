#include "screen_project.h"
#include "common.h"
#include "corelib/corelib_file.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "pitch_table_utils.h"
#include "version.h"
#include "audio_manager.h"
#include "file_browser.h"
#include "import/import_vt2.h"
#include <string.h>
#include <strings.h>

static int isCharEdit = 0;
static char* editingString = NULL;
static int editingStringLength = 0;
static int tickRateI = 0;
static uint16_t tickRateF = 0;

static void onProjectLoaded(const char* path) {
  if (!path) {
    screenSetup(&screenProject, 0);
    return;
  }

  // Store the directory path from the selected file
  char* lastSeparator = strrchr(path, PATH_SEPARATOR);
  if (lastSeparator) {
    int pathLen = lastSeparator - path;
    if (pathLen > 0 && pathLen < PATH_LENGTH) {
      strncpy(appSettings.projectPath, path, pathLen);
      appSettings.projectPath[pathLen] = '\0';
    }
  } else {
    appSettings.projectPath[0] = '\0';
  }

  playbackStop(&chipnomadState->playbackState);
  
  // Check file extension to determine loader
  const char* ext = strrchr(path, '.');
  int loadResult = -1;
  
  if (ext != NULL) {
    if (strcasecmp(ext, ".vt2") == 0) {
      // Load VT2 file
      loadResult = projectLoadVT2(path);
    } else if (strcasecmp(ext, ".cnm") == 0) {
      // Load ChipNomad native format
      loadResult = projectLoad(&chipnomadState->project, path);
    } else {
      // Try native format by default
      loadResult = projectLoad(&chipnomadState->project, path);
    }
  } else {
    // No extension, try native format
    loadResult = projectLoad(&chipnomadState->project, path);
  }
  
  if (loadResult == 0) {
    chipnomadInitChips(chipnomadState, appSettings.audioSampleRate, NULL);

    // Store filename without extension
    extractFilenameWithoutExtension(path, appSettings.projectFilename, FILENAME_LENGTH + 1);

    // Reset all screen states (including song position)
    screensInitAll();
  }

  screenSetup(&screenProject, 0);
}

static void onProjectSaved(const char* folderPath) {
  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s%s%s.cnm", folderPath, PATH_SEPARATOR_STR, appSettings.projectFilename);

  if (projectSave(&chipnomadState->project, fullPath) == 0) {
    // Save the directory path
    strncpy(appSettings.projectPath, folderPath, PATH_LENGTH);
    appSettings.projectPath[PATH_LENGTH] = 0;
  }
  screenSetup(&screenProject, 0);
}

static void onProjectCancelled(void) {
  screenSetup(&screenProject, 0);
}

static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);

static ScreenData screenProjectCommon = {
  .rows = 8,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = projectCommonColumnCount,
  .drawStatic = projectCommonDrawStatic,
  .drawCursor = projectCommonDrawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = projectCommonDrawField,
  .onEdit = projectCommonOnEdit,
};

static ScreenData* projectScreen(void) {
  ScreenData* data = &screenProjectCommon;
  if (chipnomadState->project.chipType == chipAY) {
    data = &screenProjectAY;
  }
  data->drawRowHeader = drawRowHeader;
  data->drawColHeader = drawColHeader;

  return data;
}

static void setup(int input) {
  isCharEdit = 0;
  editingString = NULL;
  editingStringLength = 0;
  tickRateI = (int)chipnomadState->project.tickRate;
  tickRateF = (uint16_t)((chipnomadState->project.tickRate - (float)tickRateI) * 1000.f);
}

static void fullRedraw(void) {
  ScreenData* screen = projectScreen();
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

int projectCommonColumnCount(int row) {
  if (row == 0) {
    return 5; // Load, save, new, export, manage
  } else if (row >= 1 && row <= 3) {
    return 24; // File, title, author
  } else if (row == 4) {
    return 1; // Linear pitch
  } else if (row == 5) {
    return 2; // Tick rate
  } else if (row > 5) {
    return 1; // Chip type, chips count
  }
  return 1; // Default value
}

void projectCommonDrawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "PROJECT");

  gfxSetFgColor(cs.textDefault);
  gfxPrintf(8, 0, "%s v%s (%s)", appTitle, appVersion, appBuild);

  gfxPrint(0, 3, "File");
  gfxPrint(0, 4, "Title");
  gfxPrint(0, 5, "Author");

  gfxPrint(0, 7, "Linear pitch");
  gfxPrint(0, 8, "Tick rate");
  gfxPrint(0, 9, "Chip type");
  gfxPrint(0, 10, "Chips count");
}

void projectCommonDrawCursor(int col, int row) {
  if (row == 0) {
    if (col == 0) {
      gfxCursor(7, 2, 4); // Load
    } else if (col == 1) {
      gfxCursor(12, 2, 4); // Save
    } else if (col == 2) {
      gfxCursor(17, 2, 3); // New
    } else if (col == 3) {
      gfxCursor(21, 2, 6); // Export
    } else if (col == 4) {
      gfxCursor(28, 2, 6); // Manage
    }
  } else if (row >= 1 && row <= 3) {
    // Text fields: file name, title, author
    gfxCursor(7 + col, 2 + row, 1);
  } else if (row == 4) {
    // Linear pitch
    gfxCursor(13, 7, 3);
  } else if (row == 5) {
    // Tick rate
    if (col == 0) {
      gfxCursor(13, 8, 3);
    } else {
      gfxCursor(17, 8, 3);
    }
  } else if (row == 6) {
    // Chip type
    // TODO: When there will be other chips, change cursor length
    gfxCursor(13, 9, 5);
  } else if (row == 7) {
    // Chips count
    gfxCursor(13, 10, 1);
  }
}

void projectCommonDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 0) {
    if (col == 0) {
      gfxPrint(7, 2, "Load");
    } else if (col == 1) {
      gfxPrint(12, 2, "Save");
    } else if (col == 2) {
      gfxPrint(17, 2, "New");
    } else if (col == 3) {
      gfxPrint(21, 2, "Export");
    } else if (col == 4) {
      gfxPrint(28, 2, "Manage");
    }
  } else if (row == 1) {
    // File name
    gfxClearRect(7, 3, FILENAME_LENGTH, 1);
    gfxPrintf(7, 3, "%s", appSettings.projectFilename);
  } else if (row == 2) {
    // Title
    gfxClearRect(7, 4, PROJECT_TITLE_LENGTH, 1);
    gfxPrintf(7, 4, "%s", chipnomadState->project.title);
  } else if (row == 3) {
    // Author
    gfxClearRect(7, 5, PROJECT_TITLE_LENGTH, 1);
    gfxPrintf(7, 5, "%s", chipnomadState->project.author);
  } else if (row == 4) {
    // Linear pitch
    gfxPrint(13, 7, chipnomadState->project.linearPitch ? "ON " : "OFF");
  } else if (row == 5) {
    // Tick rate and BPM
    gfxClearRect(13, 8, 27, 1);
    float tickRate = (float)tickRateI + (float)tickRateF / 1000.0f;
    float bpm = tickRate * 60.0f / 24.0f;
    gfxPrintf(13, 8, "%03d.%03dHz (%.1f BPM)", tickRateI, tickRateF, bpm);
  } else if (row == 6) {
    // Chip type
    gfxPrint(13, 9, "AY/YM");
  } else if (row == 7) {
    // Chips count
    gfxPrintf(13, 10, "%d", chipnomadState->project.chipsCount);
  }
}

int projectCommonOnEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (row == 0) {
    // Load/Save/New/Export
    if (col == 0) {
      // Load project - go directly to file browser
      fileBrowserSetup("LOAD PROJECT", ".cnm,.vt2", appSettings.projectPath, onProjectLoaded, onProjectCancelled);
      screenSetup(&screenFileBrowser, 0);
    } else if (col == 1) {
      // Save project - check filename first
      if (strlen(appSettings.projectFilename) == 0) {
        screenMessage(MESSAGE_TIME, "Enter filename");
        handled = 1;
      } else {
        fileBrowserSetupFolderMode("SAVE PROJECT", appSettings.projectPath, appSettings.projectFilename, ".cnm", onProjectSaved, onProjectCancelled);
        screenSetup(&screenFileBrowser, 0);
      }
    } else if (col == 2) {
      // New project
      projectInitAY(&chipnomadState->project);
      appSettings.projectFilename[0] = 0; // Clear filename
      screensInitAll(); // Reset all screen states
      fullRedraw();
      handled = 1;
    } else if (col == 3) {
      // Export - go to export screen
      screenSetup(&screenExport, 0);
      handled = 0;
    } else if (col == 4) {
      // Manage - go to manage screen
      screenSetup(&screenManage, 0);
      handled = 0;
    }
  } else if (row == 1) {
    // File name
    int res = editCharacter(action, appSettings.projectFilename, col, FILENAME_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
      editingString = appSettings.projectFilename;
      editingStringLength = FILENAME_LENGTH;
    } else if (res > 1) {
      handled = 1;
    }
  } else if (row == 2) {
    // Title
    int res = editCharacter(action, chipnomadState->project.title, col, PROJECT_TITLE_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
      editingString = chipnomadState->project.title;
      editingStringLength = PROJECT_TITLE_LENGTH;
    } else if (res > 1) {
      handled = 1;
    }
  } else if (row == 3) {
    // Author
    int res = editCharacter(action, chipnomadState->project.author, col, PROJECT_TITLE_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
      editingString = chipnomadState->project.author;
      editingStringLength = PROJECT_TITLE_LENGTH;
    } else if (res > 1) {
      handled = 1;
    }
  } else if (row == 4) {
    // Linear pitch (ON/OFF)
    handled = edit8noLast(action, &chipnomadState->project.linearPitch, 1, 0, 1);
    if (handled) {
      playbackStop(&chipnomadState->playbackState);
      reinitializePitchTable(&chipnomadState->project);
    }
  } else if (row == 5) {
    // Tick rate
    if (col == 0) {
      // Integer part (1-200), clear sets to 50
      if (action == editClear) {
        tickRateI = 50;
        handled = 1;
      } else {
        handled = edit8noLast(action, (uint8_t*)&tickRateI, 10, 1, 200);
      }
    } else {
      // Fractional part (.000-.999) with overflow
      handled = edit16withOverflow(action, &tickRateF, 100, 0, 999);
    }

    if (handled) {
      // Update project tick rate from the two components
      chipnomadState->project.tickRate = (float)tickRateI + (float)tickRateF / 1000.0f;
    }
  } else if (row == 7) {
    // Chips count (1-3 for AY chips)
    handled = edit8noLast(action, (uint8_t*)&chipnomadState->project.chipsCount, 1, 1, 3);
    if (handled) {
      // Stop playback when chips count changes
      playbackStop(&chipnomadState->playbackState);
      // Clear note preview area
      clearNotePreview();
      // Update tracks count when chips count changes
      chipnomadState->project.tracksCount = projectGetTotalTracks(&chipnomadState->project);
      // Reinitialize chips with new count
      chipnomadInitChips(chipnomadState, appSettings.audioSampleRate, NULL);
    }
  }

  return handled;
}


///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int tapCount) {
  if (keys == (keyDown | keyShift)) {
    screenSetup(&screenSong, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (isCharEdit) {
    ScreenData* screen = projectScreen();
    char result = charEditInput(keys, tapCount, editingString, screen->cursorCol, editingStringLength);

    if (result) {
      isCharEdit = 0;
      if (screen->cursorCol < editingStringLength - 1) screen->cursorCol++;
      editingString = NULL;
      editingStringLength = 0;
      fullRedraw();
    }
  } else {
    if (inputScreenNavigation(keys, tapCount)) return 1;

    ScreenData* screen = projectScreen();
    if (screenInput(screen, isKeyDown, keys, tapCount)) return 1;
  }
  return 0;
}

const AppScreen screenProject = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
