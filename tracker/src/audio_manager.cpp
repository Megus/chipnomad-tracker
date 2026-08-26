#include <stdio.h>
#include "audio_manager.h"
#include "corelib_audio.h"

#include "chipnomad_lib.h"
#include "corelib_file.h"
#include "import/wav_file.h"

static AudioManager *self = NULL; // Used to point to the current AudioManager instance for callback function

// Audio callback function called by the audio system
void audioCallback(int16_t* buffer, int stereoSamples) {
  if (self == NULL) return;

  // ChipNomad rendering:

  // Synchronous reinitialization of chips if requested
  if (self->pendingReinitChips) {
    chipnomadInitChips(self->chipnomadState, self->sampleRate, NULL);
    self->pendingReinitChips = 0;
  }

  chipnomadRender(self->chipnomadState, self->renderBuffer, stereoSamples);

  // WAV preview: resample and mix into render buffer (as float, before final conversion)
  if (self->wavPreview && !self->wavPreview->isFinished()) {
    for (int i = 0; i < stereoSamples; i++) {
      // Refill read buffer if needed
      if (self->wavPreviewBufPos >= self->wavPreviewBufCount) {
        self->wavPreviewBufCount = self->wavPreview->readSamples16(
          self->wavPreviewBuf, AudioManager::WAV_PREVIEW_BUF_SIZE);
        self->wavPreviewBufPos = 0;
        if (self->wavPreviewBufCount == 0) break;
      }

      // Mix current source sample into both channels (mono -> stereo)
      float wavSample = self->wavPreviewBuf[self->wavPreviewBufPos] * (0.5f / 32768.0f); // Scale down to reduce volume
      self->renderBuffer[i * 2] += wavSample;
      self->renderBuffer[i * 2 + 1] += wavSample;

      // Advance fractional position and consume source samples
      self->wavPreviewPosition += self->wavPreviewRateRatio;
      while (self->wavPreviewPosition >= 1.0) {
        self->wavPreviewPosition -= 1.0;
        self->wavPreviewBufPos++;
      }
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


AudioManager::AudioManager(ChipNomadState *state) {
  chipnomadState = state;
  pendingReinitChips = 0;
  wavPreview = NULL;
  wavPreviewPosition = 0.0;
  wavPreviewRateRatio = 1.0;
  wavPreviewBufPos = 0;
  wavPreviewBufCount = 0;

  // Initialize track states
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    trackStates[i] = TrackState::normal;
  }
  if (chipnomadState != nullptr) updatePlaybackMuteFlags();
}

AudioManager::~AudioManager() {
  stop();
  stopWavPreview();
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
      chipnomadState->playbackState.trackEnabled[i] = (trackStates[i] == TrackState::solo) ? 1 : 0;
    } else {
      // Mute mode: muted tracks are disabled, others enabled
      chipnomadState->playbackState.trackEnabled[i] = (trackStates[i] == TrackState::muted) ? 0 : 1;
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

int AudioManager::startWavPreview(const char* path) {
  stopWavPreview();

  WavFile* wav = new WavFile(path);
  if (wav->getResult() != WAV_OK) {
    delete wav;
    return 0;
  }

  wavPreview = wav;
  wavPreviewPosition = 0.0;
  wavPreviewRateRatio = (double)wav->getSampleRate() / (double)sampleRate;
  wavPreviewBufPos = 0;
  wavPreviewBufCount = 0;
  return 1;
}

void AudioManager::stopWavPreview() {
  if (wavPreview) {
    delete wavPreview;
    wavPreview = NULL;
  }
}

// Singleton instance of AudioManager
AudioManager& audio = *new AudioManager(nullptr);
