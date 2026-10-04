#ifndef __FONT_MOCK_H__
#define __FONT_MOCK_H__

#include "i_font.h"
#include <cstddef>

// FontMock: a minimal in-memory IFont for tests. Returns a fixed mock font and
// records setCurrent/load/free calls.
class FontMock : public IFont {
  public:
    static inline uint8_t mockFontData[1] = {0};

    Font mockFont = {
      "Mock",
      { {16, 24, mockFontData} },
      1
    };

    int setCurrentCalls = 0;
    int loadCalls = 0;
    int freeCalls = 0;
    const Font* lastSetFont = nullptr;

    const Font* getDefault() override { return &mockFont; }

    void setCurrent(const Font* f) override {
      setCurrentCalls++;
      lastSetFont = f;
    }

    const Font* getCurrent() override { return &mockFont; }

    const FontResolution* selectResolution(const Font* f, int, int) override {
      return f ? &f->resolutions[0] : &mockFont.resolutions[0];
    }

    Font* load(const char*) override {
      loadCalls++;
      return nullptr;
    }

    void freeFont(Font*) override {
      freeCalls++;
    }
};

#endif // __FONT_MOCK_H__
