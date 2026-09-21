#ifndef __BUTTON_ICONS_H__
#define __BUTTON_ICONS_H__

#include <stdint.h>

// Icon dimensions (32x8 pixels, 1 bit per pixel)
constexpr int kIconWidth = 32;
constexpr int kIconHeight = 8;
constexpr int kIconBytesPerRow = kIconWidth / 8;

extern const uint8_t iconArrowUp[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconArrowDown[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconArrowLeft[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconArrowRight[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconEdit[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconOpt[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconShift[kIconHeight * kIconBytesPerRow];
extern const uint8_t iconPlay[kIconHeight * kIconBytesPerRow];

#endif // __BUTTON_ICONS_H__
