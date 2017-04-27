# Build the Cel project library

# pixels: the wrapper with GL libraries
add_library(pixels pixels.cpp pixels.h) # do this need pixels.h ?
target_link_libraries(pixels glew glfw ${OPENGL_gl_LIBRARY})

# algebra (used to call algebra ... )
add_library(algebra algebra.cpp algebra.h)

# model
add_library(model model.cpp model.h)
target_link_libraries(model algebra)

# scene
add_library(scene scene.cpp scene.h)
target_link_libraries(scene algebra model)

# camera
add_library(camera camera.cpp camera.h)
target_link_libraries(camera algebra)

# renderer
add_library(renderer renderer.cpp renderer.h)
target_link_libraries(renderer algebra scene)

# viewer
add_library(viewer viewer.cpp viewer.h)
target_link_libraries(viewer pixels renderer scene)

# Build the whole library ?
#add_library(Cel ...)
