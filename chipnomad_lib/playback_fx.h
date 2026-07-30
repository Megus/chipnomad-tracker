#ifndef __CHIPNOMAD_LIB__PLAYBACK_FX_H__
#define __CHIPNOMAD_LIB__PLAYBACK_FX_H__

enum class PlaybackArpType {
  up,
  down,
  upDown,
  up1Oct,
  down1Oct,
  upDown1Oct,
  up2Oct,
  down2Oct,
  upDown2Oct,
  up3Oct,
  down3Oct,
  upDown3Oct,
  up4Oct,
  down4Oct,
  upDown4Oct,
  up5Oct,
  max,
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
  PlaybackArpType type;
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

#endif // __CHIPNOMAD_LIB__PLAYBACK_FX_H__
