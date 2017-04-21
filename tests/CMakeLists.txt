# All unit test for the Cel project

#-----------------------
# Test the GL libraries
#-----------------------

# GL libraries
set(GL_LIBS glew glfw ${OPENGL_gl_LIBRARY})

# we can use for each to compile a bunch of similar test programs
set(GL_lib_test play_with_glfw hello_rectangle hello_texture)
foreach(program ${GL_lib_test})
  add_executable(${program} ${program}.cpp)
  target_link_libraries(${program} ${GL_LIBS})
endforeach(program)

#----------------------
# Test the CEL modules
#----------------------

include_directories(${CELENGINE_SOURCE_DIR}/src)

# Test pixels
add_executable(test_pixels test_pixels.cpp)
target_link_libraries(test_pixels pixels)

add_test (NAME pixels COMMAND test_pixels)
