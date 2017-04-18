cmake_minimum_required (VERSION 2.6)
project (CELENGINE)

#------------------
# Get dependencies
#------------------

# Required OpenGL
find_package(OpenGL REQUIRED)

# GLEW (built) using written CMakeLists.txt
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/glew")

# GLFW (built) using native CMakeLists.txt
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/glfw")

#--------------
# Set includes
#--------------
include_directories(${PROJECT_SOURCE_DIR}/include)

#-------------------
# Add executables
#-------------------

set(CMAKE_CXX_FLAGS "-g -Wall")

# Build the tests
add_subdirectory(tests)
