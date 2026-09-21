#ifndef __ASSETS_SDL12_H__
#define __ASSETS_SDL12_H__

#include "assets.h"

class AssetsSDL12 : public Assets {
  public:
    bool copyAssets() override { return true; };
};

#endif // __ASSETS_SDL12_H__
