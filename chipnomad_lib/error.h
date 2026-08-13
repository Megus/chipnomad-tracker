#ifndef __CHIPNOMAD_LIB__ERRORS_H__
#define __CHIPNOMAD_LIB__ERRORS_H__

namespace chipnomad {

  class Error {
    public:
      // Error message buffer (40 chars + null terminator)
      static char message[41];
      // Reset error message
      static void clear(void);
      // Set error message with format string
      static void set(const char* format, ...);
  };

}

#endif // __CHIPNOMAD_LIB__ERRORS_H__