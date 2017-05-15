# Build all dependency
# You would need to fetch the submodules first

#------------------------------------------------------------------------------
# GLEW (built from here, since the release doesn't come with CMakeLists)

# add the library
add_library(glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c)

# populate the include directory property
set(GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew/include")
target_include_directories(glew PUBLIC ${GLEW_INCLUDE_DIR})

#------------------------------------------------------------------------------
# GLFW (built using native CMakeLists.txt)

# build settings
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# build it; include directory is populated automatically
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)

#------------------------------------------------------------------------------
# Assimp (built using native CmakeLists.txt)

# build settings
set(ASSIMP_BUILD_NONFREE_C4D_IMPORTER OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(NO_EXPORT ON CACHE BOOL "")
set(OPT_BUILD_PACKAGES OFF CACHE BOOL "")


# build it
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp)

# populate the include directory property
set(ASSIMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include")
target_include_directories(assimp PUBLIC ${ASSIMP_INCLUDE_DIR})
