# dawn-gfx
# Written by David Avedissian (c) 2017-2020 (git@dga.dev)
cmake_minimum_required(VERSION 3.14)
project(dawn-gfx)

include(FetchContent)
include(ExternalProject)

# Determine if this is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif()

# Pinned dependencies.
FetchContent_Declare(
    base
    URL https://github.com/dgavedissian/base/archive/aa1cd34a84a22fc0becd4ed4eb5cd022b8685cfe.zip
    URL_MD5 3584045bab754ed8e8cf163bc9b2683e
)
FetchContent_MakeAvailable(base)
make_patch_tool_available()

FetchContent_Declare(
    expected
    URL https://github.com/TartanLlama/expected/archive/fd96e4545cc50ddfbbead177ef6594b12b210d62.zip
    URL_MD5 8e0769ee95b95f4aa58d5a37d51d9bad
)
if(NOT expected_POPULATED)
    FetchContent_Populate(expected)
    add_library(expected INTERFACE)
    target_include_directories(expected INTERFACE ${expected_SOURCE_DIR}/include)
endif()

FetchContent_declare(
    fmt
    URL https://github.com/fmtlib/fmt/releases/download/6.1.2/fmt-6.1.2.zip
    URL_MD5 5ec643b082a34254ad07136e7cbd4220
)
FetchContent_MakeAvailable(fmt)

add_library(glad EXCLUDE_FROM_ALL
    src/glad/src/gl.c
    src/glad/include/glad/gl.h
    src/glad/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC src/glad/include)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE CXX)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/3.3.tar.gz
)
FetchContent_GetProperties(glfw)
if(NOT glfw_POPULATED)
    FetchContent_Populate(glfw)
    if(NOT EMSCRIPTEN)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
        add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR} EXCLUDE_FROM_ALL)
    else()
        add_library(glfw INTERFACE)
        target_include_directories(glfw INTERFACE ${glfw_SOURCE_DIR}/include)
        target_link_libraries(glfw INTERFACE "-s USE_GLFW=3" "-s USE_WEBGL2=1" "-s FULL_ES3=1")
    endif()
endif()

FetchContent_Declare(
    glslang
    URL https://github.com/KhronosGroup/glslang/archive/7.13.3496.tar.gz
    URL_MD5 9bfbff6115e490654e149fd0558dba0c
)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
mark_as_advanced(
    BUILD_TESTING
    ENABLE_GLSLANG_BINARIES
    ENABLE_HLSL
)
FetchContent_MakeAvailable(glslang)

FetchContent_Declare(
    mathgeolib
    URL https://github.com/juj/MathGeoLib/archive/01f92dd8fbd1fe2e405de713cfd985ff07d27dc9.tar.gz
    URL_MD5 bdab0f5b19661b7842501c738209e4fb
    PATCH_COMMAND ${PATCH_TOOL} -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/MathGeoLib.diff
)
set(VS_BUG ON CACHE BOOL "" FORCE)
set(MATH_TESTS_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_DIR ${CMAKE_BINARY_DIR}/_deps/mathgeolib-build CACHE BOOL "" FORCE)
set(INSTALL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/mathgeolib-build CACHE BOOL "" FORCE)
mark_as_advanced(
    VS_BUG
    MATH_TESTS_EXECUTABLE
    GENERATE_ASM_LISTING
    INSTALL_CMAKE_DIR
    INSTALL_INCLUDE_DIR
)
FetchContent_MakeAvailable(mathgeolib)
target_include_directories(MathGeoLib PUBLIC $<BUILD_INTERFACE:${mathgeolib_SOURCE_DIR}/src>)

FetchContent_Declare(
    spirv-cross
    URL https://github.com/KhronosGroup/SPIRV-Cross/archive/2020-01-16.tar.gz
    URL_MD5 0cb174fe01ffb9f6d14f4c3c53af003d
)
FetchContent_GetProperties(spirv-cross)
if(NOT spirv-cross_POPULATED)
    FetchContent_Populate(spirv-cross)
    add_subdirectory(${spirv-cross_SOURCE_DIR} ${spirv-cross_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_include_directories(spirv-cross-core PUBLIC $<BUILD_INTERFACE:${spirv-cross_SOURCE_DIR}/include/spirv_cross>)
endif()

# Vulkan
find_package(Vulkan REQUIRED)

# Main library
add_library(dawn-gfx
    include/dawn-gfx/detail/Handle.h
    include/dawn-gfx/detail/MathGeoLib.h
    include/dawn-gfx/detail/Memory.h
    include/dawn-gfx/Base.h
    include/dawn-gfx/Colour.h
    include/dawn-gfx/Input.h
    include/dawn-gfx/Logger.h
    include/dawn-gfx/MathDefs.h
    include/dawn-gfx/MeshBuilder.h
    include/dawn-gfx/Renderer.h
    include/dawn-gfx/Shader.h
    include/dawn-gfx/TriangleBuffer.h
    include/dawn-gfx/VertexDecl.h
    src/gl/GL.h
    src/gl/RenderContextGL.cpp
    src/gl/RenderContextGL.h
    src/null/RenderContextNull.cpp
    src/null/RenderContextNull.h
    src/vulkan/RenderContextVK.cpp
    src/vulkan/RenderContextVK.h
    src/Colour.cpp
    src/Glslang.h
    src/Memory.cpp
    src/MeshBuilder.cpp
    src/RenderContext.h
    src/Renderer.cpp
    src/Shader.cpp
    src/SPIRV.h
    src/TriangleBuffer.cpp
    src/VertexDecl.cpp)
target_include_directories(dawn-gfx PUBLIC include)
target_include_directories(dawn-gfx PRIVATE include/dawn-gfx src)
target_link_libraries(dawn-gfx dga-base expected fmt glad glfw glslang SPIRV MathGeoLib spirv-cross-glsl Vulkan::Vulkan)
target_compile_features(dawn-gfx PUBLIC cxx_std_17)
set_target_properties(dawn-gfx PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_definitions(dawn-gfx _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()

# ImGUI.
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/v1.74.tar.gz
    URL_MD5 df95e1b2e7f1e6f9ee750ac0ec5a9d99
)
FetchContent_MakeAvailable(imgui)
add_library(imgui EXCLUDE_FROM_ALL
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

add_library(dawn-gfx-imgui INTERFACE)
target_sources(dawn-gfx-imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/dawn-gfx-imgui/ImGuiBackend.h)
target_include_directories(dawn-gfx-imgui INTERFACE include)
target_link_libraries(dawn-gfx-imgui INTERFACE imgui)

if(MASTER_PROJECT)
    add_subdirectory(examples)
endif()
