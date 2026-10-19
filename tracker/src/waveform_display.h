#ifndef __WAVEFORM_DISPLAY_H__
#define __WAVEFORM_DISPLAY_H__

#include <stdint.h>
#include <functional>
#include "gfx.h"
#include "chipnomad_lib.h"

class WaveformDisplay {
  public:
    WaveformDisplay(Gfx& gfx);
    ~WaveformDisplay();

    // Get waveform bitmap for a track
    Bitmap* getWaveformBitmap(chipnomad::Engine engine, int trackIdx);
    // Render a sample waveform preview into a bitmap
    void renderSamplePreview(Bitmap* bitmap, uint8_t* sampleData, uint16_t startSample, uint16_t endSample);
    // Render a wavetable preview into a bitmap
    void renderAYWavetablePreview(Bitmap* bitmap, uint8_t* wavetable, int isYM);

  protected:
    static constexpr int dimBrightness = 160;

    void drawVerticalLine(Bitmap* bitmap, int x, int y1, int y2, uint8_t shade);
    void drawAYWaveformSlice(Bitmap* bitmap, int x, int amplitude, int toneHigh, int hasEnvelope, int hasNoise, int noiseShadeBase);
    int getAYEnvelopeHeight(int x, int envShape);
    void renderWaveformPreview(Bitmap* bitmap, uint8_t* data, uint16_t dataLength, std::function<uint8_t(uint8_t)> transformFunc);

    Gfx& gfx;

    Bitmap* emptyBitmap = NULL;
    Bitmap* waveformBitmaps[PROJECT_MAX_TRACKS];
    int charW = 0;
    int charH = 0;
    uint8_t noisePattern[512];
    int noiseAnimIdx = 0;
};

#endif // __WAVEFORM_DISPLAY_H__
