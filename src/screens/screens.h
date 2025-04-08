#ifndef __SCREENS_H__
#define __SCREENS_H__

#include <common.h>

enum AppEventType {
  appEventSetup,
  appEventFullRedraw,
  appEventDraw,
  appEventKey,
};

struct AppEventKey {
  int keys;
  int isDoubleTap;
};

struct AppEventSetup {
  int input;
};

union AppEventData {
  struct AppEventKey key;
  struct AppEventSetup setup;
};

struct AppEvent {
  enum AppEventType type;
  union AppEventData data;
};

typedef int (*AppScreen)(struct AppEvent event);

int screenProject(struct AppEvent event);
int screenSong(struct AppEvent event);
int screenChain(struct AppEvent event);
int screenPhrase(struct AppEvent event);
int screenGroove(struct AppEvent event);
int screenInstrument(struct AppEvent event);
int screenTable(struct AppEvent event);

extern AppScreen currentScreen;

void setupScreen(const AppScreen screen, int input);
void screenMessage(const char* format, ...);

#endif
