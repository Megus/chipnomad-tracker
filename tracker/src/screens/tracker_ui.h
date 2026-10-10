#ifndef __SCREENS_H__
#define __SCREENS_H__

#include "chipnomad_lib.h"
#include "events.h"
#include "tracker_state.h"

#define MESSAGE_TIME (60)

/*
extern const AppScreen screenProject;
extern const AppScreen screenProjectLoad;
extern const AppScreen screenProjectSave;
extern const AppScreen screenConfirm;
extern const AppScreen screenPitchTable;
extern const AppScreen screenFileBrowser;
extern const AppScreen screenCreateFolder;
extern const AppScreen screenEnterName;
extern const AppScreen screenSong;
extern const AppScreen screenChain;
extern const AppScreen screenPhrase;
extern const AppScreen screenGroove;
extern const AppScreen screenInstrument;
extern const AppScreen screenInstrumentPool;
extern const AppScreen screenModulation;
extern const AppScreen screenTable;
extern const AppScreen screenWavetable;
extern const AppScreen screenExport;
extern const AppScreen screenManage;
extern const AppScreen screenSettings;
extern const AppScreen screenColorTheme;
extern const AppScreen screenKeyMapping;

extern const AppScreen* currentScreen;
*/

void screenSetup(const AppScreen* screen, int input);
void screenDraw(void);
void screenMessage(int time, const char* format, ...);
void screensInitAll(void);
void drawScreenMap(void);


// Confirmation dialog. TODO: This should be a proper pop-up screen
//void confirmSetup(const char* message, void (*confirmCallback)(void), void (*cancelCallback)(void));

// Character edit. TODO: This should be a proper pop-up screen
/*int editCharacter(CellEditAction action, char* str, int idx, int maxLen);
char charEditInput(int keys, int tapCount, char* str, int idx, int maxLen);*/

// FX edit. TODO: This should be a proper pop-up screen
/*int editFX(CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable, uint8_t instrumentIdx);
int editFXValue(CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable, uint8_t instrumentIdx);
int fxEditInput(int keys, int tapCount, uint8_t* fx, uint8_t* lastFX);
void fxEditFullDraw(uint8_t currentFX, uint8_t instrumentIdx);*/

class TrackerUI : EventHandler {
  public:
    TrackerUI();

    bool onEvent(Event event) override;

  protected:
    void drawScreenMap();


};

#endif // __SCREENS_H__
