#ifndef CEL_WIN32_SAFE_WINDOWS_H
#define CEL_WIN32_SAFE_WINDOWS_H

/*
 * win32/safe_windows.h  --  A safe version of windows.h
 * Remove the macro polution caused by windows.h.
 */

#define NOMINMAX

#include <windows.h>

#undef near
#undef far

#endif