#include <stdarg.h>
#include <string.h>
#include "screens.h"
#include "screen_settings.h"
#include "chipnomad_lib.h"
#include "gfx.h"
#include "file_system.h"
#include "utils.h"
#include "copy_paste.h"

const AppScreen* currentScreen = NULL;

static int messageTimer = -1;
static char messageBuffer[42] = "";

static AppScreen const* pendingScreen;
static int pendingScreenInput;

void drawScreenMap() {
  const static int smY = 15;

  const ColorScheme cs = appSettings.colorScheme;
  gfxSetBgColor(cs.background);
  gfxSetFgColor(cs.textInfo);
  gfxClearRect(35, smY, 5, 5);

  // Core screens
  gfxPrint(35, smY + 1, "SCPIT");

  // Additional screens
  if (currentScreen == &screenSong || currentScreen == &screenProject) {
    gfxPrint(35, smY, "P");
  } else if (currentScreen == &screenPhrase || currentScreen == &screenGroove) {
    gfxPrint(37, smY, "G");
  } else if (currentScreen == &screenInstrument || currentScreen == &screenInstrumentPool) {
    gfxPrint(38, smY + 2, "P");
    gfxPrint(38, smY, "M");
  } else if (currentScreen == &screenModulation) {
    gfxPrint(38, smY + 2, "P");
  } else if (currentScreen == &screenTable || currentScreen == &screenWavetable) {
    gfxPrint(39, smY + 2, "W");
  }

  // Show Settings below Song
  if (currentScreen == &screenSong) {
    gfxPrint(35, smY + 2, "S");
  }

  // Highlight current screen
  gfxSetFgColor(cs.textDefault);
  if (currentScreen == &screenSong) {
    gfxPrint(35, smY + 1, "S");
  } else if (currentScreen == &screenChain) {
    gfxPrint(36, smY + 1, "C");
  } else if (currentScreen == &screenPhrase) {
    gfxPrint(37, smY + 1, "P");
  } else if (currentScreen == &screenInstrument) {
    gfxPrint(38, smY + 1, "I");
  } else if (currentScreen == &screenInstrumentPool) {
    gfxPrint(38, smY + 2, "P");
  } else if (currentScreen == &screenModulation) {
    gfxPrint(38, smY, "M");
  } else if (currentScreen == &screenTable) {
    gfxPrint(39, smY + 1, "T");
  } else if (currentScreen == &screenWavetable) {
    gfxPrint(39, smY + 2, "W");
  } else if (currentScreen == &screenProject) {
    gfxPrint(35, smY, "P");
  } else if (currentScreen == &screenGroove) {
    gfxPrint(37, smY, "G");
  } else if (currentScreen == &screenSettings) {
    gfxPrint(35, smY + 2, "S");
  }
}

void screenSetup(const AppScreen* screen, int input) {
  pendingScreen = screen;
  pendingScreenInput = input;
}

void screenDraw() {
  if (pendingScreen != NULL) {
    currentScreen = pendingScreen;
    currentScreen->setup(pendingScreenInput);
    gfxSetBgColor(appSettings.colorScheme.background);
    gfxSetCursorColor(appSettings.colorScheme.cursor);
    gfxClearRect(0, 0, 40, 20);
    currentScreen->fullRedraw();
    drawScreenMap();

    pendingScreen = NULL;
  }

  currentScreen->draw();

  // Draw cached message
  if (strlen(messageBuffer) > 0) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxClearRect(0, 19, 40, 1);
    gfxPrint(0, 19, messageBuffer);
  }

  if (messageTimer >= 0) {
    messageTimer--;
    if (messageTimer == 0) {
      messageBuffer[0] = '\0';
      gfxClearRect(0, 19, 40, 1);
    }
  }
}


void screenMessage(int time, const char* format, ...) {
  // Don't clear timed messages
  if (messageTimer > 0 && strlen(format) == 0) {
    return;
  }

  messageTimer = time;

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  // Clear message area immediately if setting empty message
  if (strlen(messageBuffer) == 0) {
    gfxClearRect(0, 19, 40, 1);
  }
}

void screensInitAll(void) {
  screenSong.init();
  screenChain.init();
  screenPhrase.init();
  screenTable.init();
  screenWavetable.init();
  screenInstrument.init();
  screenGroove.init();
  resetCopyBuffers();
}


///////////////////////////////////////////////////////////////////////////////
//
// Spreadsheet screen functions
//

