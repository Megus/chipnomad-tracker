#ifndef __CHIPNOMAD_LIB__PLAYBACK_INTERNAL_H__
#define __CHIPNOMAD_LIB__PLAYBACK_INTERNAL_H__

// Internal playback declarations shared between the playback translation units.
//
// After the C++ refactor, the playback engine is implemented as the
// chipnomad::Player and chipnomad::Engine classes (see playback.h /
// chipnomad_lib.h). All former free-function internals are now private methods
// of chipnomad::Player, so this header no longer needs to declare them.
//
// It is kept as an (intentionally near-empty) header because several
// translation units still include it. New internal declarations that must be
// shared across .cpp files (and are not appropriate as Player members) can be
// added here.

#include "playback.h"

#endif // __CHIPNOMAD_LIB__PLAYBACK_INTERNAL_H__
