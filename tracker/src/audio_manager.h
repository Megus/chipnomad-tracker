#ifndef __AUDIOMANAGER_H__
#define __AUDIOMANAGER_H__

#include "common.h"
#include "chipnomad_lib.h"
#include "tracker_state.h"

class AudioSource;

enum class TrackState: uint8_t {
  normal = 0,
  solo = 1,
  muted = 2
};

class AudioManager {
  public:
    AudioManager(TrackerState *state);
    ~AudioManager();

    // Track solo/mute states
    TrackState trackStates[PROJECT_MAX_TRACKS];

    // Audio manager lifecycle functions
    virtual int start(int sampleRate, int audioBufferSize);
    virtual void pause(void);
    virtual void resume(void);
    virtual void stop();

    // Track mute/solo functions
    void toggleTrackMute(int trackIdx);
    void toggleTrackSolo(int trackIdx);

    // Chip reinitialization function
    void reinitChips();

    // Preview functions. The AudioManager takes ownership of the source and
    // deletes it when the preview stops.
    virtual void startPreview(AudioSource* source);
    virtual void stopPreview();

    // Convenience helpers for specific source types
    virtual int startWavPreview(const char* path);
    virtual int startWavetablePreview(const char* path, bool isYM);

  private:
    TrackerState *trackerState;
    int sampleRate;
    int bufferSize;
    int pendingReinitChips;
    float* renderBuffer;

    // Preview state (generic audio source: WAV, wavetable, etc.)
    AudioSource* previewSource;
    double previewPosition;   // Fractional accumulator for resampling (0.0 to 1.0)
    double previewRateRatio;  // sourceSampleRate / outputSampleRate

    // Preview read buffer (persists across audio callbacks)
    static const int PREVIEW_BUF_SIZE = 256;
    int16_t previewBuf[PREVIEW_BUF_SIZE];
    int previewBufPos;
    int previewBufCount;

    void updatePlaybackMuteFlags(void);

    friend void audioCallback(int16_t* buffer, int stereoSamples);
};

// Singleton AudioManager instance
// TODO: Avoid the global variable and use dependency injection instead
extern AudioManager& audio;

#endif
