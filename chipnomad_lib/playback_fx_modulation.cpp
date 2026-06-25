#include "playback.h"
#include "playback_internal.h"

// Modulation 1 Amount
static void initFX_M1A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M1A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M1A should keep the accumulated offset
}

static void handleFX_M1A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[0].amountOffset += fx->acc;
}

// Modulation 1 Parameter 1
static void initFX_M11(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M11(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M11 should keep the accumulated offset
}

static void handleFX_M11(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[0].p1Offset += fx->acc;
}

// Modulation 1 Parameter 2
static void initFX_M12(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M12(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M12 should keep the accumulated offset
}

static void handleFX_M12(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[0].p2Offset += fx->acc;
}

// Modulation 1 Parameter 3
static void initFX_M13(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M13(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M13 should keep the accumulated offset
}

static void handleFX_M13(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[0].p3Offset += fx->acc;
}

// Modulation 1 Parameter 4
static void initFX_M14(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M14(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M14 should keep the accumulated offset
}

static void handleFX_M14(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[0].p4Offset += fx->acc;
}

// Modulation 2 Amount
static void initFX_M2A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M2A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M2A should keep the accumulated offset
}

static void handleFX_M2A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[1].amountOffset += fx->acc;
}

// Modulation 2 Parameter 1
static void initFX_M21(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M21(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M21 should keep the accumulated offset
}

static void handleFX_M21(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[1].p1Offset += fx->acc;
}

// Modulation 2 Parameter 2
static void initFX_M22(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M22(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M22 should keep the accumulated offset
}

static void handleFX_M22(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[1].p2Offset += fx->acc;
}

// Modulation 2 Parameter 3
static void initFX_M23(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M23(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M23 should keep the accumulated offset
}

static void handleFX_M23(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[1].p3Offset += fx->acc;
}

// Modulation 2 Parameter 4
static void initFX_M24(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M24(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M24 should keep the accumulated offset
}

static void handleFX_M24(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[1].p4Offset += fx->acc;
}

// Modulation 3 Amount
static void initFX_M3A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M3A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M3A should keep the accumulated offset
}

static void handleFX_M3A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[2].amountOffset += fx->acc;
}

// Modulation 3 Parameter 1
static void initFX_M31(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M31(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M31 should keep the accumulated offset
}

static void handleFX_M31(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[2].p1Offset += fx->acc;
}

// Modulation 3 Parameter 2
static void initFX_M32(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M32(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M32 should keep the accumulated offset
}

static void handleFX_M32(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[2].p2Offset += fx->acc;
}

// Modulation 3 Parameter 3
static void initFX_M33(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M33(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M33 should keep the accumulated offset
}

static void handleFX_M33(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[2].p3Offset += fx->acc;
}

// Modulation 3 Parameter 4
static void initFX_M34(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M34(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M34 should keep the accumulated offset
}

static void handleFX_M34(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[2].p4Offset += fx->acc;
}

// Modulation 4 Amount
static void initFX_M4A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M4A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M4A should keep the accumulated offset
}

static void handleFX_M4A(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[3].amountOffset += fx->acc;
}

// Modulation 4 Parameter 1
static void initFX_M41(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M41(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M41 should keep the accumulated offset
}

static void handleFX_M41(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[3].p1Offset += fx->acc;
}

// Modulation 4 Parameter 2
static void initFX_M42(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M42(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M42 should keep the accumulated offset
}

static void handleFX_M42(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[3].p2Offset += fx->acc;
}

// Modulation 4 Parameter 3
static void initFX_M43(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M43(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M43 should keep the accumulated offset
}

static void handleFX_M43(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[3].p3Offset += fx->acc;
}

// Modulation 4 Parameter 4
static void initFX_M44(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_M44(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - M44 should keep the accumulated offset
}

static void handleFX_M44(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.modulation[3].p4Offset += fx->acc;
}

void registerFXHandlers_Modulation(void) {
  // Modulation 1
  fxHandlers[fxM1A] = (PlaybackFXHandler){initFX_M1A, handleFX_M1A, restartFX_M1A};
  fxHandlers[fxM11] = (PlaybackFXHandler){initFX_M11, handleFX_M11, restartFX_M11};
  fxHandlers[fxM12] = (PlaybackFXHandler){initFX_M12, handleFX_M12, restartFX_M12};
  fxHandlers[fxM13] = (PlaybackFXHandler){initFX_M13, handleFX_M13, restartFX_M13};
  fxHandlers[fxM14] = (PlaybackFXHandler){initFX_M14, handleFX_M14, restartFX_M14};

  // Modulation 2
  fxHandlers[fxM2A] = (PlaybackFXHandler){initFX_M2A, handleFX_M2A, restartFX_M2A};
  fxHandlers[fxM21] = (PlaybackFXHandler){initFX_M21, handleFX_M21, restartFX_M21};
  fxHandlers[fxM22] = (PlaybackFXHandler){initFX_M22, handleFX_M22, restartFX_M22};
  fxHandlers[fxM23] = (PlaybackFXHandler){initFX_M23, handleFX_M23, restartFX_M23};
  fxHandlers[fxM24] = (PlaybackFXHandler){initFX_M24, handleFX_M24, restartFX_M24};

  // Modulation 3
  fxHandlers[fxM3A] = (PlaybackFXHandler){initFX_M3A, handleFX_M3A, restartFX_M3A};
  fxHandlers[fxM31] = (PlaybackFXHandler){initFX_M31, handleFX_M31, restartFX_M31};
  fxHandlers[fxM32] = (PlaybackFXHandler){initFX_M32, handleFX_M32, restartFX_M32};
  fxHandlers[fxM33] = (PlaybackFXHandler){initFX_M33, handleFX_M33, restartFX_M33};
  fxHandlers[fxM34] = (PlaybackFXHandler){initFX_M34, handleFX_M34, restartFX_M34};

  // Modulation 4
  fxHandlers[fxM4A] = (PlaybackFXHandler){initFX_M4A, handleFX_M4A, restartFX_M4A};
  fxHandlers[fxM41] = (PlaybackFXHandler){initFX_M41, handleFX_M41, restartFX_M41};
  fxHandlers[fxM42] = (PlaybackFXHandler){initFX_M42, handleFX_M42, restartFX_M42};
  fxHandlers[fxM43] = (PlaybackFXHandler){initFX_M43, handleFX_M43, restartFX_M43};
  fxHandlers[fxM44] = (PlaybackFXHandler){initFX_M44, handleFX_M44, restartFX_M44};
}
