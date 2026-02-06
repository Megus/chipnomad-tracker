#include "corelib_input.h"
#include "common.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Callback for raw input capture
void (*inputRawCallback)(int32_t keyCode, int isDown) = NULL;

// Keyboard layout detection (only used for first-launch default key mapping)
typedef enum {
  LAYOUT_QWERTY = 0,
  LAYOUT_QWERTZ = 1
} KeyboardLayout;

static KeyboardLayout detectKeyboardLayout(void) {
  const char* locale = NULL;

#if defined(_WIN32) && !defined(__USE_MINGW_ANSI_STDIO)
  char localeBuffer[LOCALE_NAME_MAX_LENGTH];
  if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SNAME, localeBuffer, sizeof(localeBuffer)) > 0) {
    locale = localeBuffer;
  }
#elif defined(_WIN32) && defined(__USE_MINGW_ANSI_STDIO)
  locale = getenv("LC_ALL");
  if (!locale) locale = getenv("LC_CTYPE");
  if (!locale) locale = getenv("LANG");
#else
  setlocale(LC_CTYPE, "");
  locale = setlocale(LC_CTYPE, NULL);
  if (!locale || strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0) {
    locale = getenv("LC_ALL");
    if (!locale) locale = getenv("LC_CTYPE");
    if (!locale) locale = getenv("LANG");
  }
#endif

  if (locale) {
    char localeLower[32];
    strncpy(localeLower, locale, sizeof(localeLower) - 1);
    localeLower[sizeof(localeLower) - 1] = '\0';
    for (int i = 0; localeLower[i]; i++) {
      localeLower[i] = tolower(localeLower[i]);
    }

    // QWERTZ regions
    if (strstr(localeLower, "de_") || strstr(localeLower, "de.") ||
        strstr(localeLower, "_at") || strstr(localeLower, ".at") ||
        strstr(localeLower, "_ch") || strstr(localeLower, ".ch") ||
        strstr(localeLower, "cs_") || strstr(localeLower, "cs.") ||
        strstr(localeLower, "_cz") || strstr(localeLower, ".cz") ||
        strstr(localeLower, "sk_") || strstr(localeLower, "sk.") ||
        strstr(localeLower, "_sk") || strstr(localeLower, ".sk") ||
        strstr(localeLower, "pl_") || strstr(localeLower, "pl.") ||
        strstr(localeLower, "_pl") || strstr(localeLower, ".pl") ||
        strstr(localeLower, "hu_") || strstr(localeLower, "hu.") ||
        strstr(localeLower, "_hu") || strstr(localeLower, ".hu")) {
      return LAYOUT_QWERTZ;
    }
  }

  return LAYOUT_QWERTY;
}

void inputInitDefaultKeyMapping(void) {
#if defined(DESKTOP_BUILD) || defined(PORTMASTER_BUILD)
  KeyboardLayout layout = detectKeyboardLayout();

  appSettings.keyMapping.keyUp[0] = SDLK_UP;
  appSettings.keyMapping.keyUp[1] = 0;
  appSettings.keyMapping.keyUp[2] = 0;
  appSettings.keyMapping.keyDown[0] = SDLK_DOWN;
  appSettings.keyMapping.keyDown[1] = 0;
  appSettings.keyMapping.keyDown[2] = 0;
  appSettings.keyMapping.keyLeft[0] = SDLK_LEFT;
  appSettings.keyMapping.keyLeft[1] = 0;
  appSettings.keyMapping.keyLeft[2] = 0;
  appSettings.keyMapping.keyRight[0] = SDLK_RIGHT;
  appSettings.keyMapping.keyRight[1] = 0;
  appSettings.keyMapping.keyRight[2] = 0;
  appSettings.keyMapping.keyOpt[0] = (layout == LAYOUT_QWERTZ) ? SDLK_y : SDLK_z;
  appSettings.keyMapping.keyOpt[1] = 0;
  appSettings.keyMapping.keyOpt[2] = 0;
  appSettings.keyMapping.keyPlay[0] = SDLK_SPACE;
  appSettings.keyMapping.keyPlay[1] = 0;
  appSettings.keyMapping.keyPlay[2] = 0;
  appSettings.keyMapping.keyShift[0] = SDLK_LSHIFT;
  appSettings.keyMapping.keyShift[1] = 0;
  appSettings.keyMapping.keyShift[2] = 0;
  appSettings.keyMapping.keyEdit[0] = SDLK_x;
  appSettings.keyMapping.keyEdit[1] = 0;
  appSettings.keyMapping.keyEdit[2] = 0;
#else
  memset(&appSettings.keyMapping, 0, sizeof(KeyMapping));
#endif
}

const char* inputGetKeyName(int32_t keyCode) {
  if (keyCode == 0) return "---";

  if (keyCode < 0) {
    switch (-keyCode) {
      case SDL_CONTROLLER_BUTTON_A: return "Pad A";
      case SDL_CONTROLLER_BUTTON_B: return "Pad B";
      case SDL_CONTROLLER_BUTTON_X: return "Pad X";
      case SDL_CONTROLLER_BUTTON_Y: return "Pad Y";
      case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "Pad L1";
      case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "Pad R1";
      case SDL_CONTROLLER_BUTTON_BACK: return "PadSel";
      case SDL_CONTROLLER_BUTTON_START: return "PadStrt";
      case SDL_CONTROLLER_BUTTON_DPAD_UP: return "Pad Up";
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "PadDown";
      case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "PadLeft";
      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "PadRght";
      default: return "Pad ?";
    }
  }

  // Keyboard - custom short names for common keys
  switch (keyCode) {
    case SDLK_LSHIFT: return "LShift";
    case SDLK_RSHIFT: return "RShift";
    case SDLK_LCTRL: return "LCtrl";
    case SDLK_RCTRL: return "RCtrl";
    case SDLK_LALT: return "LAlt";
    case SDLK_RALT: return "RAlt";
    case SDLK_SPACE: return "Space";
    case SDLK_RETURN: return "Return";
    case SDLK_BACKSPACE: return "BkSpace";
    case SDLK_ESCAPE: return "Escape";
  }

  const char* name = SDL_GetKeyName(keyCode);
  if (name && name[0]) return name;

  return "???";
}
