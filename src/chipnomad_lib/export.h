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

typedef struct {
    int fileId;
    int tickRate;
    int currentTick;
} PSGExporter;

// Streaming WAV export
WAVExporter* wavExportStart(const char* filename, int sampleRate, int channels, int bitDepth);
int wavExportWrite(WAVExporter* exporter, float* buffer, int samples);
int wavExportFinish(WAVExporter* exporter);

// Streaming PSG export
PSGExporter* psgExportStart(const char* filename, int tickRate);
int psgExportFinish(PSGExporter* exporter);

// Project export
int exportProjectToWAV(const char* filename, struct Project* project, int startRow, int sampleRate, int bitDepth);
int exportProjectToPSG(const char* filename, struct Project* project, int startRow);

#endif