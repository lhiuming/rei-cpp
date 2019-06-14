#ifndef CEL_CONSOLE_H
#define CEL_CONSOLE_H

#include <iostream>

#ifdef USE_MSVC
#include <windows.h>
#endif

/*
 * console.h
 * Define a logging function to print debug information to the console.
 */

namespace CEL {

// Stream buffer that write to Visual Studio Debut Output
class DebugStreambuf : public std::streambuf {
public:
  virtual int_type overflow(int_type c = EOF) {
    if (c != EOF) {
#ifdef USE_MSVC
      char_type buf[] = {traits_type::to_char_type(c), '\0'};
      OutputDebugString(buf);
#else
      std::cout.put(c);
#endif
    }
    return c;
  }
};

// The ostream type that print to the console.
class Logger : public std::ostream {
  // Base alias
  using Base = std::ostream;

public:
  Logger() : Base(&dbgstream) {}

  // Formatted print
  void printf();

private:
  DebugStreambuf dbgstream; // platform-adpted output stream buffer
};

// A gobal Logger
extern Logger console;

} // namespace CEL

#endif
