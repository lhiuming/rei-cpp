# Build the Cel project library

# pixels: the wrapper with GL libraries
add_library(pixels pixels.cpp) # do this need pixels.h ?
target_link_libraries(pixels glew glfw ${OPENGL_gl_LIBRARY})

# Build the whole library
#add_library(Cel pixels)
