cmake_minimum_required (VERSION 3.13)
# TODO rename this
project (CELENGINE)


#-----------------------
# Project configurations

# todo add system detection and build config
option(BUILD_OPENGL "Build Rei with OpenGL support" OFF)
option(BUILD_DIRECT3D "Build Rei with Direct3D support" ON)

set(core_data_name "CoreData")

if(PROJECT_NAME STREQUAL ${CMAKE_PROJECT_NAME})
	set(is_top_project true)
endif()

# Organize targets in IDE
if(is_top_project)
	SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
	define_property(
		TARGET 
		PROPERTY FOLDER INHERITED
		BRIEF_DOCS "Set the folder name."
    	FULL_DOCS  "Use to organize targets in an IDE."
	)
endif()
function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()


#----------------
# Set up Compiler
# TODO remove this, and set compling flag on each target

# Use C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile-specific stuffs
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "-g -O3 -Wall")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions(-DUSE_MSVC)
	add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# using GCC
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
endif()


#------------------------------
# Resolve external dependencies

add_subdirectory_with_folder("Externals" externals)


#-----------------------
# Build the core library

# Fetch headers and source files
# TODO change to explicit file list
file(GLOB core_source "src/*.cpp" "src/*/*.cpp")
file(GLOB core_header "src/*.h" "src/*/*.h")
source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${core_source})
source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${core_header})

# Build the library
# NOTE: headers are added to source list for IDE file grouping
# NOTE: only support static linking
# TODO: set up dedicated public include dir for core_library
set(core_library "rei")
add_library(${core_library} STATIC ${core_source} ${core_header})
target_include_directories(${core_library} PUBLIC ${PROJECT_SOURCE_DIR}/src)

# Link Graphic api libraries
set(graphic_api_libs "")
if (BUILD_OPENGL)
	target_compile_definitions(${core_library} PRIVATE OPENGL_ENABLED)
	list(APPEND graphic_api_libs ${opengl_libs})
endif()
if (BUILD_DIRECT3D)
	target_compile_definitions(${core_library} PRIVATE DIRECT3D_ENABLED)
	list(APPEND graphic_api_libs ${direct3d_libs})
endif()
message("Build with graphic API libraries: ${graphic_api_libs}")
target_link_libraries(${core_library} PRIVATE ${graphic_api_libs})

# Link assimp
target_link_libraries(${core_library} PRIVATE assimp)

# Export the core data list to a variable
# TODO bind this property to rei library targer 
set(core_data_path "${CMAKE_CURRENT_BINARY_DIR}/${core_data_name}")
set(core_data_sources "")
if(BUILD_OPENGL)
	list(APPEND core_data_sources "${PROJECT_SOURCE_DIR}/src/opengl/shader")
endif()
if(BUILD_DIRECT3D)
	list(APPEND core_data_sources "${PROJECT_SOURCE_DIR}/src/direct3d/shader")
endif()
message("Configure exporting core data to ${core_data_path}: ")
foreach(core_data ${core_data_sources})
	message("	${core_data}")
	get_filename_component(dir_name ${core_data} NAME)
	add_custom_command(TARGET ${core_library} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${core_data_sources}" "${core_data_path}/${dir_name}")
endforeach()


#------------
# Build tests
# todo move sample to sample-projects folders

#enable_testing ()
add_subdirectory_with_folder("Tests" tests)


#----------------------
# Build sample projects

# todo seperate samples from tests
#add_subdirectory_with_folder("Samples" samples)