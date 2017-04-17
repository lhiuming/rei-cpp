cmake_minimum_required (VERSION 2.6)
project (CELENGINE)

# Core libraries
find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)

set(CORELIBS ${OPENGL_LIBRARY} ${OPENGL_gl_LIBRARY})
include_directories(${OPENGL_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-g -Wall")

# Buid the tests
add_subdirectory(tests)
