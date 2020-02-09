#ifndef REI_DEBUG_H
#define REI_DEBUG_H

#if DEBUG
#include <stdexcept>
#endif

#include "console.h"
#include "string_utils.h"

namespace rei {

using MetaMsg = const char*;
constexpr MetaMsg k_empty_meta = "";

template <typename LogMsg>
static inline void log(LogMsg msg, MetaMsg meta = k_empty_meta) {
  console << msg << meta << endl;
}

template <typename LogMsg>
static inline void warning(LogMsg msg, MetaMsg meta = k_empty_meta) {
  console << "WARNING: " << msg << meta << endl;
}

template <typename T, typename LogMsg>
static inline bool warning_if(T expr, LogMsg msg, MetaMsg meta = k_empty_meta) {
  if (expr) {
    warning(msg, meta);
    return true;
  }
  return false;
}

template <typename LogMsg>
static inline void error(LogMsg msg, MetaMsg meta = k_empty_meta) {
  console << "ERROR: " << msg << meta << endl;
}

template <typename T, typename LogMsg>
static inline bool error_if(T expr, LogMsg msg, MetaMsg meta = k_empty_meta) {
  if (expr) { error(msg, meta); }
  return false;
}

static inline void not_implemented(MetaMsg meta = k_empty_meta) {
  warning("Functionality is not implemented. ", meta);
}

static inline void deprecated(MetaMsg meta = k_empty_meta) {
  error("Deprecated invocation.", meta);
}

template <typename T, typename LogMsg>
static inline bool rassert(T expr, LogMsg msg, MetaMsg meta = k_empty_meta) {
  if (!expr) {
    console << "Assertion Fail: " << msg << meta << endl;
    return false;
  }
  return true;
}

template <typename T, typename LogMsg>
static inline void uninit(T expr, LogMsg msg, MetaMsg meta = k_empty_meta) {
  rassert(!(expr), msg, meta);
}

} // namespace rei

#if defined __func__
#define REI_FUNC_META __func__
#elif defined __FUNCTION__
#define REI_FUNC_META __FUNCTION__
#else
#define REI_FUNC_META "[unavailable]"
#endif

#define REI_SYMBOLIZE(x) #x
#define REI_STRINGFY(n) REI_SYMBOLIZE(n)

#define REI_DEBUG_META \
  " -- function " REI_FUNC_META ", in file " __FILE__ " (line " REI_STRINGFY(__LINE__) ")"

#define REI_THROW(msg) throw std::runtime_error(msg)

#define REI_WARNING(msg) ::rei::warning(msg, REI_DEBUG_META)
#define REI_WARNINGIF(expr) ::rei::warning_if(expr, #expr, REI_DEBUG_META)

#if THROW
#define REI_ERROR(msg) (::rei::error(msg, REI_DEBUG_META), REI_THROW(msg))
#else
#define REI_ERROR(msg) ::rei::error(msg, REI_DEBUG_META)
#endif
#define REI_ERRORIF(expr) ::rei::error_if(expr, #expr, REI_DEBUG_META)

#if THROW
#define REI_ASSERT(expr) \
  ((expr) ? true : (::rei::rassert(false, #expr, REI_DEBUG_META), REI_THROW(#expr)))
#define REI_ASSERT_MSG(expr, msg) \
  ((expr) ? true : (::rei::rassert(false, msg, REI_DEBUG_META), REI_THROW(#expr)))
#else
#define REI_ASSERT(expr) ::rei::rassert(expr, #expr, REI_DEBUG_META)
#define REI_ASSERT_MSG(expr, msg) ::rei::rassert(expr, msg, REI_DEBUG_META)
#endif

#if THROW
#define REI_UNINIT(var) (var ? REI_THROW(#var " is not empry") : var, var)
#else
#define UNINIT(var) (::rei::uninit(var, #var "is not empty", REI_DEBUG_META), var)
#endif

#define REI_NOT_IMPLEMENT() (::rei::not_implemented(REI_DEBUG_META))

#define REI_NOT_IMPLEMENTED REI_NOT_IMPLEMENT();

#define REI_DEPRECATE() (::rei::deprecated(REI_DEBUG_META))

#define REI_DEPRECATED REI_DEPRECATE();

#endif
