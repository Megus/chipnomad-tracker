#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "font_manager.h"
#include "mainloop.h"
#include "app.h"
#include "file_system.h"
#include "font_manager.h"

#if defined(SDL12_BUILD)
// SDL1.2 includes
#include "audio_device_sdl12.h"
#include "gfx_sdl12.h"
#include "input_utils_sdl12.h"
#include "mainloop_sdl12.h"
#include "assets_sdl12.h"
#else
// SDL2 includes
#include "audio_device_sdl2.h"
#include "gfx_sdl2.h"
#include "input_utils_sdl2.h"
#include "mainloop_sdl2.h"
#if defined(ANDROID_BUILD)
#include "assets_android.h"
#else
#include "assets_sdl2.h"
#endif
#endif

int main(int argv, char** args) {
  // Initialize platform-specific implementations of the core library
  // There are only SDL1.2 and SDL2 currently, with SDL2 being the default.
  FontManager fontManager = FontManager();
  FileSystem fileSystem = FileSystem();

#if defined(SDL12_BUILD)
  AudioDeviceSDL12 audioDevice = AudioDeviceSDL12();
  GfxSDL12 gfx = GfxSDL12(fontManager);
  InputUtilsSDL12 inputUtils = InputUtilsSDL12();
  MainLoopSDL12 mainLoop = MainLoopSDL12(gfx);
  AssetsSDL12 assets = AssetsSDL12();
#else
  AudioDeviceSDL2 audioDevice = AudioDeviceSDL2();
  GfxSDL2 gfx = GfxSDL2(fontManager);
  InputUtilsSDL2 inputUtils = InputUtilsSDL2();
  MainLoopSDL2 mainLoop = MainLoopSDL2(gfx);
#if defined(ANDROID_BUILD)
  AssetsAndroid assets = AssetsAndroid();
#else
  AssetsSDL2 assets = AssetsSDL2();
#endif
#endif

  TrackerApp app = TrackerApp(gfx, fontManager, audioDevice, fileSystem, assets, inputUtils);

  app.setup();
  mainLoop.run(app);
  mainLoop.quit();
  app.teardown();

  return 0;
}

extern "C" int SDL_main(int argv, char** args) {
  return main(argv, args);
}
