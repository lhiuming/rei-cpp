# Build all dependency
# You would need to fetch the submodules first

# GLEW (built from here, since the release doesn't come with CMakeLists)
set(GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew/include")
add_library(glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c)
target_include_directories(glew PUBLIC ${GLEW_INCLUDE_DIR})

# GLFW (built using native CMakeLists.txt)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)

# Assimp (built using native CmakeLists.txt)
set(ASSIMP_BUILD_NONFREE_C4D_IMPORTER OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp)
