# Solve all dependencies
# In order to build some of them, you might need to fetch the submodules first

include(ExternalProject)

# suprress warning for deps
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

#------------------------------------------------------------------------------
# OpenGL stuffs

if(BUILD_OPENGL)
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
	set(opengl_libs glew glfw ${OPENGL_gl_LIBRARY} CACHE STRING "")

endif(BUILD_OPENGL)


#------------------------------------------------------------------------------
# Direct3D stuffs

if(BUILD_DIRECT3D)

	add_library(d3dutils INTERFACE)
	target_include_directories(d3dutils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/d3dutils)

	# for new windows (10), the lib is in standrad search path, no need to find
	set(direct3d_libs d3dutils d3d11.lib d3dcompiler.lib CACHE STRING "")

endif(BUILD_DIRECT3D)


#------------------------------------------------------------------------------
# Assimp (using native CmakeLists.txt provide in the source)

# need to set cache for lower version cmake, which is used by assimp
set(ASSIMP_BUILD_NONFREE_C4D_IMPORTER OFF CACHE BOOL "Assimp stripping") 
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Assimp stripping")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Assimp stripping")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Assimp stripping")
set(NO_EXPORT ON CACHE BOOL "Assimp stripping") 
set(OPT_BUILD_PACKAGES OFF CACHE BOOL "Assimp stripping") 
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable dyanmic linking for assimp")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp EXCLUDE_FROM_ALL)
# populate the include directory property, a.k.a modernizing this legacy target
target_include_directories(assimp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include) 
