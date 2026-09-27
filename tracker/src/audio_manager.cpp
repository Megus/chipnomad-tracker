#include <stdio.h>
#include "audio_manager.h"
#include "audio_device.h"

#include "chipnomad_lib.h"
#include "file_system.h"
#include "audio_source.h"
#include "wav_file.h"
#include "wavetable_source.h"

AudioManager::AudioManager(AudioDevice& audioDevice, int sampleRate, int audioBufferSize, chipnomad::ChipFactory factory):
  device(audioDevice),
  sampleRate(sampleRate),
  bufferSize(bufferSize),
  engine(chipnomad::Engine(factory, sampleRate)),
  pendingReinitChips(0),
  previewSource(nullptr),
  previewPosition(0.0),
  previewRateRatio(1.0),
  previewBufPos(0),
  previewBufCount(0) {

  renderBuffer = (float*)malloc(bufferSize * 2 * sizeof(float));
  device.setup(this, sampleRate, bufferSize);

  // Initialize track states
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) trackStates[i] = TrackState::normal;
  updatePlaybackMuteFlags();
}

AudioManager::~AudioManager() {
  device.teardown();
  stopPreview();
  free(renderBuffer);
}

void AudioManager::pause(void) {
  device.pause(1);
}

void AudioManager::resume(void) {
  updatePlaybackMuteFlags();
  device.pause(0);
}

void AudioManager::updatePlaybackMuteFlags(void) {
  // Check if any tracks are solo
  int hasSolo = 0;
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (trackStates[i] == TrackState::solo) {
      hasSolo = 1;
      break;
    }
  }

  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (hasSolo) {
      // Solo mode: only solo tracks are enabled
      engine.player.trackEnabled[i] = (trackStates[i] == TrackState::solo) ? 1 : 0;
    } else {
      // Mute mode: muted tracks are disabled, others enabled
      engine.player.trackEnabled[i] = (trackStates[i] == TrackState::muted) ? 0 : 1;
    }
  }
}

void AudioManager::toggleTrackMute(int trackIdx) {
  // Clear all solos when switching to mute mode
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (trackStates[i] == TrackState::solo) trackStates[i] = TrackState::normal;
  }

  trackStates[trackIdx] = (trackStates[trackIdx] == TrackState::muted) ? TrackState::normal : TrackState::muted;

  updatePlaybackMuteFlags();
}

void AudioManager::toggleTrackSolo(int trackIdx) {
  // Clear all mutes when switching to solo mode
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (trackStates[i] == TrackState::muted) trackStates[i] = TrackState::normal;
  }

  trackStates[trackIdx] = (trackStates[trackIdx] == TrackState::solo) ? TrackState::normal : TrackState::solo;

  updatePlaybackMuteFlags();
}

void AudioManager::reinitChips() {
  pendingReinitChips = 1; // Request chip reinitialization in the next audio callback
}

void AudioManager::startPreview(AudioSource* source) {
  stopPreview();

  previewSource = source;
  previewPosition = 0.0;
  previewRateRatio = (float)source->getSampleRate() / (float)sampleRate;
  previewBufPos = 0;
  previewBufCount = 0;
}

void AudioManager::stopPreview() {
  if (previewSource) {
    delete previewSource;
    previewSource = NULL;
  }
}

int AudioManager::startWavPreview(const char* path) {
  WavFile* wav = new WavFile(path);
  if (wav->getResult() != WAV_OK) {
    delete wav;
    return 0;
  }
  startPreview(wav);
  return 1;
}

int AudioManager::startWavetablePreview(const char* path, bool isYM) {
  WavetableSource* wt = new WavetableSource(path, isYM);
  if (!wt->isValid()) {
    delete wt;
    return 0;
  }
  startPreview(wt);
  return 1;
}

// Audio callback function called by the audio system
void AudioManager::onAudioOutput(int16_t* buffer, int stereoSamples) {
  // ChipNomad rendering:

  // Synchronous reinitialization of chips if requested
  if (pendingReinitChips) {
    engine.initChips();
    pendingReinitChips = 0;
  }

  engine.render(renderBuffer, stereoSamples);

  // Preview: resample and mix into render buffer (as float, before final conversion)
  if (previewSource && !previewSource->isFinished()) {
    for (int i = 0; i < stereoSamples; i++) {
      // Refill read buffer if needed
      if (previewBufPos >= previewBufCount) {
        previewBufCount = previewSource->readSamples16(previewBuf, AudioManager::PREVIEW_BUF_SIZE);
        previewBufPos = 0;
        if (previewBufCount == 0) break;
      }

      // TODO: Review this mixing code, the volume logic should go to AudioSource implementations
      // Mix current source sample into both channels (mono -> stereo)
      float previewSample = previewBuf[previewBufPos] * (0.5f / 32768.0f); // Scale down to reduce volume
      renderBuffer[i * 2] += previewSample;
      renderBuffer[i * 2 + 1] += previewSample;

      // Advance fractional position; consume whole source samples in one step
      previewPosition += previewRateRatio;
      int wholeSamples = (int)previewPosition;
      previewBufPos += wholeSamples;
      previewPosition -= wholeSamples;
    }
  }

  // Final conversion: float to int16_t with clamp
  for (int i = 0; i < stereoSamples * 2; i++) {
    int sample = (int)(renderBuffer[i] * 32767.0f);
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    buffer[i] = (int16_t)sample;
  }
}
