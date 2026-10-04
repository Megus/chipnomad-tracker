#ifndef __INPUT_SDL2_H__
#define __INPUT_SDL2_H__

#include "input_utils.h"

class InputUtilsSDL2 : public InputUtils {
  public:
    void initDefaultKeyMapping(KeyMapping& mapping) override;
    const char* getKeyName(InputCode input) override;
};

#endif // __INPUT_SDL2_H__
