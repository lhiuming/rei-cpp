# Solve all dependencies
# In order to build some of them, you might need to fetch the submodules first

include(ExternalProject)

#------------------------------------------------------------------------------
# OpenGL stuffs

IF (UNIX)

# OpenGL
find_package(OpenGL REQUIRED)

# GLEW (built from here, since the release doesn't come with CMakeLists)
#add_library(glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c)
#set(GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew/include")
#target_include_directories(glew PUBLIC ${GLEW_INCLUDE_DIR})

# GLEW (built using native CMakeList.txt
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS build)
ExternalProject_Add(glew_project
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/glew
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glew/build/cmake
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/glew
	CMAKE_ARGS
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/glew
		-DCMAKE_INSTALL_LIBDIR=${CMAKE_CURRENT_BINARY_DIR}/glew/lib
		-DCMAKE_BUILD_TYPE=Release
	CMAKE_CACHE_ARGS
		-DBUILD_UTILS:BOOL=OFF
		-DGLEW_REGAL:BOOL=OFF
		-DGLEW_OSMESA:BOOL=OFF
	  -DBUILD_FRAMEWORK:BOOL=OFF
		-DBUILD_SHARED_LIBS:BOOL=TRUE
  STEP_TARGETS build
  DEPENDEES glew
)
#ExternalProject_Add_Step(glew_project copy_lib
#  COMMAND ${CMAKE_COMMAND} -E copy_directory
#		${CMAKE_CURRENT_BINARY_DIR}/glew/lib
#		${PROJECT_BINARY_DIR}/lib
#  DEPENDEES install
#)
add_library(glew SHARED IMPORTED)
set_target_properties(glew PROPERTIES IMPORTED_LOCATION
	${CMAKE_CURRENT_BINARY_DIR}/glew/lib/libGLEW.dylib)
set(glew_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew/include")
set(glew_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/glew/lib")

# GLFW (built using native CMakeLists.txt)
# TODO: use extrenal project
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)

# set the collective library
set(GL_LIBRARY glew glfw ${OPENGL_gl_LIBRARY} CACHE STRING "")

ENDIF (UNIX)


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

ENDIF (WIN32)


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
# fetch the DLLs
get_property(ASSIMP_OUTPUT TARGET assimp PROPERTY OUTPUT_NAME)
set(ASSIMP_OUTPUT_NAME ${ASSIMP_OUTPUT}.dll CACHE STRING "")
set(ASSIMP_OUTPUT_PATH
 ${CMAKE_CURRENT_BINARY_DIR}/assimp/code/${CMAKE_CONFIGURATION_TYPES}/)
add_custom_target(post_assimp DEPENDS assimp)
add_custom_command(
	TARGET post_assimp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		"${ASSIMP_OUTPUT_PATH}/${ASSIMP_OUTPUT_NAME}"
		"${PROJECT_BINARY_DIR}/lib"
	)
