#ifndef __ASSETS_MOCK_H__
#define __ASSETS_MOCK_H__

#include "i_assets.h"

// AssetsMock: IAssets stub for tests. Records init() calls and returns a
// programmable result.
class AssetsMock : public IAssets {
  public:
    int initCalls = 0;
    int initResult = 0;

    int init() override {
      initCalls++;
      return initResult;
    }
};

#endif // __ASSETS_MOCK_H__
