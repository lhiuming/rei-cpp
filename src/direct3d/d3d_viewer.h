#ifndef CEL_DIRECT3D_D3D_VIEWER_H
#define CEL_DIRECT3D_D3D_VIEWER_H

#include <cstddef>
#include <string>
#include <vector>

#include "d3d_renderer.h"

#include "../scene.h"
#include "../camera.h"
#include "../viewer.h" // the base class

/*
* d3d_viewer.h
* Viewer class mange a window and setting interactive input by user using
* Direct3D API. It also configure a renderer and provide it with necessary
* infomation to render the window.
*/

namespace CEL {

class D3DViewer : public Viewer {

public:

  // Default counstructor : not allowed
  D3DViewer() = delete;

  // Initialize with window size and title
  D3DViewer(std::size_t window_w, std::size_t window_h, std::string title);

  // Destructor
  ~D3DViewer();

  // Start the update&render loop
  void run() override;

private:

  //WindowID window;

  static int view_count; // count the number of alive window

};

} // namespace CEL


#endif
