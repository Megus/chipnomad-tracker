#include "chipnomad_lib.h"
#include "playback.h"
#include <stdlib.h>
#include <string.h>

namespace chipnomad {
  static void detectAYPitchConflicts(ChipNomadState* state);

  static SoundChip* defaultChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
    return new SoundChipAY(sampleRate, setup);
  }

  Player::Player(ChipFactory factory, int sampleRate) {
    chipFactory = factory ? factory : defaultChipFactory;
    project = nullptr;
    memset(&playbackState, 0, sizeof(PlaybackState));
    memset(chips, 0, sizeof(chips));
    memset(trackWarnings, 0, sizeof(trackWarnings));

    this->sampleRate = sampleRate;
    frameSampleCounter = 0.0f;
    mixVolume = 1.0f;
    aySampleDithering = 1; // Default: ON
    audioOverload = 0;

    // Initialize mix buffer
    mixBufferSize = 8192;
    mixBuffer = (float*)malloc(mixBufferSize * sizeof(float));

    fillFXNames();
  }

  Player::~Player() {
    // Cleanup chips
    for (int i = 0; i < PROJECT_MAX_CHIPS; i++) {
      if (chips[i]) {
        delete chips[i];
        chips[i] = nullptr;
      }
    }

    // Cleanup mix buffer
    free(mixBuffer);
  }

  void Player::initChips() {
    // Cleanup existing chips if already initialized
    for (int i = 0; i < PROJECT_MAX_CHIPS; i++) {
      if (chips[i]) {
        delete chips[i];
        chips[i] = nullptr;
      }
    }

    // Zero the entire chips array for safety
    memset(chips, 0, sizeof(chips));

    // Initialize chips based on project's chipsCount
    for (int i = 0; i < project->chipsCount; i++) {
      chips[i] = chipFactory(i, sampleRate, project->chipSetup);
    }
  }

  int Player::render(float* buffer, int samples) {
    int samplesLeft = samples;
    int allTracksStopped = 0;

    while (samplesLeft > 0 && !allTracksStopped) {
      if ((int)frameSampleCounter == 0) {
        frameSampleCounter += sampleRate / project->tickRate;
        allTracksStopped = nextFrame();
        // Decrease audio overload cooldown each frame
        if (audioOverload > 0) {
          audioOverload--;
        }
        // Detect AY pitch conflicts each frame
        detectAYPitchConflicts();
      }

      if (allTracksStopped) break;

      int samplesToRender = ((int)frameSampleCounter < samplesLeft) ?
      (int)frameSampleCounter : samplesLeft;
      int bufferOffset = (samples - samplesLeft) * 2;

      // Clear buffer section
      for (int i = 0; i < samplesToRender * 2; i++) {
        buffer[bufferOffset + i] = 0.0f;
      }

      // Ensure mix buffer is large enough
      int requiredSize = samplesToRender * 2;
      if (requiredSize > mixBufferSize) {
        mixBufferSize = requiredSize;
        mixBuffer = (float*)realloc(mixBuffer, mixBufferSize * sizeof(float));
        if (!mixBuffer) return 0; // Out of memory
      }

      // Mix all chips
      for (int chipIdx = 0; chipIdx < project->chipsCount; chipIdx++) {
        SoundChip* chip = chips[chipIdx];
        if (chip) {
          // Render chip to mix buffer
          chip->render(mixBuffer, samplesToRender);

          // Mix into main buffer
          for (int i = 0; i < samplesToRender * 2; i++) {
            buffer[bufferOffset + i] += mixBuffer[i];
          }
        }
      }

      // Apply mix volume and detect overload
      for (int i = 0; i < samplesToRender * 2; i++) {
        buffer[bufferOffset + i] *= mixVolume;
        // Check for audio overload (values beyond -1.0 to 1.0 range)
        if (buffer[bufferOffset + i] > 1.0f || buffer[bufferOffset + i] < -1.0f) {
          audioOverload = AUDIO_OVERLOAD_COOLDOWN_FRAMES;
        }
      }

      samplesLeft -= samplesToRender;
      frameSampleCounter -= (float)samplesToRender;
    }

    // Fill remaining buffer with silence if playback stopped early
    if (samplesLeft > 0) {
      int bufferOffset = (samples - samplesLeft) * 2;
      for (int i = 0; i < samplesLeft * 2; i++) {
        buffer[bufferOffset + i] = 0.0f;
      }
    }

    return samples - samplesLeft;
  }

  void Player::detectAYPitchConflicts() {
    if (project->chipType != ChipType::AY) return;

    // Decrease existing warning cooldowns
    for (int i = 0; i < project->tracksCount; i++) {
      if (trackWarnings[i] > 0) {
        trackWarnings[i]--;
      }
    }

    // Collect tone periods for all tracks (0xFFFF = not using tone)
    uint16_t trackPeriods[PROJECT_MAX_TRACKS];
    for (int chipIdx = 0; chipIdx < project->chipsCount; chipIdx++) {
      SoundChipAY* chip = static_cast<SoundChipAY*>(chips[chipIdx]);
      if (!chip) continue;
      int trackOffset = chipIdx * 3;
      uint8_t mixer = chip->getRegister(7);

      for (int i = 0; i < 3; i++) {
        uint16_t period = chip->getRegister(i * 2) | (chip->getRegister(i * 2 + 1) << 8);
        uint8_t volume = chip->getRegister(8 + i);
        int toneEnabled = ((mixer >> i) & 1) == 0;
        int noiseEnabled = ((mixer >> (i + 3)) & 1) == 0;
        int envelopeMode = (volume & 16) != 0;

        // Check if track uses tone generation
        int isPureNoise = !toneEnabled && noiseEnabled && !envelopeMode;
        int isPureEnvelope = !toneEnabled && !noiseEnabled && envelopeMode;
        int isZeroVolume = (volume & 0xf) == 0 && !envelopeMode;
        int usesTone = !(isPureNoise || isPureEnvelope || isZeroVolume);

        trackPeriods[trackOffset + i] = (usesTone && period != 0) ? period : 0xFFFF;
      }
    }

    // Find conflicts by comparing all track periods
    for (int i = 0; i < project->tracksCount; i++) {
      for (int j = i + 1; j < project->tracksCount; j++) {
        if (trackPeriods[i] != 0xFFFF && trackPeriods[i] == trackPeriods[j]) {
          trackWarnings[i] = PITCH_CONFLICT_COOLDOWN_FRAMES;
          trackWarnings[j] = PITCH_CONFLICT_COOLDOWN_FRAMES;
        }
      }
    }
  }

  void Player::setQuality(ChipNomadQuality quality) {
    for (int i = 0; i < PROJECT_MAX_CHIPS; i++) {
      if (chips[i]) {
        chips[i]->setQuality(quality);
      }
    }
  }
};
