cmake_minimum_required (VERSION 3.0)
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

#--------------------
# Add subdirectories
#--------------------

# Include idrectory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Set compile flags
set(CMAKE_CXX_FLAGS "-g -O3 -Wall -std=c++11")


# Build the source
add_subdirectory(src)

# Build the tests
enable_testing ()
add_subdirectory(tests)
