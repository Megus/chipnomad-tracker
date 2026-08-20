#ifndef __OSCILLOSCOPE_H__
#define __OSCILLOSCOPE_H__

#ifdef __cplusplus
extern "C" {
#endif

// Number of bottom table rows the oscilloscope strip overlaps. These rows are
// drawn transparently (glyphs only) so the oscilloscope shows through behind
// the symbols. The strip itself starts at grid row (19 - OVERLAP_ROWS).
#define OSCILLOSCOPE_OVERLAP_ROWS 5

/**
 * @brief Initialize the oscilloscope system.
 *
 * @param sampleRate Audio sample rate.
 */
void oscilloscopeInit(int sampleRate);

/**
 * @brief Push per-track audio samples into the oscilloscope ring buffers.
 *
 * Called from the audio callback (audio thread).
 *
 * @param trackBuffers Array of trackCount mono float buffers.
 * @param trackCount Number of tracks.
 * @param frames Number of samples per track.
 */
void oscilloscopePushSamples(float** trackBuffers, int trackCount, int frames);

/**
 * @brief Draw the overlaid per-track oscilloscope lines.
 *
 * Called from the render loop (main thread).
 */
void oscilloscopeDraw(void);

#ifdef __cplusplus
}
#endif

#endif
