# The test for playing with cmake and glfw

# set up the directory accoding to the project root
include_directories(${CELENGINE_SOURCE_DIR}/tests)
link_directories(${CELENGINE_BINARY_DIR}/tests)

# Core libraries
set(CORELIBS glew glfw ${OPENGL_gl_LIBRARY})

# we can use for each to compile a bunch of similar test programs
set(TESTPROGRAMS play_with_glfw logl_work)
foreach(program ${TESTPROGRAMS})
  add_executable(${program} ${program}.cpp)
  target_link_libraries(${program} ${CORELIBS})
endforeach(program)

# we can build a library and link it this way
# add_library(somelib somelib.cpp)
# add_executable(use_lib use_lib.cpp)
# target_link_libraries(use_lib ${CORELIBS} somelib)
