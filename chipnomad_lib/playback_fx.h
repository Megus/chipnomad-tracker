#ifndef __PLAYBACK_FX_H__
#define __PLAYBACK_FX_H__

#ifdef __cplusplus
extern "C" {
#endif

enum PlaybackArpType {
  arpTypeUp,
  arpTypeDown,
  arpTypeUpDown,
  arpTypeUp1Oct,
  arpTypeDown1Oct,
  arpTypeUpDown1Oct,
  arpTypeUp2Oct,
  arpTypeDown2Oct,
  arpTypeUpDown2Oct,
  arpTypeUp3Oct,
  arpTypeDown3Oct,
  arpTypeUpDown3Oct,
  arpTypeUp4Oct,
  arpTypeDown4Oct,
  arpTypeUpDown4Oct,
  arpTypeUp5Oct,
  arpTypeMax,
};

struct PlaybackFXData_Bend {
  int speed;
};

struct PlaybackFXData_Slide {
  int16_t startPeriod;
  int16_t endPeriod;
};

struct PlaybackFXData_Arpeggio {
  int speed;
  enum PlaybackArpType type;
};

struct PlaybackFXData_Retrigger {
  PhraseRow row;
  int counter;
};

struct PlaybackFXState {
  uint8_t isOn;
  uint8_t fxValue;
  int counter;
  int acc;
  union {
    PlaybackFXData_Bend bend;
    PlaybackFXData_Slide slide;
    PlaybackFXData_Arpeggio arpeggio;
    PlaybackFXData_Retrigger retrigger;
  } d;
};


#ifdef __cplusplus
}
#endif

#endif
