#include "misc.h"

const char* getEnvelopeShapeASCII(int shape) {
  shape = shape & 0x0F;
  if (shape == 0) return "    ";
  if (shape <= 3 || shape == 9) return "|\\__";
  if ((shape >= 4 && shape <= 7) || shape == 15) return "/|__";
  if (shape == 8) return "|\\|\\";
  if (shape == 10) return "\\/\\/";
  if (shape == 11) return "\\|^^";
  if (shape == 12) return "/|/|";
  if (shape == 13) return "/^^^";
  if (shape == 14) return "/\\/\\";
  return "    ";
}
