cmake_minimum_required (VERSION 3.13)
project (REI)


#-----------------------
# Project configurations

option(BUILD_SAMPLES "Build REI with samples projects" ON)

set(core_data_name "CoreData")

if(PROJECT_NAME STREQUAL ${CMAKE_PROJECT_NAME})
	set(is_top_project true)
	set(BUILD_SAMPLES ON)
endif()

# Setup runtime folder
if (is_top_project)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE${PROJECT_SOURCE_DIR}/bin)
	set(CMAKE_DEBUG_POSTFIX "-debug")
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
	add_definitions(-DNOMINMAX)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# using GCC
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
endif()


#------------------------------
# Resolve external dependencies

add_subdirectory_with_folder("Externals" externals)
get_property(external_libs GLOBAL PROPERTY rei_external_libs)
get_property(external_deps GLOBAL PROPERTY rei_external_deps)


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
set(core_library "rei")
add_library(${core_library} STATIC ${core_source} ${core_header})
target_include_directories(${core_library} PRIVATE ${PROJECT_SOURCE_DIR}/src)
# TODO: set up dedicated public include dir for core_library
target_include_directories(${core_library} INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(${core_library} INTERFACE ${PROJECT_SOURCE_DIR}/src/app_utils)
target_compile_definitions(${core_library} PUBLIC $<$<CONFIG:DEBUG>:DEBUG=1>)
if(MSVC) 
target_compile_definitions(${core_library} PUBLIC $<$<CONFIG:DEBUG>:THROW=1>)
endif()
target_compile_features(${core_library} PUBLIC cxx_std_17)


# Link Graphic api libraries
# TODO set up pubblic include header and make this PRIVATE
target_link_libraries(${core_library} PUBLIC ${external_libs})
add_dependencies(${core_library} ${external_deps})
message("Build with external libraries: ${external_libs}")
message("Build with external dependencies: ${external_deps}")

# Link assimp
target_link_libraries(${core_library} PRIVATE assimp)

# Export the core data list to a variable
set(core_data_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${core_data_name}")
set(core_data_source_directories "")
set(core_data_source_files "")
# collect data directories
list(APPEND core_data_source_directories "${PROJECT_SOURCE_DIR}/src/direct3d/shader")
# collect each data file
foreach(core_data_src_dir ${core_data_source_directories})
	file(GLOB_RECURSE files "${core_data_src_dir}/*")
	list(APPEND core_data_source_files ${files})
	# prevent Visual Studio to compiler the shaders unintendedly
    set_source_files_properties(${files} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach()
# create a target for dependecy management and IDE convinience
add_custom_target(CoreData ALL SOURCES ${core_data_source_files} DEPENDS ${core_data_source_files})
message("* Configure exporting core data to ${core_data_path}: ")
# add command to copy each folder
foreach(core_data_src_dir ${core_data_source_directories})
	get_filename_component(dir_name ${core_data_src_dir} NAME)
	message("	- Copy <${core_data_src_dir}> to <${core_data_path}/${dir_name}>")
	add_custom_command(TARGET CoreData POST_BUILD
		DEPENDS ${files}
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${core_data_src_dir}" "${core_data_path}/${dir_name}"
	)
endforeach()
# add to dependency hierarchy
add_dependencies(${core_library} CoreData)


#------------
# Build tests
# todo move sample to sample-projects folders

#enable_testing ()
if(BUILD_SAMPLES)
	add_subdirectory_with_folder("Tests" tests)
endif()


#----------------------
# Build sample projects

# todo seperate samples from tests
if(BUILD_SAMPLES)
	add_subdirectory_with_folder("Samples" samples)
endif()