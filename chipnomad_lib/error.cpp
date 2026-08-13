#include "error.h"
#include <cstdarg>
#include <cstdio>

namespace chipnomad {
  char Error::message[41] = {0};

  void Error::clear(void) {
    message[0] = '\0';
  }

  void Error::set(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
  }
}
