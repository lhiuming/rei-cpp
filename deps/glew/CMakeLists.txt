# GLEW doesn't come with a CMakeLists.txt
# This file is created to build GLEW with the app

# Build glew. See Makefile in glew/
set(GLEW_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${GLEW_ROOT_DIR}/include)
add_library(glew ${GLEW_ROOT_DIR}/src/glew.c)
