#ifndef __TRACKER_STATE_H__
#define __TRACKER_STATE_H__

#include "chipnomad_lib.h"

/**
 * TrackerState holds the top-level state of the tracker application.
 *
 * It owns the Project (by value) and the playback Engine (by pointer). This is
 * the first step in eliminating global variables: it replaces the old global
 * `ChipNomadState* chipnomadState` that used to live in the library.
 *
 * OWNERSHIP: the tracker now owns the Project; the Engine only holds a Project*
 * pointing at our owned copy. Playback state lives inside engine->player and
 * audio/chip state (mixVolume, aySampleDithering, audioOverload, trackWarnings,
 * chips[]) lives on the engine.
 *
 * NOTE: For now a single global instance of this class is used as a bridge so
 * the rest of the tracker keeps compiling. Dependency injection will be
 * introduced later as parts of the tracker are wrapped into classes.
 */
class TrackerState {
  public:
    Project project;              // Owned by value
    chipnomad::Engine* engine;    // Owned; created in initEngine()
    int projectModified;          // Set when the project has unsaved changes

    TrackerState();
    ~TrackerState();

    /**
     * Create the playback engine and bind it to our owned Project.
     * Replaces the old chipnomadCreate() + chipnomadInitChips() flow.
     * @param sampleRate Audio sample rate for the engine
     * @param factory Chip factory, or nullptr for the default AY implementation
     */
    void initEngine(int sampleRate, chipnomad::ChipFactory factory);
};

#endif // __TRACKER_STATE_H__
