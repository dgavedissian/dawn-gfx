/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"

namespace dw {
namespace gfx {
struct ShaderCompileError {
    std::string compile_error;
    std::string debug_log;
};

// Compiles an in-memory GLSL shader into SPIR-V.
Result<ShaderStageInfo, ShaderCompileError> compileGLSL(
    ShaderStage stage, const std::string& glsl_source,
    const std::vector<std::string>& compile_definitions = {});
}  // namespace gfx
}  // namespace dw
