# dawn-gfx
# Written by David Avedissian (c) 2017-2020 (git@dga.dev)
cmake_minimum_required(VERSION 3.14)
project(dawn-gfx)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
include(ExternalProject)

# Use GLVND if possible.
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

# Patch tool
if(WIN32)
    # Hilariously, Windows won't run the tool without admin privileges if it's called patch.exe.
    set(PATCH_TOOL ${CMAKE_CURRENT_SOURCE_DIR}/tools/ptool.exe)
else()
    set(PATCH_TOOL patch)
endif()

# Dependencies.
find_package(OpenGL REQUIRED)

FetchContent_Declare(
    cpp-utils
    URL https://github.com/dgavedissian/cpputils/archive/4aebbf03195cd922eb6a72204beeb2e1ba37dce8.zip
)
FetchContent_MakeAvailable(cpp-utils)

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

if(EMSCRIPTEN)
    add_library(glad INTERFACE)
    target_include_directories(glad INTERFACE src/dawn-gfx/gl/glad/include)
else()
    add_library(glad EXCLUDE_FROM_ALL
        src/dawn-gfx/gl/glad/src/gl.c)
    target_include_directories(glad PUBLIC src/dawn-gfx/gl/glad/include)
endif()

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/c90c7b97109db909577e3bf540b5f884422b7e14.tar.gz
    URL_MD5 326ceda8f5184e3e79625c6d2268a3c5
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
    URL https://github.com/KhronosGroup/glslang/archive/7.10.2984.tar.gz
    URL_MD5 865bad2658dd1d61f9c5583a4a1992e6
)
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
    URL https://github.com/KhronosGroup/SPIRV-Cross/archive/ed16b3e69985feaf565efbecea70a1cc2fca2a58.tar.gz
    URL_MD5 970fa559c40cee3ba603246a1696a6c9
)
FetchContent_GetProperties(spirv-cross)
if(NOT spirv-cross_POPULATED)
    FetchContent_Populate(spirv-cross)
    add_subdirectory(${spirv-cross_SOURCE_DIR} ${spirv-cross_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_include_directories(spirv-cross-core PUBLIC $<BUILD_INTERFACE:${spirv-cross_SOURCE_DIR}/include/spirv_cross>)
endif()

add_library(dawn-gfx
    include/dawn-gfx/detail/BaseTypes.h
    include/dawn-gfx/detail/Handle.h
    include/dawn-gfx/detail/Memory.h
    include/dawn-gfx/Base.h
    include/dawn-gfx/Colour.h
    include/dawn-gfx/Input.h
    include/dawn-gfx/Logger.h
    include/dawn-gfx/Math.h
    include/dawn-gfx/RHIRenderer.h
    include/dawn-gfx/Shader.h
    include/dawn-gfx/VertexDecl.h
    src/dawn-gfx/gl/GL.h
    src/dawn-gfx/gl/GLRenderContext.cpp
    src/dawn-gfx/gl/GLRenderContext.h
    src/dawn-gfx/null/NullRenderContext.cpp
    src/dawn-gfx/null/NullRenderContext.h
    src/dawn-gfx/Colour.cpp
    src/dawn-gfx/Glslang.h
    src/dawn-gfx/Memory.cpp
    src/dawn-gfx/RHIRenderer.cpp
    src/dawn-gfx/Shader.cpp
    src/dawn-gfx/SPIRV.h
    src/dawn-gfx/VertexDecl.cpp)
target_include_directories(dawn-gfx PUBLIC include)
target_include_directories(dawn-gfx PRIVATE include/dawn-gfx src/dawn-gfx)
target_link_libraries(dawn-gfx barrier expected fmt glad glfw glslang SPIRV MathGeoLib spirv-cross-glsl spirv-cross-hlsl)

add_subdirectory(examples)
