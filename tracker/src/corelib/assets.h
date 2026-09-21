#ifndef __ASSETS_H__
#define __ASSETS_H__

// Copy bundled assets to platform-specific file storage (used in mobile builds)
class Assets {
  public:
    virtual ~Assets() = default;

    // Copy bundled assets
    virtual bool copyAssets() = 0;
};

#endif // __ASSETS_H__
