#ifndef __APP_H__
#define __APP_H__

#include "mainloop.h"
#include "tracker_state.h"

// Forward declarations
class Gfx;
class FontManager;
class AudioDevice;
class FileSystem;
class Assets;
class InputUtils;

class TrackerApp: public App {
  public:
    TrackerApp(Gfx& gfx, FontManager& fontManager, AudioDevice& audio, FileSystem& file, Assets& assets, InputUtils& inputUtils)
      : gfx(gfx), fontManager(fontManager), audioDevice(audio), file(file), assets(assets), input(inputUtils), state() {};
    ~TrackerApp() = default;

    bool setup() override;
    void teardown() override;
    void draw() override;
    void onMainLoopEvent(MainLoopEvent event) override;

  protected:
    Gfx& gfx;
    FontManager& fontManager;
    AudioDevice& audioDevice;
    FileSystem& file;
    Assets& assets;
    InputUtils& input;

    TrackerState state;
    AudioManager* audio;
};

#endif
