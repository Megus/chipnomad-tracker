#ifndef __INPUT_MOCK_H__
#define __INPUT_MOCK_H__

#include "i_input.h"

// InputMock: deterministic IInput for tests. initDefaultKeyMapping is a no-op
// (records the call); getKeyName returns a fixed string.
class InputMock : public IInput {
  public:
    int initCalls = 0;
    AppSettings* lastSettings = nullptr;

    void initDefaultKeyMapping(AppSettings* settings) override {
      initCalls++;
      lastSettings = settings;
    }

    const char* getKeyName(InputCode) override {
      return "?";
    }
};

#endif // __INPUT_MOCK_H__
