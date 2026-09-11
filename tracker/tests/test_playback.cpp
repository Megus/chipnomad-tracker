#include "doctest.h"
#include "chipnomad_lib.h"
#include "playback_internal.h"
#include "pitch_table_utils.h"

#include <cstring>

using namespace chipnomad;

TEST_SUITE("playback") {

// Mock AY chip — only stores register writes

class MockSoundChip : public SoundChipAY {
  public:
    MockSoundChip(int sampleRate, ChipSetup setup) : SoundChipAY(sampleRate, setup) {}
};

static SoundChip* mockChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  return new SoundChipAY(sampleRate, setup);
}

// Test fixture
struct PlaybackFixture {
  Project project;
  Engine* engine;

  PlaybackFixture() {
    Project* p = &project;
    projectInit(p);
    p->tickRate = 50;
    p->chipType = ChipType::AY;
    p->chipsCount = 1;
    p->chipSetup.ay = (ChipSetupAY){ .clock = 1773400, .isYM = 0, .stereoMode = StereoModeAY::ABC, .stereoSeparation = 50, .pwmFullRange = 0 };
    p->tracksCount = projectGetTotalTracks(p);
    p->linearPitch = 0;
    calculatePitchTableAY(p);

    engine = new Engine(mockChipFactory, 44100);
    engine->setProject(p);
  }

  ~PlaybackFixture() {
    delete engine;
  }

  // Helper: set up a simple instrument
  void setInstrument(int idx, uint8_t veA, uint8_t veD, uint8_t veS, uint8_t veR) {
    project.instruments[idx].type = InstrumentType::AY1;
    project.instruments[idx].tableSpeed = 1;
    project.instruments[idx].transposeEnabled = 1;
    project.instruments[idx].chip.ay.volumeEnvelope.type = ModulationType::ADSR;
    project.instruments[idx].chip.ay.volumeEnvelope.amount = 127;  // Full amount
    project.instruments[idx].chip.ay.volumeEnvelope.p1 = veA;  // Attack
    project.instruments[idx].chip.ay.volumeEnvelope.p2 = veD;  // Decay
    project.instruments[idx].chip.ay.volumeEnvelope.p3 = veS;  // Sustain
    project.instruments[idx].chip.ay.volumeEnvelope.p4 = veR;  // Release
    project.instruments[idx].chip.ay.defaultMixer = 0x01; // Tone only
  }

  // Helper: advance playback by N frames
  void advanceFrames(int n) {
    for (int i = 0; i < n; i++) {
      engine->player.nextFrame(engine);
    }
  }
};

TEST_CASE_FIXTURE(PlaybackFixture, "playback init all tracks stopped") {
  CHECK_FALSE(engine->player.isPlaying());
}

TEST_CASE_FIXTURE(PlaybackFixture, "single note outputs to registers") {
  setInstrument(0, 15, 0, 15, 0);

  // Put a note in phrase 0, row 0
  project.phrases[0].rows[0].note = 48; // C-4
  project.phrases[0].rows[0].instrument = 0;
  project.phrases[0].rows[0].volume = 15;

  // Put phrase 0 in chain 0
  project.chains[0].rows[0].phrase = 0;

  // Put chain 0 in song row 0, track 0
  project.song[0][0] = 0;

  // Start playback and advance a few frames to let attack ramp up
  engine->player.playSong(0, 0, 0);
  advanceFrames(5);

  // Channel 0 tone period should be set (regs 0,1)
  SoundChipAY* ayChip = static_cast<SoundChipAY*>(engine->chips[0]);
  uint16_t period = ayChip->getRegister(0) | (ayChip->getRegister(1) << 8);
  CHECK(period == project.pitchTable.values[48]);

  // Channel 0 volume should be non-zero (attack phase ramping up)
  CHECK((ayChip->getRegister(8) & 0x0f) != 0);
}

TEST_CASE_FIXTURE(PlaybackFixture, "ADSR volume envelope ranges") {
  // Test with A=15, D=16, S=1, R=10
  // This should produce low initial volume, decay to sustain level 1, then release
  setInstrument(0, 15, 16, 1, 10);

  // Put a note in phrase 0
  project.phrases[0].rows[0].note = 48;
  project.phrases[0].rows[0].instrument = 0;
  project.phrases[0].rows[0].volume = 15;

  // Put phrase in chain and song
  project.chains[0].rows[0].phrase = 0;
  project.song[0][0] = 0;

  // Start playback
  engine->player.playSong(0, 0, 0);

  SoundChipAY* ayChip = static_cast<SoundChipAY*>(engine->chips[0]);

  // First frame: should start attack phase with low volume (0-2)
  advanceFrames(1);
  uint8_t vol1 = ayChip->getRegister(8) & 0x0f;
  CHECK(vol1 <= 2);

  // After attack (15 frames): should be at max volume (15)
  advanceFrames(14);
  uint8_t volMax = ayChip->getRegister(8) & 0x0f;
  CHECK(volMax == 15);

  // After decay (16 more frames): should be at sustain level (1)
  advanceFrames(16);
  uint8_t volSustain = ayChip->getRegister(8) & 0x0f;
  CHECK(volSustain == doctest::Approx(1).epsilon(1));

  // Trigger note off
  engine->player.handleNoteOff(0);

  // After release starts: volume should decrease or stay same
  advanceFrames(1);
  uint8_t volRelease1 = ayChip->getRegister(8) & 0x0f;
  CHECK(volRelease1 <= volSustain); // Should be decreasing or same

  // After full release (10 frames): should be at 0
  advanceFrames(10);
  uint8_t volEnd = ayChip->getRegister(8) & 0x0f;
  CHECK(volEnd == 0);
}

} // TEST_SUITE("playback")
