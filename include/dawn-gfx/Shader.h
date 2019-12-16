/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"
#include <tl/expected.hpp>

namespace dw {
struct ShaderCompileError {
    std::string compile_error;
    std::string debug_log;
};
// Compiles an in-memory GLSL shader into SPIR-V.
tl::expected<std::vector<u32>, ShaderCompileError> compileGLSL(const std::string& glsl_source, ShaderStage stage);
}