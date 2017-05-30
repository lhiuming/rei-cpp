#ifndef CEL_CONSOLE_H
#define CEL_CONSOLE_H

#include <iostream>
#include <sstream>

#ifdef USE_MSVC
#include "win32/safe_windows.h"
#endif 

/*
 * console.h
 * Define a logging function to print debug information to the console. 
 */

namespace CEL {

class Logger : public std::ostream {

  // Base alias
  using Base = typename decltype(std::cout);

  // Stream alias 
  using char_type = Base::char_type;
  using traits_type = Base::traits_type;

public:

  // Default constructor : share stdout with std::cout
  Logger() : Base(std::cout.rdbuf()) {}

  // Destructor 
  ~Logger() {}

  // Callable
  template<class Printable> void operator()(Printable content) {
  #ifdef USE_MSVC
    OutputDebugString(content);
  #else
    Base << content;
  #endif
  }

  // operator<< for Streaming //

  // Basic template 
  template<class Streamable> Logger& operator<<(Streamable content) {
  #ifdef USE_MSVC
    std::ostringstream ss;
    ss << content;
    OutputDebugString(ss.str().c_str());
  #else
    Base << content;
  #endif
    return *this;
  }

  // Specialize for std::endl
  Logger& operator<<(  std::basic_ostream<char_type, traits_type>&
    (*manip)(std::basic_ostream<char_type, traits_type>&)  ) {
  #ifdef USE_MSVC
    OutputDebugString("\n");
  #else
    Base::operator<<(manip);
  #endif
    return *this;
  }
};


// globale logger (not actually global, though) 
Logger console;


} // namespace CEL 

#endif