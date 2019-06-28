#ifndef rei_APP_H
#define rei_APP_H

#if WIN32

#include "win_app.h"

#ifdef USE_MSVC && DEBUG
// Disable the console window if we are debugging using Visual Studio
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

namespace rei {
  using App = WinApp;
}

#endif

#endif
