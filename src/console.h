#ifndef REI_CONSOLE_H
#define REI_CONSOLE_H

#include <iostream>

#ifdef USE_MSVC
#include <windows.h>
#endif

/*
 * console.h
 * Define a logging function to print debug information to the console.
 */

namespace rei {

using std::endl;

// Stream buffer that write to Visual Studio Debut Output
class DebugStreambuf : public std::wstreambuf {
public:
  virtual int_type overflow(int_type c = EOF) {
    if (c != EOF) {
#ifdef USE_MSVC
      char_type buf[] = {traits_type::to_char_type(c), '\0'};
      OutputDebugStringW(buf);
#else
      std::cout.put(c);
#endif
    }
    return c;
  }
};

// The ostream type that print to the console.
class Logger : public std::wostream {
  // Base alias
  using Base = std::wostream;

public:
  Logger() : Base(&dbgstream) {}

private:
  DebugStreambuf dbgstream; // platform-adpted output stream buffer
};

// A gobal Logger
extern Logger console;

} // namespace REI

#endif
