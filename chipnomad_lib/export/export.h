#ifndef __CHIPNOMAD_LIB__EXPORT_H__
#define __CHIPNOMAD_LIB__EXPORT_H__

#include <stdint.h>
#include <stdio.h>

#include "project.h"
#include "chipnomad_lib.h"

// Exporter base class
class Exporter {
  protected:
    ChipNomadState* chipnomadState;
    int renderedSeconds;

  public:
    Exporter(Project* project, int startRow) {
      this->chipnomadState = chipnomadCreate();
      this->chipnomadState->project = *project;
      this->renderedSeconds = 0;
      playbackInit(&this->chipnomadState->playbackState, &this->chipnomadState->project);
      playbackStartSong(&this->chipnomadState->playbackState, startRow, 0, 0);
    };

    virtual ~Exporter() {
      if (this->chipnomadState) {
        chipnomadDestroy(this->chipnomadState);
      }
    };

    void setMixVolume(float volume) { chipnomadState->mixVolume = volume; }

    virtual int next() = 0; // Returns seconds rendered, -1 if done
    virtual int finish() = 0;
    virtual void cancel() = 0;
};


// WAV Exporter
class ExporterWAV : public Exporter {
  private:
    FILE** files;        // Array of file handles (1 for normal, trackCount for stems)
    int fileCount;       // Number of output files
    int currentTrack;    // Current track being rendered (stems mode)
    int sampleRate;
    int channels;
    int bitDepth;
    int totalSamples;
    bool stems;           // false = single mixed file, true = one file per track
    char basePath[1024]; // Base path for file naming
    float* renderBuffer;

    void writeSamples(FILE* f, float* buffer, int samples);

  public:
    ExporterWAV(const char* path, Project* project, int startRow, int sampleRate, int bitDepth, float mixVolume, bool stems = false);
    ~ExporterWAV() override { cancel(); }
    int next() override;
    int finish() override;
    void cancel() override;
};


// PSG Exporter
class ExporterPSG : public Exporter {
  private:
    FILE* files[3];
    int numChips;
    char baseFilename[1024];

  public:
    ExporterPSG(const char* filename, Project* project, int startRow);
    ~ExporterPSG() override { cancel(); }
    int next() override;
    int finish() override;
    void cancel() override;
};


// VGM Exporter
class ExporterVGM : public Exporter {
  private:
    FILE* file;
    int waitSamples;
    int totalSamples;
    char baseFilename[1024];

    void writeWait();

  public:
    ExporterVGM(const char* filename, Project* project, int startRow);
    ~ExporterVGM() override { cancel(); }
    int next() override;
    int finish() override;
    void cancel() override;
};


#endif // __CHIPNOMAD_LIB__EXPORT_H__
