# Build the Cel project library

#------------------
# Platform dependent
#------------------

# OpenGL ----

IF (UNIX)

add_definitions(-DUSE_OPENGL)

# pixels: the wrapper with GL libraries
add_library(pixels opengl/pixels.cpp opengl/pixels.h)
target_link_libraries(pixels ${GL_LIBRARY})

# renderer : OpenGL
add_library(renderer_imp opengl/gl_renderer.cpp opengl/gl_renderer.h)
target_link_libraries(renderer_imp pixels algebra scene model color)

# viewer
add_library(viewer_imp opengl/gl_viewer.cpp opengl/gl_viewer.h)
target_link_libraries(viewer_imp pixels renderer scene)

ENDIF()


# Direct3D ----

IF (WIN32)

add_definitions(-DUSE_DIRECT3D)

# renderer : DIrect3D 
add_library(renderer_imp direct3d/d3d_renderer.cpp direct3d/d3d_renderer.h)
target_link_libraries(renderer_imp algebra scene model color)

# viewer
add_library(viewer_imp direct3d/d3d_viewer.cpp direct3d/d3d_viewer.h)
target_link_libraries(viewer_imp renderer scene)

ENDIF()

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
