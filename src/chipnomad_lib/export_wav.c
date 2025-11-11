#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <corelib_file.h>
#include <playback.h>
#include <chips.h>
#include <common.h>

typedef struct {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
} WAVHeader;

static void writeWAVHeader(int fileId, int sampleRate, int channels, int bitDepth, int dataSize) {
    WAVHeader header;

    memcpy(header.riff, "RIFF", 4);
    header.fileSize = 36 + dataSize;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmtSize = 16;
    header.audioFormat = (bitDepth == 32) ? 3 : 1; // IEEE float for 32-bit, PCM for others
    header.channels = channels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitDepth;
    header.byteRate = sampleRate * channels * (bitDepth / 8);
    header.blockAlign = channels * (bitDepth / 8);
    memcpy(header.data, "data", 4);
    header.dataSize = dataSize;

    fileWrite(fileId, &header, sizeof(WAVHeader));
}

WAVExporter* wavExportStart(const char* filename, int sampleRate, int channels, int bitDepth) {
    WAVExporter* exporter = malloc(sizeof(WAVExporter));
    if (!exporter) return NULL;

    exporter->fileId = fileOpen(filename, 1);
    if (exporter->fileId == -1) {
        free(exporter);
        return NULL;
    }

    exporter->sampleRate = sampleRate;
    exporter->channels = channels;
    exporter->bitDepth = bitDepth;
    exporter->totalSamples = 0;

    // Write placeholder header
    writeWAVHeader(exporter->fileId, sampleRate, channels, bitDepth, 0);

    return exporter;
}

int wavExportWrite(WAVExporter* exporter, float* buffer, int samples) {
    if (!exporter || exporter->fileId == -1) return -1;

    if (exporter->bitDepth == 16) {
        for (int i = 0; i < samples * exporter->channels; i++) {
            int sample = (int)(buffer[i] * appSettings.mixVolume * 32767.0f);
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            int16_t finalSample = (int16_t)sample;
            fileWrite(exporter->fileId, &finalSample, sizeof(int16_t));
        }
    } else if (exporter->bitDepth == 24) {
        for (int i = 0; i < samples * exporter->channels; i++) {
            int sample = (int)(buffer[i] * appSettings.mixVolume * 8388607.0f);
            if (sample > 8388607) sample = 8388607;
            if (sample < -8388608) sample = -8388608;
            uint8_t bytes[3] = {sample & 0xFF, (sample >> 8) & 0xFF, (sample >> 16) & 0xFF};
            fileWrite(exporter->fileId, bytes, 3);
        }
    } else if (exporter->bitDepth == 32) {
        for (int i = 0; i < samples * exporter->channels; i++) {
            float sample = buffer[i] * appSettings.mixVolume;
            fileWrite(exporter->fileId, &sample, sizeof(float));
        }
    }

    exporter->totalSamples += samples;
    return 0;
}

int wavExportFinish(WAVExporter* exporter) {
    if (!exporter || exporter->fileId == -1) return -1;

    int dataSize = exporter->totalSamples * exporter->channels * (exporter->bitDepth / 8);

    // Update header with correct sizes
    fileSeek(exporter->fileId, 0, 0); // SEEK_SET = 0
    writeWAVHeader(exporter->fileId, exporter->sampleRate, exporter->channels, exporter->bitDepth, dataSize);

    fileClose(exporter->fileId);
    free(exporter);
    return 0;
}

int exportProjectToWAV(const char* filename, struct Project* project, int startRow, int sampleRate, int bitDepth) {
    WAVExporter* exporter = wavExportStart(filename, sampleRate, 2, bitDepth);
    if (!exporter) return -1;

    // Initialize playback
    struct PlaybackState playbackState;
    playbackInit(&playbackState, project);

    // Create and configure sound chip
    struct SoundChip chip = createChipAY(sampleRate, project->chipSetup);

    // Start song playback without looping
    playbackStartSong(&playbackState, startRow, 0, 0);

    // Render audio in chunks
    const int chunkSize = 1024;
    float buffer[chunkSize * 2]; // Stereo
    float frameSampleCounter = 0.0f;
    int allTracksStopped = 0;

    do {
        int samplesRendered = 0;

        while (samplesRendered < chunkSize && !allTracksStopped) {
            // Handle frame timing
            if ((int)frameSampleCounter == 0) {
                frameSampleCounter += sampleRate / project->tickRate;
                allTracksStopped = playbackNextFrame(&playbackState, &chip);

            }

            int samplesToRender = ((int)frameSampleCounter < (chunkSize - samplesRendered)) ?
                                  (int)frameSampleCounter : (chunkSize - samplesRendered);

            chip.render(&chip, buffer + samplesRendered * 2, samplesToRender);

            samplesRendered += samplesToRender;
            frameSampleCounter -= (float)samplesToRender;
        }

        if (samplesRendered > 0) {
            wavExportWrite(exporter, buffer, samplesRendered);
        }
    } while (!allTracksStopped);

    chip.cleanup(&chip);
    return wavExportFinish(exporter);
}