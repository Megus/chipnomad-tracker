#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>
#include "project.h"
#include "chipnomad_lib.h"

// Exporter interface (OOP-like with function pointers)
typedef struct Exporter {
  void* data; // Private implementation data
  ChipNomadState* chipnomadState; // Exposed ChipNomad state for configuration
  int (*next)(struct Exporter* self); // Returns seconds rendered, -1 if done
  int (*finish)(struct Exporter* self);
  void (*cancel)(struct Exporter* self);
} Exporter;

// Export factory functions
Exporter* createWAVExporter(const char* filename, Project* project, int startRow, int sampleRate, int bitDepth);
Exporter* createWAVStemsExporter(const char* basePath, Project* project, int startRow, int sampleRate, int bitDepth);
Exporter* createPSGExporter(const char* filename, Project* project, int startRow);

#endif