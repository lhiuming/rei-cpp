# Solve all dependencies 
# In order to build some of them, you might need to fetch the submodules first


#------------------------------------------------------------------------------
# OpenGL stuffs 

IF (UNIX)

# OpenGL
find_package(OpenGL REQUIRED)

# GLEW (built from here, since the release doesn't come with CMakeLists)
add_library(glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c)
set(GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew/include")
target_include_directories(glew PUBLIC ${GLEW_INCLUDE_DIR})

# GLFW (built using native CMakeLists.txt)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)

set(GL_LIBRARY glew glfw ${OPENGL_gl_LIBRARY} CACHE STRING "") 

ENDIF()


#------------------------------------------------------------------------------
# Direct3D stuffs 

IF (WIN32)

# find the window sdk path
# TODO: support more version
find_path(WIN_SDK_DIR NAMES Include/um/d3d11.h
	HINTS "$ENV{ProgramFiles\(x86\)}/Windows Kits/8.1/" 
	)

# copy the d3dcompiler dll
file(COPY ${WIN_SDK_DIR}/Redist/D3D/x86/d3dcompiler_47.dll 
	DESTINATION  ${PROJECT_BINARY_DIR}/lib
	)	

set(DirectX11_LIBRARY d3d11.lib d3dcompiler.lib CACHE STRING "") 

ENDIF()


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