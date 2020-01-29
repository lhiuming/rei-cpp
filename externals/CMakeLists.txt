# Solve all dependencies
# In order to build some of them, you might need to fetch the submodules first

#include(ExternalProject)

# suprress warning for deps
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

get_property(external_libs GLOBAL PROPERTY rei_external_libs)
get_property(external_deps GLOBAL PROPERTY rei_external_deps)


#------------------------------------------------------------------------------
# Direct3D stuffs

add_library(d3dutils INTERFACE)
target_include_directories(d3dutils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/d3dutils)

add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/dxc/dxcompiler.dll} )

add_custom_target(d3d_redist ALL 
	${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dxc/dxcompiler.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dxcompiler.dll
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dxc/dxil.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dxil.dll
)

# for new windows (10), the lib is in standrad search path, no need to find
set(direct3d_libs d3dutils dxcompiler d3d12.lib DXGI.lib d3dcompiler.lib)
set(direct3d_deps d3d_redist)

list(APPEND external_libs ${direct3d_libs})
list(APPEND external_deps ${direct3d_deps})


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
list(APPEND external_libs assimp)


#----------------------------------------------------------------------------
# ImGUI sources

add_library(dear_imgui STATIC 
	"imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_widgets.cpp"
)
target_include_directories(dear_imgui PUBLIC "imgui/")
list(APPEND external_libs dear_imgui)

# --- End of Resolving Externals --- #
# Propagate the list to toplevel project
set_property(GLOBAL PROPERTY rei_external_libs ${external_libs})
set_property(GLOBAL PROPERTY rei_external_deps ${external_deps})