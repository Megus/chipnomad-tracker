#include <stdio.h>
#include "audio_manager.h"
#include "corelib_audio.h"

#include "chipnomad_lib.h"
#include "corelib_file.h"
#include "audio_source.h"
#include "import/wav_file.h"
#include "import/wavetable_source.h"

static AudioManager *self = NULL; // Used to point to the current AudioManager instance for callback function

// Audio callback function called by the audio system
void audioCallback(int16_t* buffer, int stereoSamples) {
  if (self == NULL) return;

  // ChipNomad rendering:

  // Synchronous reinitialization of chips if requested
  if (self->pendingReinitChips) {
    self->trackerState->engine->initChips();
    self->pendingReinitChips = 0;
  }

  self->trackerState->engine->render(self->renderBuffer, stereoSamples);

  // Preview: resample and mix into render buffer (as float, before final conversion)
  if (self->previewSource && !self->previewSource->isFinished()) {
    for (int i = 0; i < stereoSamples; i++) {
      // Refill read buffer if needed
      if (self->previewBufPos >= self->previewBufCount) {
        self->previewBufCount = self->previewSource->readSamples16(
          self->previewBuf, AudioManager::PREVIEW_BUF_SIZE);
        self->previewBufPos = 0;
        if (self->previewBufCount == 0) break;
      }

      // Mix current source sample into both channels (mono -> stereo)
      float previewSample = self->previewBuf[self->previewBufPos] * (0.5f / 32768.0f); // Scale down to reduce volume
      self->renderBuffer[i * 2] += previewSample;
      self->renderBuffer[i * 2 + 1] += previewSample;

      // Advance fractional position; consume whole source samples in one step
      self->previewPosition += self->previewRateRatio;
      int wholeSamples = (int)self->previewPosition;
      self->previewBufPos += wholeSamples;
      self->previewPosition -= wholeSamples;
    }
  }

  // Final conversion: float to int16_t with clamp
  for (int i = 0; i < stereoSamples * 2; i++) {
    int sample = (int)(self->renderBuffer[i] * 32767.0f);
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    buffer[i] = (int16_t)sample;
  }
}


AudioManager::AudioManager(TrackerState *state) {
  trackerState = state;
  pendingReinitChips = 0;
  previewSource = NULL;
  previewPosition = 0.0;
  previewRateRatio = 1.0;
  previewBufPos = 0;
  previewBufCount = 0;

  // Initialize track states
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    trackStates[i] = TrackState::normal;
  }
  if (trackerState != nullptr && trackerState->engine != nullptr) updatePlaybackMuteFlags();
}

AudioManager::~AudioManager() {
  stop();
  stopPreview();
  free(renderBuffer);
  self = NULL; // Clear the static pointer
}

int AudioManager::start(int sampleRate, int bufferSize) {
  self = this; // Set the static pointer to this instance
  this->sampleRate = sampleRate;
  this->bufferSize = bufferSize;
  renderBuffer = (float*)malloc(bufferSize * 2 * sizeof(float));

  audioSetup(audioCallback, sampleRate, bufferSize);

  // Initialize track states
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    trackStates[i] = TrackState::normal;
  }
  updatePlaybackMuteFlags();

  return 1;
}

void AudioManager::pause(void) {
  audioPause(1);
}

void AudioManager::resume(void) {
  updatePlaybackMuteFlags();
  audioPause(0);
}

void AudioManager::stop() {
  audioCleanup();
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
      trackerState->engine->player.trackEnabled[i] = (trackStates[i] == TrackState::solo) ? 1 : 0;
    } else {
      // Mute mode: muted tracks are disabled, others enabled
      trackerState->engine->player.trackEnabled[i] = (trackStates[i] == TrackState::muted) ? 0 : 1;
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
  previewRateRatio = (double)source->getSampleRate() / (double)sampleRate;
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

// Singleton instance of AudioManager
AudioManager& audio = *new AudioManager(nullptr);
