cmake_minimum_required (VERSION 2.6)
project (CELENGINE)

#find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

set(CMAKE_CXX_FLAGS "-g -Wall")

# Buid the tests
add_subdirectory(tests)
