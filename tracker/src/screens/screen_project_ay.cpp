#include <string.h>
#include "gfx.h"
#include "audio_manager.h"
#include "chipnomad_lib.h"
#include "screen_project.h"
#include "chip_ay.h"

int chipClockLength = 0;

void ScreenPartialProjectAY::drawStatic() {
  Gfx& gfx = screen.gfx;

  gfx.setFgColor(screen.theme().textDefault);
  gfx.print(0, 11, "Subtype");
  gfx.print(0, 12, "Stereo");
  gfx.print(0, 13, "Stereo width");
  gfx.print(0, 14, "PWM range");
  gfx.print(0, 15, "Chip clock");
  gfx.print(0, 16, "Pitch table");
}

void ScreenPartialProjectAY::drawCursor(int col, int row) {
  int startRow = screen.commonRowCount;
  Gfx& gfx = screen.gfx;

  if (row == startRow) {
    // Chip type
    gfx.cursor(13, 11, screen.state.project->chipSetup.ay.isYM ? 7 : 9);
  } else if (row == startRow + 1) {
    // Panning scheme
    gfx.cursor(13, 12, 3);
  } else if (row == startRow + 2) {
    // Stereo width
    gfx.cursor(13, 13, 4);
  } else if (row == startRow + 3) {
    // PWM range
    gfx.cursor(13, 14, 3);
  } else if (row == startRow + 4) {
    // Chip clock
    gfx.cursor(13, 15, chipClockLength);
  } else if (row == startRow + 5) {
    // Pitch table
    gfx.cursor(13, 16, strlen(screen.state.project->pitchTable.name));
  }
}

void ScreenPartialProjectAY::drawCell(int col, int row, CellState state) {
  int startRow = screen.commonRowCount;
  Gfx& gfx = screen.gfx;

  gfx.setFgColor(state == CellState::focus ? screen.theme().textValue : screen.theme().textDefault);

  if (row == startRow) {
    gfx.clearRect(13, 11, 9, 1);
    gfx.print(13, 11, screen.state.project->chipSetup.ay.isYM ? "YM2149F" : "AY-3-8910");
  } else if (row == startRow + 1) {
    // Panning scheme
    gfx.clearRect(13, 12, 5, 1);
    gfx.print(13, 12, stereoModes[static_cast<int>(screen.state.project->chipSetup.ay.stereoMode)]);
  } else if (row == startRow + 2) {
    // Stereo width
    gfx.printf(13, 13, "%03d%%", screen.state.project->chipSetup.ay.stereoSeparation);
  } else if (row == startRow + 3) {
    // PWM range
    gfx.clearRect(13, 14, 3, 1);
    gfx.print(13, 14, screen.state.project->chipSetup.ay.pwmFullRange ? "256" : "16");
  } else if (row == startRow + 4) {
    int presetIndex = getClockPresetIndex(screen.state.project->chipSetup.ay.clock);
    char clockText[20];

    if (presetIndex >= 0) {
      strcpy(clockText, clockNames[presetIndex]);
    } else {
      snprintf(clockText, sizeof(clockText), "%d Hz", screen.state.project->chipSetup.ay.clock);
    }

    chipClockLength = strlen(clockText);
    gfx.clearRect(13, 15, 20, 1);
    gfx.print(13, 15, clockText);
  } else if (row == startRow + 5) {
    gfx.clearRect(13, 16, PROJECT_PITCH_TABLE_TITLE_LENGTH, 1);
    gfx.print(13, 16, screen.state.project->pitchTable.name);
  }
}

bool ScreenPartialProjectAY::onEdit(int col, int row, EditCellAction action) {
  int startRow = screen.commonRowCount;
  bool handled = false;

  if (row == startRow) {
    // Chip subtype (AY-3-8910 / YM2149F)
    handled = edit8noLast(action, &screen.state.project->chipSetup.ay.isYM, 1, 0, 1);
    if (handled) {
      for (int i = 0; i < screen.state.project->chipsCount; i++) {
        static_cast<chipnomad::SoundChipAY*>(screen.audio.engine.chips[i])->updateType(screen.state.project->chipSetup.ay.isYM);
      }
    }
  } else if (row == startRow + 1) {
    // Stereo mode (ABC, ACB, BAC)
    handled = edit8noLast(action, (uint8_t*)&screen.state.project->chipSetup.ay.stereoMode, 1, 0, 2);
    if (handled) {
      for (int i = 0; i < screen.state.project->chipsCount; i++) {
        static_cast<chipnomad::SoundChipAY*>(screen.audio.engine.chips[i])->updateStereoMode(screen.state.project->chipSetup.ay.stereoMode, screen.state.project->chipSetup.ay.stereoSeparation);
      }
    }
  } else if (row == startRow + 2) {
    // Stereo width (0-100%)
    handled = edit8noLast(action, &screen.state.project->chipSetup.ay.stereoSeparation, 10, 0, 100);
    if (handled) {
      for (int i = 0; i < screen.state.project->chipsCount; i++) {
        static_cast<chipnomad::SoundChipAY*>(screen.audio.engine.chips[i])->updateStereoMode(screen.state.project->chipSetup.ay.stereoMode, screen.state.project->chipSetup.ay.stereoSeparation);
      }
    }
  } else if (row == startRow + 3) {
    // PWM range (16 / 256)
    handled = edit8noLast(action, &screen.state.project->chipSetup.ay.pwmFullRange, 1, 0, 1);
  } else if (row == startRow + 4) {
    // Chip clock presets
    int presetIndex = getClockPresetIndex(screen.state.project->chipSetup.ay.clock);
    if (presetIndex < 0) presetIndex = 0; // Default to first preset if not found
    uint8_t newIndex = presetIndex;
    handled = edit8noLast(action, &newIndex, 1, 0, 4);
    if (handled) {
      screen.state.project->chipSetup.ay.clock = clockPresets[newIndex];
      for (int i = 0; i < screen.state.project->chipsCount; i++) {
        static_cast<chipnomad::SoundChipAY*>(screen.audio.engine.chips[i])->updateClock(screen.state.project->chipSetup.ay.clock);
      }
    }
  } else if (row == startRow + 5) {
    // Pitch table - enter pitch table screen
    screen.events.dispatch(Event::pushScreen(ScreenEnum::pitchTable));
    handled = true;
  }

  if (handled) screen.state.projectModified = 1;

  return handled;
}

int ScreenPartialProjectAY::getClockPresetIndex(int clock) {
  for (int i = 0; i < 5; i++) {
    if (clockPresets[i] == clock) return i;
  }
  return -1; // Not found
}
