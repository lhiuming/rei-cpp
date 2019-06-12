cmake_minimum_required (VERSION 3.1)
# TODO rename this
project (CELENGINE)


#-----------------------
# Project configurations

# todo add system detection and build config
set(BUILD_OPENGL false)
set(BUILD_DIRECT3D true)

set(core_data_name "CoreData")


#----------------
# Set up Compiler

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


#---------------------------
# Add 3rd-party dependencies

# Build dependence libraries
add_subdirectory("${PROJECT_SOURCE_DIR}/deps")

# Add 3rd-party headers
include_directories("${PROJECT_SOURCE_DIR}/deps/assimp/include")

# Graphic api libraries
set(graphic_api_libs "")
if (BUILD_OPENGL)
	list(APPEND graphic_api_libs ${opengl_libs})
endif()
if (BUILD_DIRECT3D)
	list(APPEND graphic_api_libs ${direct3d_libs})
endif()
message("Build with graphic API libraries: ${graphic_api_libs}")

#-----------------------
# Build the core library

# Platform configs
if(BUILD_OPENGL)
	add_definitions(-D OPENGL_ENABLED)
endif()
if(BUILD_DIRECT3D)
	add_definitions(-D DIRECT3D_ENABLED)
endif()

# Fetch headers and source files
# TODO change to explicit file list
file(GLOB core_source "src/*.cpp" "src/*/*.cpp")
file(GLOB core_header "src/*.h" "src/*/*.h")
source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${core_source})
source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${core_header})

# Build the library
# NOTE: add header list for IDE file grouping
set(core_library "rei")
add_library(${core_library} STATIC ${core_header} ${core_source})
target_link_libraries(${core_library} ${graphic_api_libs})

# Export the core data list to a variable
set(core_data_path "${CMAKE_CURRENT_BINARY_DIR}/${core_data_name}")
set(core_data_sources "")
if(BUILD_OPENGL)
	list(APPEND core_data_sources "${PROJECT_SOURCE_DIR}/src/opengl/shader")
endif()
if(BUILD_DIRECT3D)
	list(APPEND core_data_sources "${PROJECT_SOURCE_DIR}/src/direct3d/shader")
endif()
message("Configure exporting of core data to ${core_data_path}: ")
foreach(core_data ${core_data_sources})
	message("	${core_data}")
	get_filename_component(dir_name ${core_data} NAME)
	add_custom_command(TARGET ${core_library} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${core_data_sources}" "${core_data_path}/${dir_name}")
endforeach()

#------------
# Build tests
# todo move sample to sample-projects folders
#enable_testing ()
add_subdirectory(tests)


#----------------------
# Build sample projects
# check this: set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# todo seperate samples from tests