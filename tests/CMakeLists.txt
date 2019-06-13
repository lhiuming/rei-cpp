# All unit test for the Cel project

#---------------------------------------
# Test the GL deps and related libraries
#---------------------------------------

IF (BUILD_OPENGL)

# GL libraries
set(GL_LIBS glew glfw ${OPENGL_gl_LIBRARY})

# we can use for each to compile a bunch of similar test programs
set(GL_lib_test play_with_glfw hello_rectangle hello_texture)
foreach(program ${GL_lib_test})
  add_executable(${program} ${program}.cpp)
  target_link_libraries(${program} ${GL_LIBS})
endforeach(program)

# Test pixels
#add_executable(test_pixels test_pixels.cpp)
#target_link_libraries(test_pixels pixels)

ENDIF()


#-----------------------
# Test the D3D libraries
#-----------------------

IF(BUILD_DIRECTX)

add_executable(test_d3d11 WIN32 test_d3d11.cpp)
target_link_libraries (test_d3d11 ${DirectX11_LIBRARY})

# copy the shader file
file(COPY ${CMAKE_CURRENT_ROOT_DIR}test_effects.hlsl
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIGURATION_TYPES}")

# copy the .dll for all tests
file(COPY ${PROJECT_BINARY_DIR}/lib/d3dcompiler_47.dll
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIGURATION_TYPES}")

# also copy the Core shaders for testing
file(COPY ${PROJECT_SOURCE_DIR}/src/direct3d/shader.hlsl
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIGURATION_TYPES}/direct3d")

ENDIF()


#-----------------------------
# Test some utility libraries
#-----------------------------

# assimp
add_executable(try_assimp try_assimp.cpp)
target_link_libraries(try_assimp assimp ${core_library})

# copy assimp dll for windows
IF (MSVC14)
#file(COPY ${PROJECT_BINARY_DIR}/lib/${ASSIMP_OUTPUT_NAME} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIGURATION_TYPES}")
ENDIF(MSVC14)

#----------------------
# Test the CEL modules
#----------------------

#[[
include_directories(${CELENGINE_SOURCE_DIR}/src)

# Test algebra
add_executable(test_algebra test_algebra.cpp)
target_link_libraries(test_algebra algebra console)

# Test color
add_executable(test_color test_color.cpp)
target_link_libraries(test_color color console)

# Test model
add_executable(test_model test_model.cpp)
target_link_libraries(test_model model console)

# Test camera
add_executable(test_camera test_camera.cpp)
target_link_libraries(test_camera camera console)

# Test asset_loader
add_executable(test_asset_loader test_asset_loader.cpp)
target_link_libraries(test_asset_loader
asset_loader scene camera model console)

# Test viewer
add_executable(test_viewer test_viewer.cpp)
target_link_libraries(test_viewer viewer camera scene renderer console)
]]

#-----------------------------
# Sample Projects 
# Serves to test modules integration
#-----------------------------

# Three triangle test (for color and z-buffer)
add_executable(three_triangle three_triangle.cpp)
target_link_libraries(three_triangle ${core_library})
add_custom_command(TARGET three_triangle POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${core_data_path}" "$<TARGET_FILE_DIR:three_triangle>/${core_data_name}")
if(MSVC)
	set_property(TARGET three_triangle PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:three_triangle>")
endif()

# Draw Cube test (model loading overall)
add_executable(draw_model draw_model.cpp)
target_link_libraries(draw_model ${core_library})

# Draw world test (almost everything)
add_executable(draw_world draw_world.cpp)
target_link_libraries(draw_world ${core_library})
