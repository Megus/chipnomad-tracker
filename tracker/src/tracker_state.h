#ifndef __TRACKER_STATE_H__
#define __TRACKER_STATE_H__

#include "chipnomad_lib.h"
#include "app_settings.h"

class TrackerState {
  public:
    TrackerState():
      projectModified(false), songRow(0), songTrack(0), chainRow(0) {};
    ~TrackerState() = default;

    AppSettings settings; // Application settings

    chipnomad::Project project; // Current project
    bool projectModified; // Is project modified?

    int songRow; // Selected song row
    int songTrack; // Selected track
    int chainRow; // Selected chain row
};

#endif // __TRACKER_STATE_H__
