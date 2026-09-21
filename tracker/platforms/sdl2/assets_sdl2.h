#ifndef __ASSETS_SDL2_H__
#define __ASSETS_SDL2_H__

#include "assets.h"

class AssetsSDL2 : public Assets {
  public:
    bool copyAssets() override { return true; };
};

#endif // __ASSETS_SDL2_H__
