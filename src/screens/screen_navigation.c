#include "screen_navigation.h"
#include <screens.h>
#include <common.h>

// Common navigation handler
int handleScreenNavigation(const struct NavigationConfig* config, int keys, int isDoubleTap) {
  if (!config || !config->entries) return 0;
  
  for (int i = 0; i < config->count; i++) {
    const struct NavigationEntry* entry = &config->entries[i];
    if (entry->keys == keys) {
      screenSetup(entry->screen, entry->input);
      return 1;
    }
  }
  return 0;
}

// Common cursor navigation handler
int handleCursorNavigation(const struct CursorNavConfig* config, int keys) {
  if (!config || !config->cursor) return 0;
  
  int needsPageRedraw = 0;
  int needsCursorRedraw = 0;
  
  if (keys == keyUp && *config->cursor > 0) {
    (*config->cursor)--;
    if (config->topRow && *config->cursor < *config->topRow) {
      (*config->topRow)--;
      needsPageRedraw = 1;
    } else {
      needsCursorRedraw = 1;
    }
  } else if (keys == keyDown && *config->cursor < config->maxItems - 1) {
    (*config->cursor)++;
    if (config->topRow && *config->cursor >= *config->topRow + config->pageSize) {
      (*config->topRow)++;
      needsPageRedraw = 1;
    } else {
      needsCursorRedraw = 1;
    }
  } else if (keys == keyLeft && config->pageSize > 1) {
    // Page up
    *config->cursor -= config->pageSize;
    if (*config->cursor < 0) *config->cursor = 0;
    if (config->topRow) {
      *config->topRow -= config->pageSize;
      if (*config->topRow < 0) *config->topRow = 0;
    }
    needsPageRedraw = 1;
  } else if (keys == keyRight && config->pageSize > 1) {
    // Page down
    *config->cursor += config->pageSize;
    if (*config->cursor >= config->maxItems) *config->cursor = config->maxItems - 1;
    if (config->topRow) {
      *config->topRow += config->pageSize;
      if (*config->topRow + config->pageSize >= config->maxItems) {
        *config->topRow = config->maxItems - config->pageSize;
      }
      if (*config->topRow < 0) *config->topRow = 0;
    }
    needsPageRedraw = 1;
  } else {
    return 0; // Key not handled
  }
  
  // Call appropriate callback
  if (needsPageRedraw && config->onPageChange) {
    config->onPageChange();
  } else if (needsCursorRedraw && config->onCursorChange) {
    config->onCursorChange();
  }
  
  return 1; // Key handled
}

// Song screen navigation
static const struct NavigationEntry songNavEntries[] = {
  NAV_ENTRY(keyRight | keyShift, &screenChain, -1),
  NAV_ENTRY(keyUp | keyShift, &screenProject, 0),
  NAV_END
};

const struct NavigationConfig songNavigation = {
  .entries = songNavEntries,
  .count = 2
};

// Instrument screen navigation  
static const struct NavigationEntry instrumentNavEntries[] = {
  NAV_ENTRY(keyRight | keyShift, &screenTable, -1), // Will use cInstrument
  NAV_ENTRY(keyLeft | keyShift, &screenPhrase, -1),
  NAV_ENTRY(keyDown | keyShift, &screenInstrumentPool, -1), // Will use cInstrument
  NAV_END
};

const struct NavigationConfig instrumentNavigation = {
  .entries = instrumentNavEntries,
  .count = 3
};

// Instrument pool screen navigation
static const struct NavigationEntry instrumentPoolNavEntries[] = {
  NAV_ENTRY(keyUp | keyShift, &screenInstrument, -1), // Will use cursorRow
  NAV_ENTRY(keyLeft | keyShift, &screenPhrase, -1),
  NAV_ENTRY(keyRight | keyShift, &screenTable, -1), // Will use cursorRow
  NAV_END
};

const struct NavigationConfig instrumentPoolNavigation = {
  .entries = instrumentPoolNavEntries,
  .count = 3
};

// Chain screen navigation
static const struct NavigationEntry chainNavEntries[] = {
  NAV_ENTRY(keyRight | keyShift, &screenPhrase, -1),
  NAV_ENTRY(keyLeft | keyShift, &screenSong, 0),
  NAV_END
};

const struct NavigationConfig chainNavigation = {
  .entries = chainNavEntries,
  .count = 2
};

// Phrase screen navigation
static const struct NavigationEntry phraseNavEntries[] = {
  NAV_ENTRY(keyLeft | keyShift, &screenChain, -1),
  NAV_ENTRY(keyUp | keyShift, &screenGroove, 0),
  NAV_END
};

const struct NavigationConfig phraseNavigation = {
  .entries = phraseNavEntries,
  .count = 2
};

// Table screen navigation
static const struct NavigationEntry tableNavEntries[] = {
  NAV_END
};

const struct NavigationConfig tableNavigation = {
  .entries = tableNavEntries,
  .count = 0
};

// Project screen navigation
static const struct NavigationEntry projectNavEntries[] = {
  NAV_ENTRY(keyDown | keyShift, &screenSong, 0),
  NAV_END
};

const struct NavigationConfig projectNavigation = {
  .entries = projectNavEntries,
  .count = 1
};

// Groove screen navigation
static const struct NavigationEntry grooveNavEntries[] = {
  NAV_ENTRY(keyDown | keyShift, &screenPhrase, 0),
  NAV_END
};

const struct NavigationConfig grooveNavigation = {
  .entries = grooveNavEntries,
  .count = 1
};

// Helper function to create cursor navigation config
struct CursorNavConfig createCursorNavConfig(int* cursor, int* topRow, int maxItems, int pageSize, 
                                           void (*onCursorChange)(void), void (*onPageChange)(void)) {
  struct CursorNavConfig config = {
    .cursor = cursor,
    .topRow = topRow,
    .maxItems = maxItems,
    .pageSize = pageSize,
    .onCursorChange = onCursorChange,
    .onPageChange = onPageChange
  };
  return config;
}