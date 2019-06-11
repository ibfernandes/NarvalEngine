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
set(GLAD "${GLFW_SOURCE_DIR}/deps/glad/glad.h"
         "${GLFW_SOURCE_DIR}/deps/glad.c")

# STB loaders
include_directories(includes/stb)

# GLM library
include_directories(includes/glm)

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

# Sources files
file(GLOB SOURCE_FILES
    "src/*.h"
    "src/*.cpp"
)

add_executable(NarvalEngine ${SOURCE_FILES} ${GLAD} ${IMGUI_SOURCES})
target_link_libraries(NarvalEngine glfw ${GLFW_LIBRARIES})