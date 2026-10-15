#ifndef __APP_H__
#define __APP_H__

#include "mainloop.h"
#include "gfx.h"
#include "font_manager.h"
#include "audio_device.h"
#include "file_system.h"
#include "assets.h"
#include "audio_manager.h"
#include "events.h"
#include "tracker_state.h"
#include "tracker_ui.h"

class TrackerApp: public App, public EventDispatcher {
  public:
    TrackerApp(Gfx& gfx, FontManager& fontManager, AudioDevice& audio, FileSystem& file, Assets& assets, InputUtils& inputUtils);
    ~TrackerApp() = default;

    bool setup() override;
    void teardown() override;
    void draw() override;
    void onMainLoopEvent(MainLoopEvent event) override;
    void dispatch(Event e) override;

  protected:
    static constexpr int eventQueueMaxLength = 16; // Max number of events in the queue

    Gfx& gfx;
    FontManager& fontManager;
    AudioDevice& audioDevice;
    FileSystem& file;
    Assets& assets;
    InputUtils& input;

    TrackerState state;
    AudioManager* audio;
    TrackerUI *ui;

    int pressedButtons = 0; // Currently pressed buttons
    int tapTimerCount = 0; // Frame counter for tap detection
    int tapButton = 0; // Button that triggered tap timer
    int tapCount = 0 ; // Number of taps detected
    int keyRepeatCount = 0; // Frame counter for key repeats

    int autosaveCounter = 0; // Autosave period frame counter

    Event eventQueue[eventQueueMaxLength];
    int eventQueueLength = 0;
};

#endif
