cmake_minimum_required (VERSION 3.0)
project (CELENGINE)

#------------------
# Get dependencies
#------------------

# Required OpenGL
find_package(OpenGL REQUIRED)

# Built dependence in deps directory
add_subdirectory("${PROJECT_SOURCE_DIR}/deps")

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
