#include <stdint.h>
#include <SDL/SDL.h>
#include "mainloop_sdl12.h"
#include "gfx.h"
#include "keymap.h"
#include "input_utils.h"

#define FPS 60

void MainLoopSDL12::run(App& app) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int menu = 0;
  MainLoopEventData eventData;

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        eventData.type = MainLoopEvent::exit;
        eventData.data.value = 0;
        app.onEvent(eventData);
        return;
      } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == BTN_MENU) {
          menu = event.type == SDL_KEYDOWN;
        } else if (menu && event.type == SDL_KEYDOWN && event.key.keysym.sym == BTN_X) {
          eventData.type = MainLoopEvent::exit;
          eventData.data.value = 0;
          app.onEvent(eventData);
          return;
        } else {
          eventData.type = event.type == SDL_KEYDOWN ? MainLoopEvent::keyDown : MainLoopEvent::keyUp;
          eventData.data.input = (InputCode){InputDeviceType::keyboard, event.key.keysym.sym};
          app.onEvent(eventData);
        }
      }
    }

    eventData.type = MainLoopEvent::tick;
    eventData.data.value = 0;
    app.onEvent(eventData);

    app.draw();
    gfx.updateScreen();

    busytime = SDL_GetTicks() - start;
    if (delay > busytime) {
      SDL_Delay(delay - busytime);
    }
  }
}

void MainLoopSDL12::quit() {
  SDL_Quit();
}

void MainLoopSDL12::triggerQuit() {
  SDL_Event quitEvent;
  quitEvent.type = SDL_QUIT;
  SDL_PushEvent(&quitEvent);
}
