#ifndef __SCREENS_H__
#define __SCREENS_H__

#include <common.h>

enum AppEventType {
  appEventKey,
};

struct AppEvent {

  int keys;
};

struct AppScreen {
  void (*setup)(int input);
  void (*fullRedraw)(void);
  void (*draw)(void);
  int (*onEvent)(struct AppEvent event);
};


extern const struct AppScreen screenSong;

#endif
