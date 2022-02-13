set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_GENERATOR_PLATFORM "x64")
cmake_minimum_required(VERSION 3.10)
project(NarvalEngine)

# Project options
option(NE_DEBUG_MODE "Whether the program will be compiled in debug mode." ON)

if(NE_DEBUG_MODE)
	message("DEBUG_MODE is enabled.")
	add_compile_definitions(NE_DEBUG_MODE)
else()
	message("DEBUG_MODE is disabled.")
endif()

# Creates the "defines.h" files based on Cmake options
configure_file(src/defines.h.in src/defines.h)
include_directories(${CMAKE_BINARY_DIR}/src)

# Global definitions
add_compile_definitions(NOMINMAX)

# Using cpp17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")

#----------------------------------------------------------------------
# Vulkan
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Vulkan detected.")
else()
    message(STATUS "Vulkan not found.")
endif()

#----------------------------------------------------------------------
# Magic Enum
include_directories(includes/magic_enum/include)
set(MAGIC_ENUM "includes/magic_enum/include/magic_enum.hpp")

#----------------------------------------------------------------------
# Google Test
enable_testing()
include(GoogleTest)
add_subdirectory(includes/googletest)
include_directories(includes/googletest/googletest)
include_directories(includes/googletest/googletest/include)

#----------------------------------------------------------------------
#Google Logging
add_compile_definitions(GLOG_NO_ABBREVIATED_SEVERITIES)
add_compile_definitions(GLOG_CUSTOM_PREFIX_SUPPORT)
set(WITH_CUSTOM_PREFIX TRUE)
add_subdirectory(includes/glog)
include_directories(includes/glog/src)
# The logging.h file is created in the build directory, so this line below is a workaround to include this file
include_directories(${CMAKE_CURRENT_BINARY_DIR}/includes/glog)

#----------------------------------------------------------------------
##Tracy (DISABLED)
##set should be conditional to debug mode...
#add_definitions(-DTRACY_ENABLE)
##add_subdirectory(includes/tracy)
#include_directories(includes/tracy)
#set(TRACY "includes/tracy/TracyClient.cpp" "includes/tracy/Tracy.hpp")

#----------------------------------------------------------------------
#Optick
#NOTE: Completly removing the miniz source files due to conflicts with assimp.
#Another solution would be to include them and manually set #define OPTICK_ENABLE_COMPRESSION (0) in optick_miniz.h, which is not ideal.
#----------------------------------------------------------------------
add_compile_definitions(OPTICK_ENABLE_GPU_D3D12=0)
include_directories(includes/optick/src)
set(optick_src_path "${NarvalEngine_SOURCE_DIR}/includes/optick/src")
file(
    GLOB_RECURSE OPTICK_SOURCE_FILES 
    LIST_DIRECTORIES false
    "${optick_src_path}/*.c*"
    "${optick_src_path}/*.h*"
)

# Remove miniz.
list(FILTER OPTICK_SOURCE_FILES EXCLUDE REGEX "optick_miniz")

foreach(_source IN ITEMS ${OPTICK_SOURCE_FILES})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${optick_src_path}" "${_source_path}")
endforeach()

#----------------------------------------------------------------------
# RapidJSON
include_directories(includes/rapidjson/include)
SET( RAPIDJSON_LIBRARIES ${RapidJSON_LIBRARY})

#----------------------------------------------------------------------
# GLAD
include_directories(includes/glad/include)
set(GLAD "includes/glad/include/glad/glad.h"
         "includes/glad/src/glad.c")

#----------------------------------------------------------------------
# STB 
include_directories(includes/stb)
set(STB_SOURCES
    "includes/stb/stb_image.h"
    "includes/stb/stb_image_write.h"
)

#----------------------------------------------------------------------
# NanoVDB
# Recently merged into main branch of OpenVDB.
#include_directories(includes/nanovdb)
#set(NANOVDB "includes/nanovdb/NanoVDB.h")

#----------------------------------------------------------------------
# ASSIMP
# Set ASSIMP options and add directory
set(ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE) 
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE) 
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE) 
add_subdirectory(includes/assimp)
include_directories(includes/assimp/include)
# The config.h file is created only in the build directory, so this line below is a workaround to include this file
include_directories(${CMAKE_CURRENT_BINARY_DIR}/includes/assimp/include)

#----------------------------------------------------------------------
# GLM library
add_compile_definitions(GLM_FORCE_XYZW_ONLY)
include_directories(includes/glm)

#----------------------------------------------------------------------
# Fastnoise
include_directories(includes/fastnoise)
set(FASTNOISE_SOURCES
    "includes/fastnoise/FastNoise.h"
    "includes/fastnoise/FastNoise.cpp"
)

#----------------------------------------------------------------------
# tinyfiledialogs
include_directories(includes/tinyfiledialogs)

set(TINYFILEDIALOGS_SOURCES
    "includes/tinyfiledialogs/tinyfiledialogs.h"
    "includes/tinyfiledialogs/tinyfiledialogs.c"
)

#----------------------------------------------------------------------
# ImGui
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)
add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)
include_directories(includes/imgui)

set(IMGUI_SOURCES
    "includes/imgui/imgui.cpp"
    "includes/imgui/imgui_draw.cpp"
    "includes/imgui/imgui_demo.cpp"
    "includes/imgui/imgui_widgets.cpp"
    "includes/imgui/imgui_tables.cpp"
    "includes/imgui/backends/imgui_impl_opengl3.h"
    "includes/imgui/backends/imgui_impl_opengl3.cpp"
    "includes/imgui/backends/imgui_impl_glfw.h"
    "includes/imgui/backends/imgui_impl_glfw.cpp"
    "includes/ImGuizmo/ImGuizmo.h"
    "includes/ImGuizmo/ImGuizmo.cpp"
    "includes/imnodes/imnodes.h"
    "includes/imnodes/imnodes.cpp"
    "includes/imnodes/imnodes_internal.h"
)

#----------------------------------------------------------------------
# ImGui Addons
include_directories(includes/ImGuizmo)
include_directories(includes/imnodes)

#----------------------------------------------------------------------
# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(includes/glfw)
include_directories(includes/glfw/deps) #<glad/gl.h> used by ImGui
include_directories(includes/glfw/include)

# --------------------------------------------------------------------------------------
# Finders

# OpenEXR
include(FindZLIB)
find_package(openexr)

if (ZLIB_FOUND)
	message(STATUS "ZLIB detected.")
    FIND_PATH(OPENEXR_INCLUDE_DIR ImfRgbaFile.h PATH_SUFFIXES OpenEXR PATHS $ENV{OPENEXR_PATH}/include $ENV{CPATH})
    FIND_LIBRARY(OPENEXR_HALF_LIBRARY NAMES Half PATHS $ENV{OPENEXR_PATH}/lib $ENV{LIBRARY_PATH})
    FIND_LIBRARY(OPENEXR_IEX_LIBRARY NAMES Iex PATHS $ENV{OPENEXR_PATH}/lib $ENV{LIBRARY_PATH})
    FIND_LIBRARY(OPENEXR_IMATH_LIBRARY NAMES Imath PATHS $ENV{OPENEXR_PATH}/lib $ENV{LIBRARY_PATH})
    FIND_LIBRARY(OPENEXR_ILMIMF_LIBRARY NAMES IlmImf PATHS $ENV{OPENEXR_PATH}/lib $ENV{LIBRARY_PATH})
    FIND_LIBRARY(OPENEXR_ILMTHREAD_LIBRARY NAMES IlmThread PATHS $ENV{OPENEXR_PATH}/lib $ENV{LIBRARY_PATH})
	
	message(STATUS ${OPENEXR_HALF_LIBRARY})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OpenEXR REQUIRED_VARS
            OPENEXR_INCLUDE_DIR
            OPENEXR_HALF_LIBRARY
            OPENEXR_IEX_LIBRARY
            OPENEXR_IMATH_LIBRARY
            OPENEXR_ILMIMF_LIBRARY
            OPENEXR_ILMTHREAD_LIBRARY)
    
    set(OPENEXR_LIBRARIES
        ${OPENEXR_ILMIMF_LIBRARY}
        ${OPENEXR_ILMTHREAD_LIBRARY}
        ${OPENEXR_IMATH_LIBRARY}
        ${OPENEXR_HALF_LIBRARY}
        ${OPENEXR_IEX_LIBRARY}
        ${ZLIB_LIBRARY})
    
    mark_as_advanced(
            OPENEXR_INCLUDE_DIR
            OPENEXR_HALF_LIBRARY
            OPENEXR_IEX_LIBRARY
            OPENEXR_IMATH_LIBRARY
            OPENEXR_ILMIMF_LIBRARY
            OPENEXR_ILMTHREAD_LIBRARY
            OPENEXR_LIBRARIES)
else()
    message(STATUS "OpenEXR not available: Could not find ZLib.")
endif()

# --------------------------------------------------------------------------------------
# TinyEXR
find_path(TINYEXR_INCLUDE_DIR NAMES tinyexr.h)
find_library(TINYEXR_LIBRARY NAMES tinyexr)
include_directories(${TINYEXR_INCLUDE_DIR})

if (TINYEXR_FOUND)
	message(STATUS "TinyEXR detected.")
else()
    message(STATUS "TinyEXR not detected.")
endif()

# --------------------------------------------------------------------------------------
# OpenVDB
find_path(OPENVDB_INCLUDE_DIR NAMES openvdb/openvdb.h)
find_library(OPENVDB_LIBRARY NAMES openvdb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenVDB REQUIRED_VARS OPENVDB_INCLUDE_DIR OPENVDB_LIBRARY)

SET( OPENVDB_COMPILE_FLAGS)

IF( OPENVDB_FOUND)
	message(STATUS "OpenVDB detected.")
    SET( OPENVDB_LIBRARIES ${OPENVDB_LIBRARY})
    SET( OPENVDB_INCLUDE_DIRS ${OPENVDB_INCLUDE_DIR})
ELSE()
	message(STATUS "OpenVDB not detected.")
    SET( OPENVDB_LIBRARIES)
    SET( OPENVDB_INCLUDE_DIRS)
ENDIF()

mark_as_advanced(OPENVDB_INCLUDE_DIR OPENVDB_LIBRARY)

# --------------------------------------------------------------------------------------
#TBB FINDER
find_path(TBB_INCLUDE_DIR NAMES tbb/tbb.h)
find_library(TBB_LIBRARY NAMES tbb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TBB REQUIRED_VARS TBB_INCLUDE_DIR TBB_LIBRARY)

set(TBB_LIBRARIES ${TBB_LIBRARY})
set(TBB_INCLUDE_DIRS ${TBB_INCLUDE_DIR})

mark_as_advanced(TBB_INCLUDE_DIR TBB_LIBRARY)

# --------------------------------------------------------------------------------------
# OpenVDB

find_package(OpenEXR)
if (OPENEXR_FOUND)
    message(STATUS "OpenEXR detected. Building with .exr support")
    add_definitions(-DOPENEXR_AVAILABLE)
else()
    message(STATUS "OpenEXR not detected. Building without .exr support")
endif()

find_package(OpenVDB)
find_package(Boost)
find_package(TBB)

if (Boost_FOUND)
	add_definitions(-DBOOST_LOG_DYN_LINK=1)
	message(STATUS "BOOST detected.")
else()
	message(STATUS "BOOST not detected.")
endif()

if (TBB_FOUND)
	message(STATUS "TBB detected.")
else()
	message(STATUS "TBB not detected.")
endif()

if (OPENEXR_FOUND AND OPENVDB_FOUND AND TBB_FOUND)
    message(STATUS "OpenVDB detected. Building with .vdb support")
    add_definitions(-DOPENVDB_AVAILABLE)
    include_directories(${OPENVDB_INCLUDE_DIR} ${OPENVDB_INCLUDE_DIRS} ${OPENEXR_INCLUDE_DIR} ${TBB_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} )
    set(core_libs ${OPENVDB_LIBRARIES} ${TBB_LIBRARIES} ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES})
endif()

# --------------------------------------------------------------------------------------
# Sources files
set(_src_root_path "${NarvalEngine_SOURCE_DIR}/src/")
file(
    GLOB_RECURSE SOURCE_FILES 
    LIST_DIRECTORIES false
    "${_src_root_path}/*.c*"
    "${_src_root_path}/*.h*"
)

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${_src_root_path}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

include_directories( "${CMAKE_SOURCE_DIR}/src" )

set(NE_ALL_LIBRARIES TBB::tbb Vulkan::Vulkan ${core_libs} assimp ${RAPIDJSON_LIBRARIES} ${TINYEXR_LIBRARIES} glog::glog glfw ${GLFW_LIBRARIES})
set(NE_ALL_SOURCES ${SOURCE_FILES} ${OPTICK_SOURCE_FILES} ${MAGIC_ENUM} ${GLAD} ${NANOVDB} ${IMGUI_SOURCES} ${TINYFILEDIALOGS_SOURCES} ${FASTNOISE_SOURCES} ${STB_SOURCES})

# --------------------------------------------------------------------------------------
# Correct lib/dll folder output
set_target_properties(glfw
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_target_properties(assimp
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_target_properties(glog
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# --------------------------------------------------------------------------------------
# Executable and linking
add_executable(NarvalEngine ${NE_ALL_SOURCES} )
target_link_libraries(NarvalEngine ${NE_ALL_LIBRARIES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NarvalEngine)

# --------------------------------------------------------------------------------------
# Google testing project (Not working yet)
set(GOOGLETEST_SRC "includes/googletest/googletest/src/gtest-all.cc")
set(NE_TESTS_SRC "unitTests/tests.h" "unitTests/tests.cpp")

add_library(NELib STATIC ${NE_ALL_SOURCES})
add_library(NeLibInterface INTERFACE)
target_link_libraries(NeLibInterface INTERFACE ${NE_ALL_LIBRARIES} )

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_executable(NarvalEngineTests ${NE_TESTS_SRC} ${GOOGLETEST_SRC})
target_link_libraries(NarvalEngineTests NELib NeLibInterface )
gtest_discover_tests(NarvalEngineTests)
target_compile_definitions(NELib PUBLIC USE_OPTICK=0) #Disable optick for tests.
target_compile_definitions(NeLibInterface INTERFACE WITH_GTEST=OFF) #Disable glog with GTEST.
