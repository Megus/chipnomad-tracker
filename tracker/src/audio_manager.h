#ifndef __AUDIOMANAGER_H__
#define __AUDIOMANAGER_H__

#include "common.h"
#include "chipnomad_lib.h"

class WavFile;

enum class TrackState: uint8_t {
  normal = 0,
  solo = 1,
  muted = 2
};

class AudioManager {
  public:
    AudioManager(ChipNomadState *state);
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

    // WAV preview functions
    virtual int startWavPreview(const char* path);
    virtual void stopWavPreview();

  private:
    ChipNomadState *chipnomadState;
    int sampleRate;
    int bufferSize;
    int pendingReinitChips;
    float* renderBuffer;

    // WAV preview state
    WavFile* wavPreview;
    double wavPreviewPosition;  // Fractional accumulator for resampling (0.0 to 1.0)
    double wavPreviewRateRatio; // sourceSampleRate / outputSampleRate

    // WAV preview read buffer (persists across audio callbacks)
    static const int WAV_PREVIEW_BUF_SIZE = 256;
    int16_t wavPreviewBuf[WAV_PREVIEW_BUF_SIZE];
    int wavPreviewBufPos;
    int wavPreviewBufCount;

    void updatePlaybackMuteFlags(void);

    friend void audioCallback(int16_t* buffer, int stereoSamples);
};

// Singleton AudioManager instance
// TODO: Avoid the global variable and use dependency injection instead
extern AudioManager& audio;

#endif
