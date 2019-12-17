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

add_executable(Examples Examples.cpp)
target_link_libraries(Examples dawn-gfx stb)

if(EMSCRIPTEN)
    set_target_properties(Examples PROPERTIES SUFFIX .html)
    set(link_flags "-s DEMANGLE_SUPPORT=1 -s TOTAL_MEMORY=256MB -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(link_flags "${link_flags} --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/media@/media")
    set_target_properties(Examples PROPERTIES LINK_FLAGS ${link_flags})
endif()