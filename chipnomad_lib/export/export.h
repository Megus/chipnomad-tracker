#ifndef __CHIPNOMAD_LIB__EXPORT_H__
#define __CHIPNOMAD_LIB__EXPORT_H__

#include <stdint.h>
#include <stdio.h>

#include "project.h"
#include "chipnomad_lib.h"

// Exporter base class
class Exporter {
  protected:
    // The engine is owned by the exporter. The caller owns the Project and
    // must keep it alive for the lifetime of the exporter.
    chipnomad::Engine engine;
    int renderedSeconds;

    // Deferred start: subclasses call this after they've set up their chip
    // factory state (e.g. output files) so the factory runs at the right time.
    void startExport(Project* project, int startRow) {
      engine.setProject(project);
      engine.player.playSong(startRow, 0, 0);
    }

  public:
    // factory: chip factory to use for this export (nullptr = default AY chip).
    // sampleRate: render sample rate for the engine.
    Exporter(chipnomad::ChipFactory factory, int sampleRate)
      : engine(factory, sampleRate), renderedSeconds(0) {}

    virtual ~Exporter() {}

    void setMixVolume(float volume) { engine.mixVolume = volume; }

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
