#ifndef __SCREENS_H__
#define __SCREENS_H__

#include "chipnomad_lib.h"
#include "gfx.h"
#include "audio_manager.h"
#include "events.h"
#include "tracker_state.h"
#include "screen.h"

class TrackerUI : public EventHandler {
  public:
    TrackerUI(Gfx& gfx, FontManager& fontManager, AudioManager& audio, FileSystem& file, InputUtils& input, EventDispatcher& dispatcher, TrackerState& state);

    bool onEvent(Event event) override;
    void draw();

  protected:
    static constexpr int screenStackDepth = 3; // Root screen, 1st popup (e.g. file browser), 2nd popup (e.g. yes/no confirmation)

    void drawScreenMap();

    Gfx& gfx;
    FontManager& fontManager;
    AudioManager& audio;
    FileSystem& file;
    InputUtils& input;
    EventDispatcher& dispatcher;
    TrackerState& state;

    Screen* screenStack[screenStackDepth];
    int screenStackIdx = 0;

    int messageTimer = 0;
    char messageBuffer[41];

    Screen* screenSong;

    /*
    Screen* screenProject;
    Screen* screenProjectLoad;
    Screen* screenProjectSave;
    Screen* screenConfirm;
    Screen* screenPitchTable;
    Screen* screenFileBrowser;
    Screen* screenCreateFolder;
    Screen* screenEnterName;
    Screen* screenChain;
    Screen* screenPhrase;
    Screen* screenGroove;
    Screen* screenInstrument;
    Screen* screenInstrumentPool;
    Screen* screenModulation;
    Screen* screenTable;
    Screen* screenWavetable;
    Screen* screenExport;
    Screen* screenManage;
    Screen* screenSettings;
    Screen* screenColorTheme;
    Screen* screenKeyMapping;
    */

};

#endif // __SCREENS_H__
