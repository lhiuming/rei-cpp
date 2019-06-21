#ifndef REI_DEBUG_H
#define REI_DEBUG_H

#if !NDEBUG
#include <stdexcept>
#endif

#include "console.h"

namespace rei {

using LogMsg = const std::string&;
constexpr char* k_empty_chars = "";

static inline void log(LogMsg msg, const char* meta = k_empty_chars) {
  console << msg << meta << endl;
}

static inline void warning(LogMsg msg, const char* meta = k_empty_chars) {
  console << "WARNING: " << msg << meta << endl;
}

static inline void error(LogMsg msg, const char* meta = k_empty_chars) {
  #if NDEBUG
  console << "ERROR: " << msg << meta << endl;
  #else
  throw std::runtime_error("ERROR: " + msg);
  #endif
}

static inline void not_implemented(const char* meta = k_empty_chars) {
  warning("Functionality is not implemented. ", meta);
}

static inline void deprecated(const char* meta = k_empty_chars) {
  error("Deprecated invocation.", meta);
}

template<class T>
static inline bool rassert(T expr, LogMsg msg, const char* meta = k_empty_chars) {
  if (!expr) {
    console << "Assertion Fail: " << msg << meta << endl;
    return false;
  }
  return true;
}

template<class T>
static inline void uninit(T expr, LogMsg msg, const char* meta = k_empty_chars) {
  rassert(!(expr), msg, meta);
}

}

#if defined __func__
#define REI_FUNC_META __func__
#elif defined __FUNCTION__
#define REI_FUNC_META __FUNCTION__
#else
#define REI_FUNC_META "[unavailable]"
#endif

#define REI_SYMBOLIZE(x) #x
#define REI_STRINGFY(n) REI_SYMBOLIZE(n)

#define REI_DEBUG_META " -- function " REI_FUNC_META ", in file " __FILE__ " (line " REI_STRINGFY(__LINE__) ")"

#define REI_THROW(msg) throw std::runtime_error(msg)

#if THROW
#define ASSERT(expr) ((expr) ? true : REI_THROW(#expr))
#else
#define ASSERT(expr) ::rei::rassert(expr, #expr, REI_DEBUG_META)
#endif

#if THROW
#define UNINIT(var) (var ? REI_THROW(#var " is not empry") : var, var)
#else
#define UNINIT(var) (::rei::uninit(var, #var "is not empty", REI_DEBUG_META), var)
#endif

#define NOT_IMPLEMENT() (::rei::not_implemented(REI_DEBUG_META))

#define NOT_IMPLEMENTED NOT_IMPLEMENT();

#define DEPRECATE() (::rei::deprecated(REI_DEBUG_META))

#define DEPRECATED DEPRECATE();

#define WARNING(expr) ::rei::warning(expr, REI_DEBUG_META)

#endif
