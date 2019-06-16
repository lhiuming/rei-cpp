#ifndef REI_DEBUG_H
#define REI_DEBUG_H

#include "console.h"

namespace rei {

using LogMsg = const std::string&;
constexpr char* k_emptry_chars = "";

static inline void log(LogMsg msg, const char* meta = k_emptry_chars) {
  console << msg << meta << endl;
}

static inline void warning(LogMsg msg, const char* meta = k_emptry_chars) {
  console << "WARNING: " << msg << meta << endl;
}

static inline void error(LogMsg msg, const char* meta = k_emptry_chars) {
  console << "ERROR: " << msg << meta << endl;
}

static inline void deprecated(const char* meta = k_emptry_chars) {
  error("Deprecated invocation.", meta);
}

template<class T>
static inline void rassert(T expr, LogMsg msg, const char* meta = k_emptry_chars) {
  if (!expr) { console << "Assertion Fail: " << msg << meta << endl; }
}

template<class T>
static inline void uninit(T expr, LogMsg msg, const char* meta = k_emptry_chars) {
  rassert(!(expr), msg, meta);
}

}

#if REI_ASSERT || !NDEBUG

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

#define ASSERT(expr) ::rei::rassert(expr, #expr, REI_DEBUG_META)

#define UNINIT(expr) (::rei::uninit(expr, #expr "is not empty", REI_DEBUG_META), (expr))

#define DEPRECATE (::rei::deprecated(REI_DEBUG_META));

#define WARNING(expr) ::rei::warning(expr, REI_DEBUG_META)

#endif

#endif
