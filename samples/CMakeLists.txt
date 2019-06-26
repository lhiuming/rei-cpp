# Sample projects for demostration or documentation. 

#------------------------
# Some simple sample apps
# -----------------------

add_executable(procedural_mesh procedural_mesh.cpp)
target_link_libraries(procedural_mesh ${core_library})
set_property(TARGET procedural_mesh PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})