#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "corelib/corelib_file.h"
#include "playback.h"
#include "chips/chips.h"
#include <common.h>
#include "external/ayumi/ayumi.h"
#include "external/ayumi/ayumi_filters.h"

// WAV implementation data
typedef struct {
    int fileId;
    int sampleRate;
    int channels;
    int bitDepth;
    int totalSamples;
    struct PlaybackState playbackState;
    struct SoundChip chip;
    float frameSampleCounter;
    int allTracksStopped;
    int renderedSeconds;
    char filename[1024];
} WAVExporterData;

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
    header.audioFormat = (bitDepth == 32) ? 3 : 1;
    header.channels = channels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitDepth;
    header.byteRate = sampleRate * channels * (bitDepth / 8);
    header.blockAlign = channels * (bitDepth / 8);
    memcpy(header.data, "data", 4);
    header.dataSize = dataSize;

    fileWrite(fileId, &header, sizeof(WAVHeader));
}

static int wavExportWrite(WAVExporterData* data, float* buffer, int samples) {
    if (data->bitDepth == 16) {
        for (int i = 0; i < samples * data->channels; i++) {
            int sample = (int)(buffer[i] * appSettings.mixVolume * 32767.0f);
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            int16_t finalSample = (int16_t)sample;
            fileWrite(data->fileId, &finalSample, sizeof(int16_t));
        }
    } else if (data->bitDepth == 24) {
        for (int i = 0; i < samples * data->channels; i++) {
            int sample = (int)(buffer[i] * appSettings.mixVolume * 8388607.0f);
            if (sample > 8388607) sample = 8388607;
            if (sample < -8388608) sample = -8388608;
            uint8_t bytes[3] = {sample & 0xFF, (sample >> 8) & 0xFF, (sample >> 16) & 0xFF};
            fileWrite(data->fileId, bytes, 3);
        }
    } else if (data->bitDepth == 32) {
        for (int i = 0; i < samples * data->channels; i++) {
            float sample = buffer[i] * appSettings.mixVolume;
            fileWrite(data->fileId, &sample, sizeof(float));
        }
    }
    data->totalSamples += samples;
    return 0;
}

// WAV exporter methods
static int wavNext(Exporter* self) {
    WAVExporterData* data = (WAVExporterData*)self->data;
    if (data->allTracksStopped) return -1;

    const int samplesPerSecond = data->sampleRate;
    const int chunkSize = 1024;
    float buffer[chunkSize * 2];
    int totalSamplesRendered = 0;

    while (totalSamplesRendered < samplesPerSecond && !data->allTracksStopped) {
        int samplesRendered = 0;
        int samplesToRender = (samplesPerSecond - totalSamplesRendered < chunkSize) ? 
                             (samplesPerSecond - totalSamplesRendered) : chunkSize;

        while (samplesRendered < samplesToRender && !data->allTracksStopped) {
            if ((int)data->frameSampleCounter == 0) {
                data->frameSampleCounter += data->sampleRate / data->playbackState.p->tickRate;
                data->allTracksStopped = playbackNextFrame(&data->playbackState, &data->chip);
            }

            int frameSamples = ((int)data->frameSampleCounter < (samplesToRender - samplesRendered)) ?
                              (int)data->frameSampleCounter : (samplesToRender - samplesRendered);

            data->chip.render(&data->chip, buffer + samplesRendered * 2, frameSamples);
            samplesRendered += frameSamples;
            data->frameSampleCounter -= (float)frameSamples;
        }

        if (samplesRendered > 0) {
            wavExportWrite(data, buffer, samplesRendered);
            totalSamplesRendered += samplesRendered;
        }
    }

    return data->allTracksStopped ? -1 : ++data->renderedSeconds;
}

static int wavFinish(Exporter* self) {
    WAVExporterData* data = (WAVExporterData*)self->data;
    
    int dataSize = data->totalSamples * data->channels * (data->bitDepth / 8);
    fileSeek(data->fileId, 0, 0);
    writeWAVHeader(data->fileId, data->sampleRate, data->channels, data->bitDepth, dataSize);
    
    data->chip.cleanup(&data->chip);
    fileClose(data->fileId);
    free(data);
    free(self);
    return 0;
}

static void wavCancel(Exporter* self) {
    WAVExporterData* data = (WAVExporterData*)self->data;
    
    data->chip.cleanup(&data->chip);
    fileClose(data->fileId);
    fileDelete(data->filename);
    free(data);
    free(self);
}

Exporter* createWAVExporter(const char* filename, struct Project* project, int startRow, int sampleRate, int bitDepth) {
    Exporter* exporter = malloc(sizeof(Exporter));
    if (!exporter) return NULL;

    WAVExporterData* data = malloc(sizeof(WAVExporterData));
    if (!data) {
        free(exporter);
        return NULL;
    }

    data->fileId = fileOpen(filename, 1);
    if (data->fileId == -1) {
        free(data);
        free(exporter);
        return NULL;
    }

    data->sampleRate = sampleRate;
    data->channels = 2;
    data->bitDepth = bitDepth;
    data->totalSamples = 0;
    data->frameSampleCounter = 0.0f;
    data->allTracksStopped = 0;
    data->renderedSeconds = 0;
    strncpy(data->filename, filename, sizeof(data->filename) - 1);
    data->filename[sizeof(data->filename) - 1] = 0;

    writeWAVHeader(data->fileId, sampleRate, 2, bitDepth, 0);
    
    playbackInit(&data->playbackState, project);
    data->chip = createChipAY(sampleRate, project->chipSetup);
    
    // Use high-quality filter for export
    ayumi_set_filter_quality((struct ayumi*)data->chip.userdata, ayumi_filter_full);
    
    playbackStartSong(&data->playbackState, startRow, 0, 0);

    exporter->data = data;
    exporter->next = wavNext;
    exporter->finish = wavFinish;
    exporter->cancel = wavCancel;

    return exporter;
}

