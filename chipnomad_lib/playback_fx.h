#ifndef __PLAYBACK_FX_H__
#define __PLAYBACK_FX_H__

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

typedef struct PlaybackFXData_PBN {
  uint8_t fxValue;
  int value;
  uint8_t lowByte;
} PlaybackFXData_PBN;

typedef struct PlaybackFXData_CountFX {
  uint8_t fxValue;
  uint8_t counter;
} PlaybackFXData_CountFX;

typedef struct PlaybackFXData_PSL {
  uint8_t fxValue;
  int16_t startPeriod;
  int16_t endPeriod;
  uint8_t counter;
} PlaybackFXData_PSL;

typedef struct PlaybackCommonFXState {
  PlaybackFXData_CountFX fxARP; // Arpeggio
  PlaybackFXData_CountFX fxPVB; // Pitch vibrato
  PlaybackFXData_PBN fxPBN; // Pitch bend
  PlaybackFXData_PSL fxPSL; // Pitch slide (portamento)
  PlaybackFXData_CountFX fxRET; // Retrigger
  PlaybackFXData_CountFX fxDEL; // Delay
  PlaybackFXData_CountFX fxOFF; // Off
  PlaybackFXData_CountFX fxKIL; // Kill note
} PlaybackCommonFXState;

typedef struct PlaybackAYFXState {
  PlaybackFXData_CountFX fxEVB; // Envelope vibrato
  PlaybackFXData_PBN fxEBN; // Envelope bend
  PlaybackFXData_PSL fxESL; // Envelope slide (portamento)
} PlaybackAYFXState;

typedef union PlaybackChipFXState {
  PlaybackAYFXState ay;
} PlaybackChipFXState;

#endif