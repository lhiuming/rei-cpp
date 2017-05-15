# Version of the library that depens on OpenGL 

# pixels: the wrapper with GL libraries
add_library(pixels pixels.cpp pixels.h)
target_link_libraries(pixels glew glfw ${OPENGL_gl_LIBRARY})

# viewer
add_library(viewer viewer.cpp viewer.h)
target_link_libraries(viewer pixels renderer scene)

# renderer : OpenGL
add_library(renderer renderer.cpp renderer.h)
target_link_libraries(renderer algebra scene)
