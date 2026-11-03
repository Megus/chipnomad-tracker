#include <string.h>
#include <strings.h>
#include "screen_project.h"
#include "file_system.h"
#include "gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "pitch_table_utils.h"
#include "version.h"
#include "audio_manager.h"
#include "import/import_vt2.h"
#include "error.h"

/*
static void onProjectLoaded(const char* path) {
  if (!path) {
    screenSetup(&screenProject, 0);
    return;
  }

  // Store the directory path from the selected file
  const char* lastSeparator = strrchr(path, PATH_SEPARATOR);
  if (lastSeparator) {
    int pathLen = lastSeparator - path;
    if (pathLen > 0 && pathLen < PATH_LENGTH) {
      strncpy(appSettings.projectPath, path, pathLen);
      appSettings.projectPath[pathLen] = '\0';
    }
  } else {
    appSettings.projectPath[0] = '\0';
  }

  chipnomadState->engine->player.stop();

  // Check file extension to determine loader
  const char* ext = strrchr(path, '.');
  int success = 0;

  if (ext != NULL && strcasecmp(ext, ".vt2") == 0) {
    // Load VT2 file
    success = projectLoadVT2(path);
  } else {
    // No extension, try native format
    success = projectLoad(&chipnomadState->project, path);
  }

  if (success) {
    projectModified = 0; // Clear modified flag after loading
    audio.reinitChips();

    // Store filename without extension
    extractFilenameWithoutExtension(path, appSettings.projectFilename, FILENAME_LENGTH + 1);

    // Reset all screen states (including song position)
    screensInitAll();
    // Clear FX states for all tracks
    // TODO: Make it a cleaner solution and have it somewhere in chipnomad_lib
    for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
      memset(&chipnomadState->engine->player.tracks[i].note.fx, 0, sizeof(chipnomadState->engine->player.tracks[i].note.fx));
    }
  } else {
    screenMessage(MESSAGE_TIME, "%s", chipnomad::Error::message);
  }

  screenSetup(&screenProject, 0);
}

static void onProjectSaved(const char* folderPath) {
  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s%s%s.cnm", folderPath, PATH_SEPARATOR_STR, appSettings.projectFilename);

  if (projectSave(&chipnomadState->project, fullPath)) {
    // Save the directory path
    projectModified = 0; // Clear modified flag after saving
    strncpy(appSettings.projectPath, folderPath, PATH_LENGTH);
    appSettings.projectPath[PATH_LENGTH] = 0;
  }
  screenSetup(&screenProject, 0);
}

static void onProjectCancelled(void) {
  screenSetup(&screenProject, 0);
}


static void doNewProject(void) {
  projectInitAY(&chipnomadState->project);
  projectModified = 0;
  appSettings.projectFilename[0] = 0;
  screensInitAll();
  screenSetup(&screenProject, 0);
}

static void doLoadProject(void) {
  fileBrowserSetup("LOAD PROJECT", ".cnm,.vt2", appSettings.projectPath, onProjectLoaded, onProjectCancelled);
  screenSetup(&screenFileBrowser, 0);
}

static void cancelConfirm(void) {
  screenSetup(&screenProject, 0);
}
*/

ScreenProject::ScreenProject(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx): Screen(state, dispatcher, gfx) {
  partialAY = new ScreenPartialProjectAY(*this, 8);
}

ScreenProject::~ScreenProject() {
  delete partialAY;
}

void ScreenProject::setup(int input) {
  editingString = NULL;
  tickRateI = (int)state.project.tickRate;
  tickRateF = (uint16_t)((state.project.tickRate - (float)tickRateI) * 1000.f);
  partial = partialAY; // TODO: Select chip-specific partial
}

int ScreenProject::getRowCount() {
  return ScreenProject::commonRowCount + partial->getRowCount();
}

int ScreenProject::getColumnCount(int row) {
  if (row == 0) {
    return 5; // Load, save, new, export, manage
  } else if (row >= 1 && row <= 3) {
    return 24; // File, title, author
  } else if (row == 4) {
    return 1; // Linear pitch
  } else if (row == 5) {
    return 2; // Tick rate
  } else if (row >= 6) {
    return 1; // Chip type, chips count
  } else {
    return partial->getColumnCount(row);
  }
}

void ScreenProject::drawStatic() {
  gfx.setFgColor(theme().textTitles);
  gfx.print(0, 0, "PROJECT");

  gfx.setFgColor(theme().textDefault);
  gfx.printf(8, 0, "%s v%s (%s)", appTitle, appVersion, appBuild);

  gfx.print(0, 3, "File");
  gfx.print(0, 4, "Title");
  gfx.print(0, 5, "Author");

  gfx.print(0, 7, "Linear pitch");
  gfx.print(0, 8, "Tick rate");
  gfx.print(0, 9, "Chip type");
  gfx.print(0, 10, "Chips count");

  partial->drawStatic();
}

void ScreenProject::drawCursor(int col, int row) {
  if (row == 0) {
    if (col == 0) {
      gfx.cursor(7, 2, 4); // Load
    } else if (col == 1) {
      gfx.cursor(12, 2, 4); // Save
    } else if (col == 2) {
      gfx.cursor(17, 2, 3); // New
    } else if (col == 3) {
      gfx.cursor(21, 2, 6); // Export
    } else if (col == 4) {
      gfx.cursor(28, 2, 6); // Manage
    }
  } else if (row >= 1 && row <= 3) {
    // Text fields: file name, title, author
    gfx.cursor(7 + col, 2 + row, 1);
  } else if (row == 4) {
    // Linear pitch
    gfx.cursor(13, 7, 3);
  } else if (row == 5) {
    // Tick rate
    if (col == 0) {
      gfx.cursor(13, 8, 3);
    } else {
      gfx.cursor(17, 8, 3);
    }
  } else if (row == 6) {
    // Chip type
    // TODO: When there will be other chips, change cursor length
    gfx.cursor(13, 9, 5);
  } else if (row == 7) {
    // Chips count
    gfx.cursor(13, 10, 1);
  } else {
    partial->drawCursor(col, row);
  }
}

void ScreenProject::drawCell(int col, int row, CellState cellState) {
  gfx.setFgColor(cellState == CellState::focus ? theme().textValue : theme().textDefault);

  if (row == 0) {
    if (col == 0) {
      gfx.print(7, 2, "Load");
    } else if (col == 1) {
      gfx.print(12, 2, "Save");
    } else if (col == 2) {
      gfx.print(17, 2, "New");
    } else if (col == 3) {
      gfx.print(21, 2, "Export");
    } else if (col == 4) {
      gfx.print(28, 2, "Manage");
    }
  } else if (row == 1) {
    // File name
    gfx.clearRect(7, 3, kFileNameLength, 1);
    gfx.printf(7, 3, "%s", state.settings.projectFilename);
  } else if (row == 2) {
    // Title
    gfx.clearRect(7, 4, PROJECT_TITLE_LENGTH, 1);
    gfx.printf(7, 4, "%s", state.project.title);
  } else if (row == 3) {
    // Author
    gfx.clearRect(7, 5, PROJECT_TITLE_LENGTH, 1);
    gfx.printf(7, 5, "%s", state.project.author);
  } else if (row == 4) {
    // Linear pitch
    gfx.print(13, 7, state.project.linearPitch ? "ON " : "OFF");
  } else if (row == 5) {
    // Tick rate and BPM
    gfx.clearRect(13, 8, 27, 1);
    float tickRate = (float)tickRateI + (float)tickRateF / 1000.0f;
    float bpm = tickRate * 60.0f / 24.0f;
    gfx.printf(13, 8, "%03d.%03dHz (%.1f BPM)", tickRateI, tickRateF, bpm);
  } else if (row == 6) {
    // Chip type
    gfx.print(13, 9, "AY/YM");
  } else if (row == 7) {
    // Chips count
    gfx.printf(13, 10, "%d", state.project.chipsCount);
  } else {
    partial->drawCell(col, row, cellState);
  }
}

bool ScreenProject::onEdit(int col, int row, EditCellAction action) {
  int handled = 0;

  if (row == 0) {
    // Load/Save/New/Export
    if (col == 0) {
      // Load project
      if (projectModified) {
        confirmSetup("Lose unsaved changes?", doLoadProject, cancelConfirm);
        screenSetup(&screenConfirm, 0);
      } else {
        doLoadProject();
      }
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
      if (projectModified) {
        confirmSetup("Lose unsaved changes?", doNewProject, cancelConfirm);
        screenSetup(&screenConfirm, 0);
      } else {
        doNewProject();
      }
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
      projectModified = 1;
      chipnomadState->engine->player.stop();
      reinitializePitchTable(&chipnomadState->project);
    }
  } else if (row == 5) {
    // Tick rate
    if (col == 0) {
      // Integer part (1-200), clear sets to 50
      if (action == EditCellAction::clear) {
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
      projectModified = 1;
    }
  } else if (row == 7) {
    // Chips count (1-3 for AY)
    handled = edit8noLast(action, (uint8_t*)&chipnomadState->project.chipsCount, 1, 1, 3);
    if (handled) {
      chipnomadState->engine->player.stop();
      gfxClearRect(35, 3, 5, PROJECT_MAX_TRACKS); // Clear the note preview area for all tracks (right side of screen)
      chipnomadState->project.tracksCount = projectGetTotalTracks(&chipnomadState->project);
      audio.reinitChips();
      projectModified = 1;
    }
  }

  return handled;
}

bool ScreenProject::onNavigationInput(InputEventData e) {
  if (e.keys == (keyDown | keyShift)) {
    screenSetup(&screenSong, 0);
    return 1;
  }
  return 0;
}

bool ScreenProject::onEvent(Event e) {
  if (isCharEdit) {
    ScreenData* screen = projectScreen();
    char result = charEditInput(keys, tapCount, editingString, screen->cursorCol, editingStringLength);

    if (result) {
      projectModified = 1;
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
