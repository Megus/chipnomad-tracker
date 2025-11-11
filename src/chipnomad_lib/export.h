#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>
#include <project.h>

typedef struct {
    int fileId;
    int sampleRate;
    int channels;
    int bitDepth;
    int totalSamples;
} WAVExporter;

// Streaming WAV export
WAVExporter* wavExportStart(const char* filename, int sampleRate, int channels, int bitDepth);
int wavExportWrite(WAVExporter* exporter, float* buffer, int samples);
int wavExportFinish(WAVExporter* exporter);

// Project export
int exportProjectToWAV(const char* filename, struct Project* project, int startRow, int sampleRate, int bitDepth);

#endif