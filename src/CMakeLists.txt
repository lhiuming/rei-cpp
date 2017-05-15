# Build the Cel project library

#------------------
# OpenGL dependent
#------------------

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/opengl")

#--------------------
# Direct3D dependent
#--------------------

# Not done, yet.

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

# assert loader
add_library(asset_loader asset_loader.cpp asset_loader.h)
target_link_libraries(asset_loader assimp)
