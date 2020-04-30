list(APPEND CMAKE_MODULE_PATH "C:/vcpkg/package/openvdb_x64-windows/share/openvdb")
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_GENERATOR_PLATFORM "x64")
cmake_minimum_required(VERSION 3.9)
project(NarvalEngine)

configure_file(src/defines.h.in src/defines.h)
include_directories(${CMAKE_BINARY_DIR}/src)

# Using cpp17
set(CMAKE_CXX_STANDARD 17)

# Build only dependencies of GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# GLFW paths
add_subdirectory(includes/glfw)
include_directories(includes/glfw)
include_directories(includes/glfw/deps)
include_directories(includes/glfw/include)

# GLFW executables.
include_directories(includes/glad/include/glad)
include_directories(includes/glad/include/KHR)
include_directories(includes/glad/include)
set(GLAD "includes/glad/include/glad/glad.h"
         "includes/glad/src/glad.c")

# STB loaders
include_directories(includes/stb)

# ASSIMP
# Set ASSIMP options and add directory
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
add_subdirectory(includes/assimp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inncludes/assimp/include)

# Debug!: For some reason the config.h file is created only in
# the build directory, so this line below is a workaround to include
# this file
include_directories(${CMAKE_CURRENT_BINARY_DIR}/includes/assimp/include)


# GLM library
include_directories(includes/glm)

# GLM fastnoise
include_directories(includes/fastnoise)

# Imgui paths
include_directories(includes/imgui)
include_directories(includes/imgui/examples)

# Imgui define options
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)

set(IMGUI_SOURCES
    "includes/imgui/imgui.cpp"
    "includes/imgui/imgui_draw.cpp"
    "includes/imgui/imgui_demo.cpp"
    "includes/imgui/imgui_widgets.cpp"
    "includes/imgui/examples/imgui_impl_glfw.cpp"
    "includes/imgui/examples/imgui_impl_opengl3.cpp"
)

set(FASTNOISE_SOURCES
    "includes/fastnoise/FastNoise.h"
    "includes/fastnoise/FastNoise.cpp"
)

set(STB_SOURCES
    "includes/stb/stb_image.h"
)

# --------------------------------------------------------------------------------------
# Finders

# OPENEXR FINDER
include(FindZLIB)
find_package(openexr)

if (ZLIB_FOUND)
	message(STATUS "ZLIB detected. :)")
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
    message(STATUS "OpenEXR not available: Could not find ZLib")
endif()



#OPENVDB FINDER
find_path(OPENVDB_INCLUDE_DIR NAMES openvdb/openvdb.h)
find_library(OPENVDB_LIBRARY NAMES openvdb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenVDB REQUIRED_VARS OPENVDB_INCLUDE_DIR OPENVDB_LIBRARY)

SET( OPENVDB_COMPILE_FLAGS)

IF( OPENVDB_FOUND)
	message(STATUS "OK openvdb_found")
    SET( OPENVDB_LIBRARIES ${OPENVDB_LIBRARY})
    SET( OPENVDB_INCLUDE_DIRS ${OPENVDB_INCLUDE_DIR})
ELSE()
	message(STATUS "ERRO openvdb_found")
    SET( OPENVDB_LIBRARIES)
    SET( OPENVDB_INCLUDE_DIRS)
ENDIF()

mark_as_advanced(OPENVDB_INCLUDE_DIR OPENVDB_LIBRARY)

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
    message(STATUS "No OpenEXR detected. Building without .exr support")
endif()

find_package(OpenVDB)
find_package(Boost)
find_package(TBB)
if (Boost_FOUND)
	add_definitions(-DBOOST_LOG_DYN_LINK=1)
	message(STATUS "BOOST Found.")
endif()

if (OPENEXR_FOUND AND OPENVDB_FOUND AND TBB_FOUND)
    message(STATUS "OpenVDB detected. Building with .vdb support")
    add_definitions(-DOPENVDB_AVAILABLE)
	#add_definitions(-DOPENVDB_3_ABI_COMPATIBLE)
	#add_definitions(-DOPENVDB_ENABLE_3_ABI_COMPATIBLE)
	#add_definitions(-DOPENVDB_ABI_VERSION_NUMBER=3)
	#openvdb::OPENVDB_VERSION_NAME
    include_directories(${OPENVDB_INCLUDE_DIR} ${OPENVDB_INCLUDE_DIRS} ${OPENEXR_INCLUDE_DIR} ${TBB_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
    set(core_libs ${OPENVDB_LIBRARIES} ${TBB_LIBRARIES} ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES})
else()
    if (NOT OPENVDB_FOUND)
        message(STATUS "No OpenVDB detected. Building without .vdb support")
    elseif(NOT OPENEXR_FOUND)
        message(STATUS "No OpenEXR libraries detected. Building without .vdb support")
    else()
        message(STATUS "No TBB detected. Building without .vdb support")
    endif()
endif()

file(COPY "C:/vcpkg/installed/x86-windows/bin/Half-2_3.dll" DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY "C:/vcpkg/installed/x86-windows/bin/tbb.dll" DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY "C:/vcpkg/installed/x64-windows/bin/Half-2_3.dll" DESTINATION ${CMAKE_BINARY_DIR}/Debug)
file(COPY "C:/vcpkg/installed/x64-windows/bin/tbb.dll" DESTINATION ${CMAKE_BINARY_DIR}/Debug)
#set(TBB_BUILD_TYPE DEBUG)
#https://github.com/xtensor-stack/xtensor/blob/master/cmake/FindTBB.cmake

# Sources files
file(GLOB SOURCE_FILES
    "src/*.h"
    "src/*.cpp"
)

add_executable(NarvalEngine ${SOURCE_FILES} ${GLAD} ${IMGUI_SOURCES} ${FASTNOISE_SOURCES} ${STB_SOURCES} )
target_link_libraries(NarvalEngine ${core_libs} glfw ${GLFW_LIBRARIES} assimp)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NarvalEngine)