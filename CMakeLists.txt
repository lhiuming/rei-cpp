cmake_minimum_required (VERSION 3.1)
project (CELENGINE)

#-------------------
# Solve dependencies
#-------------------

# Built dependence in deps directory
add_subdirectory("${PROJECT_SOURCE_DIR}/deps")


#----------------
# Set up Compiler
#----------------

# Use C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile-specific stuffs
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

	set(CMAKE_CXX_FLAGS "-g -O3 -Wall")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	add_definitions(-DUSE_MSVC)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

  # using GCC

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")

  # using Intel C++

endif()


#--------------------
# Add subdirectories
#--------------------

# Include idrectory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Build the source
add_subdirectory(src)

# Build the tests
enable_testing ()
add_subdirectory(tests)
