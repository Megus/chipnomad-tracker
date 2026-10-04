#ifndef __INPUT_SDL12_H__
#define __INPUT_SDL12_H__

#include "input_utils.h"

class InputUtilsSDL12 : public InputUtils {
  public:
    void initDefaultKeyMapping(KeyMapping& mapping) override;
    const char* getKeyName(InputCode input) override;
};

#endif // __INPUT_SDL12_H__
