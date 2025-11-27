#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>
#include "project.h"

// Forward declaration
struct Exporter;

// Exporter interface (OOP-like with function pointers)
typedef struct Exporter {
    void* data; // Private implementation data
    int (*next)(struct Exporter* self); // Returns seconds rendered, -1 if done
    int (*finish)(struct Exporter* self);
    void (*cancel)(struct Exporter* self);
} Exporter;

// Export factory functions
Exporter* createWAVExporter(const char* filename, struct Project* project, int startRow, int sampleRate, int bitDepth);
Exporter* createPSGExporter(const char* filename, struct Project* project, int startRow);

#endif