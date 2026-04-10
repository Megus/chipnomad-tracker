#include "unity.h"
#include "chipnomad_lib.h"
#include "playback_internal.h"
#include "pitch_table_utils.h"
#include <string.h>

// Mock AY chip — only stores register writes

static void mockSetRegister(SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg < 256) self->regs[reg] = value;
}

static SoundChip mockChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  SoundChip chip;
  memset(&chip, 0, sizeof(SoundChip));
  chip.setRegister = mockSetRegister;
  chip.regs[7] = 0x3f;
  return chip;
}

// Test state

static ChipNomadState* state;

static void setupAYProject(void) {
  state = chipnomadCreate();

  Project* p = &state->project;
  p->tickRate = 50;
  p->chipType = chipAY;
  p->chipsCount = 1;
  p->chipSetup.ay = (ChipSetupAY){ .clock = 1773400, .isYM = 0, .stereoMode = ayStereoABC, .stereoSeparation = 50 };
  p->tracksCount = projectGetTotalTracks(p);
  calculatePitchTableAY(p);

  chipnomadInitChips(state, 44100, mockChipFactory);
  playbackInit(&state->playbackState, p);
}

void setUp(void) {
  setupAYProject();
}

void tearDown(void) {
  chipnomadDestroy(state);
  state = NULL;
}

// Helper: set up a simple instrument
static void setInstrument(int idx, uint8_t veA, uint8_t veD, uint8_t veS, uint8_t veR) {
  state->project.instruments[idx].type = instAY;
  state->project.instruments[idx].tableSpeed = 1;
  state->project.instruments[idx].transposeEnabled = 1;
  state->project.instruments[idx].chip.ay.veA = veA;
  state->project.instruments[idx].chip.ay.veD = veD;
  state->project.instruments[idx].chip.ay.veS = veS;
  state->project.instruments[idx].chip.ay.veR = veR;
  state->project.instruments[idx].chip.ay.defaultMixer = 0x01; // Tone only
}

// Helper: advance playback by N frames
static void advanceFrames(int n) {
  for (int i = 0; i < n; i++) {
    playbackNextFrame(&state->playbackState, state->chips);
  }
}

// Tests

void test_playback_init_all_tracks_stopped(void) {
  TEST_ASSERT_FALSE(playbackIsPlaying(&state->playbackState));
}

void test_single_note_outputs_to_registers(void) {
  setInstrument(0, 15, 0, 15, 0);

  // Put a note in phrase 0, row 0
  state->project.phrases[0].rows[0].note = 48; // C-4
  state->project.phrases[0].rows[0].instrument = 0;
  state->project.phrases[0].rows[0].volume = 15;

  // Put phrase 0 in chain 0
  state->project.chains[0].rows[0].phrase = 0;

  // Put chain 0 in song row 0, track 0
  state->project.song[0][0] = 0;

  // Start playback and advance one frame
  playbackStartSong(&state->playbackState, 0, 0, 0);
  advanceFrames(1);

  // Channel 0 tone period should be set (regs 0,1)
  uint16_t period = state->chips[0].regs[0] | (state->chips[0].regs[1] << 8);
  TEST_ASSERT_EQUAL(state->project.pitchTable.values[48], period);

  // Channel 0 volume should be non-zero
  TEST_ASSERT_NOT_EQUAL(0, state->chips[0].regs[8] & 0x0f);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_playback_init_all_tracks_stopped);
  RUN_TEST(test_single_note_outputs_to_registers);
  return UNITY_END();
}
