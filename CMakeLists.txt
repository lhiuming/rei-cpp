cmake_minimum_required (VERSION 2.6)
project (CELENGINE)

#------------------
# Get dependencies
#------------------

# Required to be installed
find_package(OpenGL REQUIRED)

# GLFW (built)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/glfw")
include_directories("${PROJECT_SOURCE_DIR}/deps/glfw/include")

# TODO: GLEW (built)


#--------------
# Set includes
#--------------
include_directories(${OPENGL_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-g -Wall")

# Build the tests
add_subdirectory(tests)
