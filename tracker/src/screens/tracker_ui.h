#ifndef __SCREENS_H__
#define __SCREENS_H__

#include <unordered_map>
#include "chipnomad_lib.h"
#include "gfx.h"
#include "audio_manager.h"
#include "events.h"
#include "tracker_state.h"
#include "screen.h"
#include "waveform_display.h"
#include "clipboard.h"

class TrackerUI : public EventHandler {
  public:
    TrackerUI(Gfx& gfx, FontManager& fontManager, AudioManager& audio, FileSystem& file, InputUtils& input, EventDispatcher& dispatcher, TrackerState& state);
    virtual ~TrackerUI();

    void draw();
    bool onEvent(Event e) override;

  protected:
    static constexpr int screenStackMaxDepth = 3; // Root screen, 1st popup (e.g. file browser), 2nd popup (e.g. yes/no confirmation)
    static constexpr int timedMessageFrames = 60; // Duration of a timed message

    Screen* getCurrentScreen();
    void drawScreenMap();
    void onShowMessage(Event e, bool isTimed);
    bool onInput(Event e);
    bool onPlay(Event e);

    Gfx& gfx;
    FontManager& fontManager;
    AudioManager& audio;
    FileSystem& file;
    InputUtils& input;
    EventDispatcher& events;
    TrackerState& state;
    Clipboard clipboard = Clipboard();

    WaveformDisplay* waveDisplay; // TODO: Maybe this class can be moved to Gfx?

    Screen* screenStack[screenStackMaxDepth];
    int screenStackIdx = -1;

    int messageTimer = 0;
    char messageBuffer[41];

    std::unordered_map<ScreenEnum, Screen*> screens;
};

#endif // __SCREENS_H__
