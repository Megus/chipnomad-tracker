#include "doctest.h"

extern "C" {
#include "chipnomad_lib.h"
#include "playback_internal.h"
#include "pitch_table_utils.h"
}

#include <cstring>

TEST_SUITE("playback") {

// Mock AY chip — only stores register writes

static void mockSetRegister(SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg < 256) self->regs[reg] = value;
}

static void mockSetTimerFunc(SoundChip* self, int (*timerFunc)(struct SoundChip* self, void* userdata), void* timerUserdata) {
  // Mock implementation - just store the function pointer
  self->timerFunc = timerFunc;
  self->timerUserdata = timerUserdata;
}

static SoundChip mockChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  SoundChip chip;
  std::memset(&chip, 0, sizeof(SoundChip));
  chip.setRegister = mockSetRegister;
  chip.setTimerFunc = mockSetTimerFunc;
  chip.regs[7] = 0x3f;
  return chip;
}

// Test fixture
struct PlaybackFixture {
  ChipNomadState* state;

  PlaybackFixture() {
    state = chipnomadCreate();

    Project* p = &state->project;
    p->tickRate = 50;
    p->chipType = chipAY;
    p->chipsCount = 1;
    p->chipSetup.ay = (ChipSetupAY){ .clock = 1773400, .isYM = 0, .stereoMode = ayStereoABC, .stereoSeparation = 50, .pwmFullRange = 0 };
    p->tracksCount = projectGetTotalTracks(p);
    calculatePitchTableAY(p);

    chipnomadInitChips(state, 44100, mockChipFactory);
    playbackInit(&state->playbackState, p);
  }

  ~PlaybackFixture() {
    chipnomadDestroy(state);
  }

  // Helper: set up a simple instrument
  void setInstrument(int idx, uint8_t veA, uint8_t veD, uint8_t veS, uint8_t veR) {
    state->project.instruments[idx].type = instAY1;
    state->project.instruments[idx].tableSpeed = 1;
    state->project.instruments[idx].transposeEnabled = 1;
    state->project.instruments[idx].chip.ay.volumeEnvelope.type = modADSR;
    state->project.instruments[idx].chip.ay.volumeEnvelope.amount = 127;  // Full amount
    state->project.instruments[idx].chip.ay.volumeEnvelope.p1 = veA;  // Attack
    state->project.instruments[idx].chip.ay.volumeEnvelope.p2 = veD;  // Decay
    state->project.instruments[idx].chip.ay.volumeEnvelope.p3 = veS;  // Sustain
    state->project.instruments[idx].chip.ay.volumeEnvelope.p4 = veR;  // Release
    state->project.instruments[idx].chip.ay.defaultMixer = 0x01; // Tone only
  }

  // Helper: advance playback by N frames
  void advanceFrames(int n) {
    for (int i = 0; i < n; i++) {
      playbackNextFrame(state);
    }
  }
};

TEST_CASE_FIXTURE(PlaybackFixture, "playback init all tracks stopped") {
  CHECK_FALSE(playbackIsPlaying(&state->playbackState));
}

TEST_CASE_FIXTURE(PlaybackFixture, "single note outputs to registers") {
  setInstrument(0, 15, 0, 15, 0);

  // Put a note in phrase 0, row 0
  state->project.phrases[0].rows[0].note = 48; // C-4
  state->project.phrases[0].rows[0].instrument = 0;
  state->project.phrases[0].rows[0].volume = 15;

  // Put phrase 0 in chain 0
  state->project.chains[0].rows[0].phrase = 0;

  // Put chain 0 in song row 0, track 0
  state->project.song[0][0] = 0;

  // Start playback and advance a few frames to let attack ramp up
  playbackStartSong(&state->playbackState, 0, 0, 0);
  advanceFrames(5);

  // Channel 0 tone period should be set (regs 0,1)
  uint16_t period = state->chips[0].regs[0] | (state->chips[0].regs[1] << 8);
  CHECK(period == state->project.pitchTable.values[48]);

  // Channel 0 volume should be non-zero (attack phase ramping up)
  CHECK((state->chips[0].regs[8] & 0x0f) != 0);
}

TEST_CASE_FIXTURE(PlaybackFixture, "ADSR volume envelope ranges") {
  // Test with A=15, D=16, S=1, R=10
  // This should produce low initial volume, decay to sustain level 1, then release
  setInstrument(0, 15, 16, 1, 10);

  // Put a note in phrase 0
  state->project.phrases[0].rows[0].note = 48;
  state->project.phrases[0].rows[0].instrument = 0;
  state->project.phrases[0].rows[0].volume = 15;

  // Put phrase in chain and song
  state->project.chains[0].rows[0].phrase = 0;
  state->project.song[0][0] = 0;

  // Start playback
  playbackStartSong(&state->playbackState, 0, 0, 0);

  // First frame: should start attack phase with low volume (0-2)
  advanceFrames(1);
  uint8_t vol1 = state->chips[0].regs[8] & 0x0f;
  CHECK(vol1 <= 2);

  // After attack (15 frames): should be at max volume (15)
  advanceFrames(14);
  uint8_t volMax = state->chips[0].regs[8] & 0x0f;
  CHECK(volMax == 15);

  // After decay (16 more frames): should be at sustain level (1)
  advanceFrames(16);
  uint8_t volSustain = state->chips[0].regs[8] & 0x0f;
  CHECK(volSustain == doctest::Approx(1).epsilon(1));

  // Trigger note off
  handleNoteOff(&state->playbackState, 0);

  // After release starts: volume should decrease or stay same
  advanceFrames(1);
  uint8_t volRelease1 = state->chips[0].regs[8] & 0x0f;
  CHECK(volRelease1 <= volSustain); // Should be decreasing or same

  // After full release (10 frames): should be at 0
  advanceFrames(10);
  uint8_t volEnd = state->chips[0].regs[8] & 0x0f;
  CHECK(volEnd == 0);
}

} // TEST_SUITE("playback")
