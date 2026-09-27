#ifndef __TRACKER_STATE_H__
#define __TRACKER_STATE_H__

#include "chipnomad_lib.h"
#include "app_settings.h"

class TrackerState {
  public:
    TrackerState();
    ~TrackerState() = default;

    AppSettings settings;

    chipnomad::Project project;
    bool projectModified;

    int* pSongRow;
    int* pSongTrack;
    int* pChainRow;
};

#endif // __TRACKER_STATE_H__
