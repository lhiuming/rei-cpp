#ifndef CEL_CONSOLE_H
#define CEL_CONSOLE_H

#include <iostream>

#ifdef USE_MSVC
#include "win32/safe_windows.h"
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
      char buf[] = { c, '\0' };
    #ifdef USE_MSVC
      OutputDebugString(buf);
    #else
      std::puts(buff);
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