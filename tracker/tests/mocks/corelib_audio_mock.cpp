#include "chipnomad_lib.h"
#include "tracker_state.h"
#include "audio_mock.h"

// The tracker global state. In the real app this lives in common.cpp (not part
// of the test build), so the test binary provides its own definition here.
// Tests that use it (copy_paste, return_values, edit_common) assign their own
// TrackerState instance to it.
TrackerState* chipnomadState = nullptr;

// The audio device global. In the real app this is defined by the compiled
// platform source (platforms/<platform>/corelib_audio.cpp), which is not part
// of the test build, so the test binary provides its own mock instance.
static AudioMock defaultAudioMock;
IAudio* audioDevice = &defaultAudioMock;
