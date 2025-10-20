#ifndef __SCREEN_NAVIGATION_H__
#define __SCREEN_NAVIGATION_H__

#include <screens.h>

// Navigation directions
enum NavigationDirection {
  NAV_LEFT,
  NAV_RIGHT,
  NAV_UP,
  NAV_DOWN
};

// Navigation entry
struct NavigationEntry {
  int keys;                           // Key combination
  const struct AppScreen* screen;     // Target screen
  int input;                          // Input parameter for target screen (-1 for default)
};

// Navigation configuration
struct NavigationConfig {
  const struct NavigationEntry* entries;
  int count;
};

// Cursor navigation configuration
struct CursorNavConfig {
  int* cursor;                        // Pointer to cursor position
  int* topRow;                        // Pointer to top row (for scrolling)
  int maxItems;                       // Maximum number of items
  int pageSize;                       // Items per page (for scrolling)
  void (*onCursorChange)(void);       // Callback when cursor changes
  void (*onPageChange)(void);         // Callback when page changes
};

// Common navigation handler
int handleScreenNavigation(const struct NavigationConfig* config, int keys, int isDoubleTap);

// Common cursor navigation handler
int handleCursorNavigation(const struct CursorNavConfig* config, int keys);

// Helper function to create cursor navigation config
struct CursorNavConfig createCursorNavConfig(int* cursor, int* topRow, int maxItems, int pageSize, 
                                           void (*onCursorChange)(void), void (*onPageChange)(void));

// Helper macros for common navigation patterns
#define NAV_ENTRY(key_combo, target_screen, param) {key_combo, target_screen, param}
#define NAV_END {0, NULL, 0}

// Common navigation configurations
extern const struct NavigationConfig songNavigation;
extern const struct NavigationConfig instrumentNavigation;
extern const struct NavigationConfig instrumentPoolNavigation;
extern const struct NavigationConfig chainNavigation;
extern const struct NavigationConfig phraseNavigation;
extern const struct NavigationConfig tableNavigation;
extern const struct NavigationConfig projectNavigation;
extern const struct NavigationConfig grooveNavigation;

#endif