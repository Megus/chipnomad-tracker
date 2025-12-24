#include <stdio.h>
#include "audio_manager.h"
#include "corelib_audio.h"

#include "chipnomad_lib.h"

static int aSampleRate;
static int aBufferSize;

static void updatePlaybackMuteFlags(void) {
  // Check if any tracks are solo
  int hasSolo = 0;
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (audioManager.trackStates[i] == TRACK_SOLO) {
      hasSolo = 1;
      break;
    }
  }

  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (hasSolo) {
      // Solo mode: only solo tracks are enabled
      chipnomadState->playbackState.trackEnabled[i] = (audioManager.trackStates[i] == TRACK_SOLO) ? 1 : 0;
    } else {
      // Mute mode: muted tracks are disabled, others enabled
      chipnomadState->playbackState.trackEnabled[i] = (audioManager.trackStates[i] == TRACK_MUTED) ? 0 : 1;
    }
  }
}

static void audioCallback(int16_t* buffer, int stereoSamples) {
  static float* floatBuffer = NULL;
  static int floatBufferSize = 0;

  if (floatBufferSize < stereoSamples * 2) {
    floatBuffer = realloc(floatBuffer, stereoSamples * 2 * sizeof(float));
    floatBufferSize = stereoSamples * 2;
  }

  chipnomadRender(chipnomadState, floatBuffer, stereoSamples);

  // Convert float to int16_t with volume
  for (int i = 0; i < stereoSamples * 2; i++) {
    int sample = floatBuffer[i] * appSettings.volume * 32767;
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    buffer[i] = sample;
  }
}

static int start(int sampleRate, int bufferSize) {
  aSampleRate = sampleRate;
  aBufferSize = bufferSize;

  audioSetup(audioCallback, sampleRate, bufferSize);

  // Initialize track states
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    audioManager.trackStates[i] = TRACK_NORMAL;
  }
  updatePlaybackMuteFlags();

  return 0;
}

static void pause(void) {
  audioPause(1);
}

static void resume(void) {
  audioPause(0);
}

static void stop() {
  audioCleanup();
}

static void toggleTrackMute(int trackIdx) {
  if (trackIdx < 0 || trackIdx >= PROJECT_MAX_TRACKS) return;

  // Clear all solos when switching to mute mode
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (audioManager.trackStates[i] == TRACK_SOLO) {
      audioManager.trackStates[i] = TRACK_NORMAL;
    }
  }

  if (audioManager.trackStates[trackIdx] == TRACK_MUTED) {
    audioManager.trackStates[trackIdx] = TRACK_NORMAL;
  } else {
    audioManager.trackStates[trackIdx] = TRACK_MUTED;
  }
  updatePlaybackMuteFlags();
}

static void toggleTrackSolo(int trackIdx) {
  if (trackIdx < 0 || trackIdx >= PROJECT_MAX_TRACKS) return;

  // Clear all mutes when switching to solo mode
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    if (audioManager.trackStates[i] == TRACK_MUTED) {
      audioManager.trackStates[i] = TRACK_NORMAL;
    }
  }

  if (audioManager.trackStates[trackIdx] == TRACK_SOLO) {
    audioManager.trackStates[trackIdx] = TRACK_NORMAL;
  } else {
    audioManager.trackStates[trackIdx] = TRACK_SOLO;
  }

  updatePlaybackMuteFlags();
}


// Singleton AudioManager struct
struct AudioManager audioManager = {
  .start = start,
  .pause = pause,
  .resume = resume,
  .stop = stop,
  .toggleTrackMute = toggleTrackMute,
  .toggleTrackSolo = toggleTrackSolo
};
