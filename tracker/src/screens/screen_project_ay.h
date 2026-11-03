#ifndef __SCREEN_PROJECT_AY_H__
#define __SCREEN_PROJECT_AY_H__

#include "screen_partial.h"

class ScreenPartialProjectAY : public ScreenPartial {
  public:
    ScreenPartialProjectAY(Screen& parent, int startRow): ScreenPartial(parent, startRow) {};
    ~ScreenPartialProjectAY() = default;
};

#endif // __SCREEN_PROJECT_AY_H__