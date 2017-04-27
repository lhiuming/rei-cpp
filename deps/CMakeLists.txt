# Build all dependency
# You would need to fetch the submodules first

# GLEW (built from here, because GLEW doesn't come with a CMakeLists.txt)
set(GLEW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew")
add_library(glew ${GLEW_ROOT_DIR}/src/glew.c)
target_include_directories(glew ${GLEW_ROOT_DIR}/include)

# GLFW (built using native CMakeLists.txt)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glfw")
