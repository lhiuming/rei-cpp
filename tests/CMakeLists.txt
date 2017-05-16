# All unit test for the Cel project

#---------------------------------------
# Test the GL deps and related libraries
#---------------------------------------

# GL libraries
set(GL_LIBS glew glfw ${OPENGL_gl_LIBRARY})

# we can use for each to compile a bunch of similar test programs
set(GL_lib_test play_with_glfw hello_rectangle hello_texture)
foreach(program ${GL_lib_test})
  add_executable(${program} ${program}.cpp)
  target_link_libraries(${program} ${GL_LIBS})
endforeach(program)

#-----------------------
# Test the D3D libraries
#-----------------------

# Yes, we dont have any yet.

#-----------------------------
# Test some utility libraries
#-----------------------------

# assimp
add_executable(try_assimp try_assimp.cpp)
target_link_libraries(try_assimp assimp)

#----------------------
# Test the CEL modules
#----------------------

include_directories(${CELENGINE_SOURCE_DIR}/src)

# Test pixels
add_executable(test_pixels test_pixels.cpp)
target_link_libraries(test_pixels pixels)

# Test algebra
add_executable(test_algebra test_algebra.cpp)
target_link_libraries(test_algebra algebra)

# Test color
add_executable(test_color test_color.cpp)
target_link_libraries(test_color color)

# Test model
add_executable(test_model test_model.cpp)
target_link_libraries(test_model model)

# Test camera
add_executable(test_camera test_camera.cpp)
target_link_libraries(test_camera camera)

# Test asset_loader
add_executable(test_asset_loader test_asset_loader.cpp)
target_link_libraries(test_asset_loader asset_loader model)

# Test viewer
add_executable(test_viewer test_viewer.cpp)
target_link_libraries(test_viewer viewer camera scene renderer)

#-----------------------------
# Testing modules integration
#-----------------------------

# Three triangle test (for color and z-buffer)
add_executable(three_triangle three_triangle.cpp)
target_link_libraries(three_triangle model scene camera viewer renderer)


# Three triangle test (for color and z-buffer)
add_executable(draw_cube draw_cube.cpp)
target_link_libraries(draw_cube model scene camera viewer renderer asset_loader)
