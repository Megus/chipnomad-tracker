#ifndef BUTTON_ICONS_H
#define BUTTON_ICONS_H

#include <stdint.h>

// Icon dimensions (32x32 pixels, 1 bit per pixel)
#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_BYTES_PER_ROW 4  // 32 bits = 4 bytes per row

// Button icon bitmaps (1 = white pixel, 0 = transparent)
// Replace these with your Aseprite exports

// Arrow Up
extern const uint8_t icon_arrow_up[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// Arrow Down  
extern const uint8_t icon_arrow_down[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// Arrow Left
extern const uint8_t icon_arrow_left[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// Arrow Right
extern const uint8_t icon_arrow_right[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// EDIT text
extern const uint8_t icon_edit[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// OPT text
extern const uint8_t icon_opt[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// SHIFT text
extern const uint8_t icon_shift[ICON_HEIGHT * ICON_BYTES_PER_ROW];

// PLAY text
extern const uint8_t icon_play[ICON_HEIGHT * ICON_BYTES_PER_ROW];

#endif // BUTTON_ICONS_H