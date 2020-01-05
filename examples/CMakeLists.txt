# stb for image loading.
FetchContent_Declare(
    stb
    URL https://github.com/nothings/stb/archive/e6afb9cbae4064da8c3e69af3ff5c4629579c1d2.tar.gz
    URL_MD5 e52f1d8c18612d4d7a2eb1c5e2d7ed0f
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

macro(add_example EXAMPLE)
    add_executable(Example-${EXAMPLE} ${EXAMPLE}.cpp Common.h)
    target_link_libraries(Example-${EXAMPLE} dawn-gfx stb dawn-gfx-imgui)
    if(EMSCRIPTEN)
        set_target_properties(Example-${EXAMPLE} PROPERTIES SUFFIX .html)
        set(link_flags "-s DEMANGLE_SUPPORT=1 -s TOTAL_MEMORY=256MB -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
        set(link_flags "${link_flags} --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/media@/media")
        set_target_properties(Example-${EXAMPLE} PROPERTIES LINK_FLAGS ${link_flags})
    endif()
endmacro()

add_example(01-BasicVertexBuffer)
add_example(02-BasicIndexBuffer)
add_example(03-Simple3D)
add_example(04-TransientIndexBuffer)
add_example(05-Textured3DCube)
add_example(06-NormalMapping)
add_example(07-PostProcessing)
add_example(08-DeferredShading)
