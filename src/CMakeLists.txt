# Build the Cel project library

#------------------
# Platform dependent
#------------------

# OpenGL ----

# pixels: the wrapper with GL libraries
add_library(pixels opengl/pixels.cpp opengl/pixels.h)
target_link_libraries(pixels glew glfw ${OPENGL_gl_LIBRARY})

# viewer
add_library(viewer_imp opengl/gl_viewer.cpp opengl/gl_viewer.h)
target_link_libraries(viewer_imp pixels renderer scene)

# renderer : OpenGL
add_library(renderer_imp opengl/gl_renderer.cpp opengl/gl_renderer.h)
target_link_libraries(renderer_imp pixels algebra scene model color)


# Direct3D

#----------------
# Cross platform
#----------------

# algebra (used to call math ... )
add_library(algebra algebra.cpp algebra.h)

# color
add_library(color color.cpp color.h)

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
target_link_libraries(renderer renderer_imp)

# viewer
add_library(viewer viewer.cpp viewer.h)
target_link_libraries(viewer viewer_imp)

# assert loader
add_library(asset_loader asset_loader.cpp asset_loader.h)
target_link_libraries(asset_loader assimp color model)
