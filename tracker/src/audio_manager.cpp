#include <stdio.h>
#include "audio_manager.h"
#include "corelib_audio.h"
#include "oscilloscope.h"

#include "chipnomad_lib.h"
#include "corelib_file.h"

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

  chipnomadRenderTracks(self->chipnomadState, self->renderBuffer, stereoSamples, self->trackRenderBuffer, self->chipnomadState->project.tracksCount);

  // Feed per-track samples to the oscilloscope visualization
  oscilloscopePushSamples(self->trackRenderBuffer, self->chipnomadState->project.tracksCount, stereoSamples);

  // Convert float to int16_t
  for (int i = 0; i < stereoSamples * 2; i++) {
    int sample = self->renderBuffer[i] * 32767;
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    buffer[i] = sample;
  }

  // WAV player rendering:

}


AudioManager::AudioManager(ChipNomadState *state) {
  chipnomadState = state;
  pendingReinitChips = 0;

  // Initialize track states
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    trackStates[i] = TrackState::normal;
  }
  if (chipnomadState != nullptr) updatePlaybackMuteFlags();
}

AudioManager::~AudioManager() {
  stop();
  free(renderBuffer);
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    free(trackRenderBuffer[i]);
  }
  self = NULL; // Clear the static pointer
}

int AudioManager::start(int sampleRate, int bufferSize) {
  self = this; // Set the static pointer to this instance
  this->sampleRate = sampleRate;
  this->bufferSize = bufferSize;
  renderBuffer = (float*)malloc(bufferSize * 2 * sizeof(float));
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    trackRenderBuffer[i] = (float*)malloc(bufferSize * sizeof(float));
  }

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
  return 0;
}

void AudioManager::stopWavPreview() {

}

// Singleton instance of AudioManager
AudioManager& audio = *new AudioManager(nullptr);
