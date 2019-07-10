# Sample projects for demostration or documentation. 

#------------------------
# Some simple sample apps
# -----------------------

add_executable(hello_cube hello_cube.cpp)
target_link_libraries(hello_cube ${core_library})
set_property(TARGET hello_cube PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_executable(procedural_mesh procedural_mesh.cpp)
target_link_libraries(procedural_mesh ${core_library})
set_property(TARGET procedural_mesh PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_executable(rt_raytracing_demo rt_raytracing_demo.cpp)
target_link_libraries(rt_raytracing_demo ${core_library})
if(BUILD_DIRECT3D)
target_compile_definitions(rt_raytracing_demo PRIVATE DIRECT3D_ENABLED)
endif()
set_property(TARGET rt_raytracing_demo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})