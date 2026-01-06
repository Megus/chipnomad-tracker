#include "keyboard_layout.h"
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <ctype.h> // For tolower()

// Only include SDL headers for keyboard detection when building for desktop
// Embedded platforms don't need SDL for locale-based detection
#if defined(DESKTOP_BUILD) && defined(SDL2)
#include <SDL2/SDL.h>
#endif
// Note: SDL1.2 builds don't include headers here since they're embedded or use different paths

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __linux__
#include <langinfo.h>
#endif

// Forward declaration
static KeyboardLayout detectLayoutFromKeyPosition(void);

// Layout detection based on system locale and keyboard mapping patterns
KeyboardLayout detectKeyboardLayout(void) {
  // First try to detect from system locale
  const char* locale = NULL;
  
  // Try different methods to get locale information
#if defined(_WIN32) && !defined(__USE_MINGW_ANSI_STDIO)
  // Windows-specific locale detection (MSVC)
  char localeBuffer[LOCALE_NAME_MAX_LENGTH];
  if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SNAME, localeBuffer, sizeof(localeBuffer)) > 0) {
    locale = localeBuffer;
  }
#elif defined(_WIN32) && defined(__USE_MINGW_ANSI_STDIO)
  // MinGW build - fallback to environment variables
  locale = getenv("LC_ALL");
  if (!locale) locale = getenv("LC_CTYPE");
  if (!locale) locale = getenv("LANG");
#else
  // Unix-like systems (Linux, macOS)
  // First try setlocale to ensure locale is properly initialized
  setlocale(LC_CTYPE, "");
  locale = setlocale(LC_CTYPE, NULL);
  if (!locale || strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0) {
    // Fallback to environment variables in order of precedence
    locale = getenv("LC_ALL");
    if (!locale) locale = getenv("LC_CTYPE");
    if (!locale) locale = getenv("LANG");
    if (!locale) locale = getenv("LANGUAGE");
  }
#endif

  // Analyze locale to determine keyboard layout
  if (locale) {
    // Convert to lowercase for case-insensitive comparison
    char localeLower[32];
    strncpy(localeLower, locale, sizeof(localeLower) - 1);
    localeLower[sizeof(localeLower) - 1] = '\0';
    for (int i = 0; localeLower[i]; i++) {
      localeLower[i] = tolower(localeLower[i]);
    }

    // German-speaking regions typically use QWERTZ
    if (strstr(localeLower, "de") ||     // German
        strstr(localeLower, "at") ||     // Austrian
        strstr(localeLower, "ch") ||     // Swiss
        strstr(localeLower, "lu")) {     // Luxembourg
      return KEYBOARD_LAYOUT_QWERTZ;
    }

    // French-speaking regions typically use AZERTY
    if (strstr(localeLower, "fr") ||     // French
        strstr(localeLower, "be") ||     // Belgian
        strstr(localeLower, "ca")) {     // Canadian French
      return KEYBOARD_LAYOUT_AZERTY;
    }

    // Czech Republic and Slovakia use QWERTZ
    if (strstr(localeLower, "cs") ||     // Czech
        strstr(localeLower, "sk")) {     // Slovak
      return KEYBOARD_LAYOUT_QWERTZ;
    }

    // Polish uses QWERTZ variant
    if (strstr(localeLower, "pl")) {     // Polish
      return KEYBOARD_LAYOUT_QWERTZ;
    }

    // Hungarian uses QWERTZ
    if (strstr(localeLower, "hu")) {     // Hungarian
      return KEYBOARD_LAYOUT_QWERTZ;
    }

    // Spanish and Portuguese typically use QWERTY
    if (strstr(localeLower, "es") ||     // Spanish
        strstr(localeLower, "pt") ||     // Portuguese
        strstr(localeLower, "it") ||     // Italian
        strstr(localeLower, "nl") ||     // Dutch
        strstr(localeLower, "sv") ||     // Swedish
        strstr(localeLower, "no") ||     // Norwegian
        strstr(localeLower, "da") ||     // Danish
        strstr(localeLower, "fi")) {     // Finnish
      return KEYBOARD_LAYOUT_QWERTY;
    }

    // Check for DVORAK layouts explicitly
    if (strstr(localeLower, "dvorak") ||
        strstr(localeLower, "dvp")) {    // Programmer Dvorak
      return KEYBOARD_LAYOUT_DVORAK;
    }

    // US and UK use QWERTY
    if (strstr(localeLower, "en_us") ||  // US English
        strstr(localeLower, "en_gb") ||  // British English
        strstr(localeLower, "en") ||     // General English
        strstr(localeLower, "us") ||     // US
        strstr(localeLower, "gb")) {     // Great Britain
      return KEYBOARD_LAYOUT_QWERTY;
    }
  }

  // Try SDL-based detection as fallback
#if defined(DESKTOP_BUILD) && defined(SDL2)
  // SDL2 provides more detailed keyboard information
  // Try to get keyboard layout name if available
  char* keyboardName = SDL_GetKeyboardName(0); // Get first keyboard's name
  if (keyboardName) {
    char keyboardLower[64];
    strncpy(keyboardLower, keyboardName, sizeof(keyboardLower) - 1);
    keyboardLower[sizeof(keyboardLower) - 1] = '\0';
    for (int i = 0; keyboardLower[i]; i++) {
      keyboardLower[i] = tolower(keyboardLower[i]);
    }

    // Check keyboard name for layout indicators
    if (strstr(keyboardLower, "qwertz") ||
        strstr(keyboardLower, "german") ||
        strstr(keyboardLower, "deutsch")) {
      return KEYBOARD_LAYOUT_QWERTZ;
    }

    if (strstr(keyboardLower, "azerty") ||
        strstr(keyboardLower, "french") ||
        strstr(keyboardLower, "français")) {
      return KEYBOARD_LAYOUT_AZERTY;
    }

    if (strstr(keyboardLower, "dvorak")) {
      return KEYBOARD_LAYOUT_DVORAK;
    }

    if (strstr(keyboardLower, "qwerty")) {
      return KEYBOARD_LAYOUT_QWERTY;
    }
  }
#endif

  // Try active key position testing as final attempt
  // This tests where Y and Z keys are physically located
  KeyboardLayout layoutFromKeyTest = detectLayoutFromKeyPosition();
  if (layoutFromKeyTest != KEYBOARD_LAYOUT_AUTO) {
    return layoutFromKeyTest;
  }

  // Default to AUTO if no specific locale detected
  // This maintains backward compatibility
  return KEYBOARD_LAYOUT_AUTO;
}

// Try to detect layout by testing key positions
// This is a more advanced method that checks the actual mapping
static KeyboardLayout detectLayoutFromKeyPosition(void) {
  // This is a placeholder for a more sophisticated detection
  // In a real implementation, we could:
  // 1. Check scancodes for Y and Z keys
  // 2. Compare with expected positions for different layouts
  // 3. Use platform-specific APIs for keyboard mapping
  
#if defined(DESKTOP_BUILD) && defined(SDL2)
  // SDL2 allows us to get scancode information
  // For now, this is a simplified version
  SDL_Scancode yScan = SDL_GetScancodeFromKey(SDLK_y);
  SDL_Scancode zScan = SDL_GetScancodeFromKey(SDLK_z);
  
  // In QWERTZ layouts, Y and Z are swapped compared to QWERTY
  // This is a basic heuristic
  if (yScan == SDL_SCANCODE_Z && zScan == SDL_SCANCODE_Y) {
    return KEYBOARD_LAYOUT_QWERTZ;
  } else if (yScan == SDL_SCANCODE_Y && zScan == SDL_SCANCODE_Z) {
    return KEYBOARD_LAYOUT_QWERTY;
  }
#endif

  return KEYBOARD_LAYOUT_AUTO;
}

const char* getKeyboardLayoutName(KeyboardLayout layout) {
  switch (layout) {
    case KEYBOARD_LAYOUT_AUTO: return "AUTO ";
    case KEYBOARD_LAYOUT_QWERTY: return "QWERTY";
    case KEYBOARD_LAYOUT_QWERTZ: return "QWERTZ";
    case KEYBOARD_LAYOUT_AZERTY: return "AZERTY";
    case KEYBOARD_LAYOUT_DVORAK: return "DVORAK";
    default: return "UNKNOWN";
  }
}

// Enhanced key mapping for different keyboard layouts
int getKeyForLayout(int keyCode, KeyboardLayout layout) {
  // For AUTO, use current hardcoded mapping
  if (layout == KEYBOARD_LAYOUT_AUTO) {
    return 0; // Let the main decodeKey function handle it
  }
  
  // Here we could add layout-specific mappings
  // For now, return 0 to use default mapping
  return 0;
}

void initKeyboardLayout(void) {
  // Always detect locale-based layout first for reference
  KeyboardLayout localeLayout = detectKeyboardLayout();
  
  // If user has manually set a specific layout (not AUTO), use that
  // If set to AUTO, use the detected locale layout
  if (appSettings.keyboardLayout == KEYBOARD_LAYOUT_AUTO) {
    appSettings.keyboardLayout = localeLayout;
  }
  // Manual setting (not AUTO) takes precedence over locale detection
}

void updateKeyboardLayout(void) {
  // Re-evaluate keyboard layout - useful after settings changes
  initKeyboardLayout();
}
