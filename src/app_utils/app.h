#ifndef rei_APP_H
#define rei_APP_H

#include <unordered_map>

#include "../render_pipeline.h"
#include "../renderer.h"

#if WIN32

#include "win_app.h"

#if RELEASE
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

namespace rei {
using App = WinApp;
} // namespace rei

#endif

#endif
