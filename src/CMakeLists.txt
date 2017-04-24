# Build the Cel project library

# pixels: the wrapper with GL libraries
add_library(pixels pixels.cpp pixels.h) # do this need pixels.h ?
target_link_libraries(pixels glew glfw ${OPENGL_gl_LIBRARY})

# math
add_library(math math.h)

# renderer
add_library(renderer renderer.cpp renderer.h)

# scene
add_library(scene scene.cpp scene.h)

# viewer
add_library(viewer viewer.cpp viewer.h)
target_link_libraries(viewer pixels renderer scene)

# Build the whole library ?
#add_library(Cel pixels)
