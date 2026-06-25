#include "corelib_mainloop.h"
#include <stdint.h>
#include <SDL/SDL.h>

#include "corelib_gfx.h"
#include "corelib_keymap.h"
#include "../../src/corelib/corelib_assets.h"

#define FPS 60

void mainLoopRun(void (*draw)(void), void (*onEvent)(MainLoopEventData eventData)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int menu = 0;
  MainLoopEventData eventData;

  assetsInit();

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        eventData.type = eventExit;
        eventData.data.value = 0;
        onEvent(eventData);
        return;
      } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == BTN_MENU) {
          menu = event.type == SDL_KEYDOWN;
        } else if (menu && event.type == SDL_KEYDOWN && event.key.keysym.sym == BTN_X) {
          eventData.type = eventExit;
          eventData.data.value = 0;
          onEvent(eventData);
          return;
        } else {
          eventData.type = event.type == SDL_KEYDOWN ? eventKeyDown : eventKeyUp;
          eventData.data.input = (InputCode){inputKeyboard, event.key.keysym.sym};
          onEvent(eventData);
        }
      }
    }

    eventData.type = eventTick;
    eventData.data.value = 0;
    onEvent(eventData);

    draw();
    gfxUpdateScreen();

    busytime = SDL_GetTicks() - start;
    if (delay > busytime) {
      SDL_Delay(delay - busytime);
    }
  }
}

void mainLoopDelay(int ms) {
  SDL_Delay(ms);
}

void mainLoopQuit(void) {
  SDL_Quit();
}

void mainLoopTriggerQuit(void) {
  SDL_Event quitEvent;
  quitEvent.type = SDL_QUIT;
  SDL_PushEvent(&quitEvent);
}
