#ifndef __ASSETS_ANDROID_H__
#define __ASSETS_ANDROID_H__

#include "assets.h"

// AssetsAndroid: Android assets implementation. Copies bundled assets from the
// APK's AAssetManager into the app's documents directory on first launch.
class AssetsAndroid : public Assets {
  public:
    bool copyAssets() override;
};

#endif // __ASSETS_ANDROID_H__
