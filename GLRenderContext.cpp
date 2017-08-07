/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2017 (git@dga.me.uk)
 */
#include "Common.h"
#include "core/StringUtils.h"
#include "renderer/SPIRV.h"
#include "renderer/api/GLRenderContext.h"

#define GL_CHECK() __CHECK(__FILE__, __LINE__)
#define __CHECK(FILE, LINE)                                                        \
    {                                                                              \
        GLuint err = glGetError();                                                 \
        if (err != 0) {                                                            \
            log().error("glGetError() returned 0x%.4X at %s:%s", err, FILE, LINE); \
            assert(false);                                                         \
            abort();                                                               \
        }                                                                          \
    }

namespace dw {
namespace {
// GL TextureFormatInfo.
struct TextureFormatInfo {
    GLenum internal_format;
    GLenum internal_format_srgb;
    GLenum format;
    GLenum type;
    bool supported;
};

// clang-format off
TextureFormatInfo s_texture_format[] =
    {
        { GL_ALPHA,              GL_ZERO,         GL_ALPHA,            GL_UNSIGNED_BYTE,                false }, // A8
        { GL_R8,                 GL_ZERO,         GL_RED,              GL_UNSIGNED_BYTE,                false }, // R8
        { GL_R8I,                GL_ZERO,         GL_RED,              GL_BYTE,                         false }, // R8I
        { GL_R8UI,               GL_ZERO,         GL_RED,              GL_UNSIGNED_BYTE,                false }, // R8U
        { GL_R8_SNORM,           GL_ZERO,         GL_RED,              GL_BYTE,                         false }, // R8S
        { GL_R16,                GL_ZERO,         GL_RED,              GL_UNSIGNED_SHORT,               false }, // R16
        { GL_R16I,               GL_ZERO,         GL_RED,              GL_SHORT,                        false }, // R16I
        { GL_R16UI,              GL_ZERO,         GL_RED,              GL_UNSIGNED_SHORT,               false }, // R16U
        { GL_R16F,               GL_ZERO,         GL_RED,              GL_HALF_FLOAT,                   false }, // R16F
        { GL_R16_SNORM,          GL_ZERO,         GL_RED,              GL_SHORT,                        false }, // R16S
        { GL_R32I,               GL_ZERO,         GL_RED,              GL_INT,                          false }, // R32I
        { GL_R32UI,              GL_ZERO,         GL_RED,              GL_UNSIGNED_INT,                 false }, // R32U
        { GL_R32F,               GL_ZERO,         GL_RED,              GL_FLOAT,                        false }, // R32F
        { GL_RG8,                GL_ZERO,         GL_RG,               GL_UNSIGNED_BYTE,                false }, // RG8
        { GL_RG8I,               GL_ZERO,         GL_RG,               GL_BYTE,                         false }, // RG8I
        { GL_RG8UI,              GL_ZERO,         GL_RG,               GL_UNSIGNED_BYTE,                false }, // RG8U
        { GL_RG8_SNORM,          GL_ZERO,         GL_RG,               GL_BYTE,                         false }, // RG8S
        { GL_RG16,               GL_ZERO,         GL_RG,               GL_UNSIGNED_SHORT,               false }, // RG16
        { GL_RG16I,              GL_ZERO,         GL_RG,               GL_SHORT,                        false }, // RG16I
        { GL_RG16UI,             GL_ZERO,         GL_RG,               GL_UNSIGNED_SHORT,               false }, // RG16U
        { GL_RG16F,              GL_ZERO,         GL_RG,               GL_FLOAT,                        false }, // RG16F
        { GL_RG16_SNORM,         GL_ZERO,         GL_RG,               GL_SHORT,                        false }, // RG16S
        { GL_RG32I,              GL_ZERO,         GL_RG,               GL_INT,                          false }, // RG32I
        { GL_RG32UI,             GL_ZERO,         GL_RG,               GL_UNSIGNED_INT,                 false }, // RG32U
        { GL_RG32F,              GL_ZERO,         GL_RG,               GL_FLOAT,                        false }, // RG32F
        { GL_RGB8,               GL_SRGB8,        GL_RGB,              GL_UNSIGNED_BYTE,                false }, // RGB8
        { GL_RGB8I,              GL_ZERO,         GL_RGB,              GL_BYTE,                         false }, // RGB8I
        { GL_RGB8UI,             GL_ZERO,         GL_RGB,              GL_UNSIGNED_BYTE,                false }, // RGB8U
        { GL_RGB8_SNORM,         GL_ZERO,         GL_RGB,              GL_BYTE,                         false }, // RGB8S
        { GL_RGBA8,              GL_SRGB8_ALPHA8, GL_BGRA,             GL_UNSIGNED_BYTE,                false }, // BGRA8
        { GL_RGBA8,              GL_SRGB8_ALPHA8, GL_RGBA,             GL_UNSIGNED_BYTE,                false }, // RGBA8
        { GL_RGBA8I,             GL_ZERO,         GL_RGBA,             GL_BYTE,                         false }, // RGBA8I
        { GL_RGBA8UI,            GL_ZERO,         GL_RGBA,             GL_UNSIGNED_BYTE,                false }, // RGBA8U
        { GL_RGBA8_SNORM,        GL_ZERO,         GL_RGBA,             GL_BYTE,                         false }, // RGBA8S
        { GL_RGBA16,             GL_ZERO,         GL_RGBA,             GL_UNSIGNED_SHORT,               false }, // RGBA16
        { GL_RGBA16I,            GL_ZERO,         GL_RGBA,             GL_SHORT,                        false }, // RGBA16I
        { GL_RGBA16UI,           GL_ZERO,         GL_RGBA,             GL_UNSIGNED_SHORT,               false }, // RGBA16U
        { GL_RGBA16F,            GL_ZERO,         GL_RGBA,             GL_HALF_FLOAT,                   false }, // RGBA16F
        { GL_RGBA16_SNORM,       GL_ZERO,         GL_RGBA,             GL_SHORT,                        false }, // RGBA16S
        { GL_RGBA32I,            GL_ZERO,         GL_RGBA,             GL_INT,                          false }, // RGBA32I
        { GL_RGBA32UI,           GL_ZERO,         GL_RGBA,             GL_UNSIGNED_INT,                 false }, // RGBA32U
        { GL_RGBA32F,            GL_ZERO,         GL_RGBA,             GL_FLOAT,                        false }, // RGBA32F
        { GL_RGB565,             GL_ZERO,         GL_RGB,              GL_UNSIGNED_SHORT_5_6_5,         false }, // R5G6B5
        { GL_RGBA4,              GL_ZERO,         GL_RGBA,             GL_UNSIGNED_SHORT_4_4_4_4_REV,   false }, // RGBA4
        { GL_RGB5_A1,            GL_ZERO,         GL_RGBA,             GL_UNSIGNED_SHORT_1_5_5_5_REV,   false }, // RGB5A1
        { GL_RGB10_A2,           GL_ZERO,         GL_RGBA,             GL_UNSIGNED_INT_2_10_10_10_REV,  false }, // RGB10A2
        { GL_R11F_G11F_B10F,     GL_ZERO,         GL_RGB,              GL_UNSIGNED_INT_10F_11F_11F_REV, false }, // RG11B10F
        { GL_DEPTH_COMPONENT16,  GL_ZERO,         GL_DEPTH_COMPONENT,  GL_UNSIGNED_SHORT,               false }, // D16
        { GL_DEPTH_COMPONENT24,  GL_ZERO,         GL_DEPTH_COMPONENT,  GL_UNSIGNED_INT,                 false }, // D24
        { GL_DEPTH24_STENCIL8,   GL_ZERO,         GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8,            false }, // D24S8
        { GL_DEPTH_COMPONENT32,  GL_ZERO,         GL_DEPTH_COMPONENT,  GL_UNSIGNED_INT,                 false }, // D32
        { GL_DEPTH_COMPONENT32F, GL_ZERO,         GL_DEPTH_COMPONENT,  GL_FLOAT,                        false }, // D16F
        { GL_DEPTH_COMPONENT32F, GL_ZERO,         GL_DEPTH_COMPONENT,  GL_FLOAT,                        false }, // D24F
        { GL_DEPTH_COMPONENT32F, GL_ZERO,         GL_DEPTH_COMPONENT,  GL_FLOAT,                        false }, // D32F
        { GL_STENCIL_INDEX8,     GL_ZERO,         GL_STENCIL_INDEX,    GL_UNSIGNED_BYTE,                false }, // D0S8
    };
// clang-format on
static_assert(static_cast<int>(TextureFormat::Count) ==
                  sizeof(s_texture_format) / sizeof(s_texture_format[0]),
              "Texture format mapping mismatch.");

// Uniform binder.
class UniformBinder : public Object {
public:
    DW_OBJECT(UniformBinder);

    UniformBinder(Context* context) : Object{context}, uniform_location_{0} {
    }

    void operator()(const int& value) {
        glUniform1i(uniform_location_, value);
        GL_CHECK();
    }

    void operator()(const float& value) {
        glUniform1f(uniform_location_, value);
        GL_CHECK();
    }

    void operator()(const Vec2& value) {
        glUniform2f(uniform_location_, value.x, value.y);
        GL_CHECK();
    }

    void operator()(const Vec3& value) {
        glUniform3f(uniform_location_, value.x, value.y, value.z);
        GL_CHECK();
    }

    void operator()(const Vec4& value) {
        glUniform4f(uniform_location_, value.x, value.y, value.z, value.w);
        GL_CHECK();
    }

    void operator()(const Mat3& value) {
        glUniformMatrix3fv(uniform_location_, 1, GL_TRUE, value.ptr());
        GL_CHECK();
    }

    void operator()(const Mat4& value) {
        glUniformMatrix4fv(uniform_location_, 1, GL_TRUE, value.ptr());
        GL_CHECK();
    }

    void updateUniform(GLint location, const RenderItem::UniformData& data) {
        uniform_location_ = location;
        VariantApplyVisitor(*this, data);
    }

private:
    GLint uniform_location_;
};
}  // namespace

GLRenderContext::GLRenderContext(Context* context) : RenderContext{context} {
}

GLRenderContext::~GLRenderContext() {
    // TODO: detect resource leaks.
}

void GLRenderContext::createWindow(u16 width, u16 height, const String& title) {
    log().info("Creating window.");

    // Initialise GLFW.
    log().info("GLFW Version: %s", glfwGetVersionString());
    if (!glfwInit()) {
        // TODO: report error correctly.
        throw Exception{"Failed to initialise GLFW."};
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create the window.
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window_);

    // Initialise GL extensions.
    if (gl3wInit() != 0) {
        throw Exception{"gl3wInit failed."};
    }

    // Print GL information.
    log().info("OpenGL: %s - GLSL: %s", glGetString(GL_VERSION),
               glGetString(GL_SHADING_LANGUAGE_VERSION));
    log().info("OpenGL Renderer: %s", glGetString(GL_RENDERER));

    // Hand off context to render thread.
    glfwMakeContextCurrent(nullptr);
}

void GLRenderContext::destroyWindow() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

void GLRenderContext::processEvents() {
    glfwPollEvents();
}

bool GLRenderContext::isWindowClosed() const {
    return glfwWindowShouldClose(window_) != 0;
}

void GLRenderContext::startRendering() {
    glfwMakeContextCurrent(window_);
}

void GLRenderContext::processCommandList(Vector<RenderCommand>& command_list) {
    assert(window_);
    for (auto& command : command_list) {
        VariantApplyVisitor(*this, command);
    }
}

bool GLRenderContext::frame(const Vector<View>& views) {
    assert(window_);

    // Process views.
    for (auto& v : views) {
        if (v.render_items.empty()) {
            continue;
        }

        // Set up framebuffer.
        assert(v.frame_buffer != FrameBufferHandle::invalid);
        if (v.frame_buffer.internal() > 0) {
            FrameBufferData& fb_data = frame_buffer_map_.at(v.frame_buffer);
            glBindFramebuffer(GL_FRAMEBUFFER, fb_data.frame_buffer);
            GL_CHECK();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Set up view.
        glClearColor(v.clear_colour.r(), v.clear_colour.g(), v.clear_colour.b(),
                     v.clear_colour.a());
        glEnable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render items.
        for (uint i = 0; i < v.render_items.size(); ++i) {
            auto* previous = i > 0 ? &v.render_items[i - 1] : nullptr;
            auto* current = &v.render_items[i];

            // Update render state.
            static HashMap<BlendEquation, GLenum> blend_equation_map = {
                {BlendEquation::Add, GL_FUNC_ADD},
                {BlendEquation::Subtract, GL_FUNC_SUBTRACT},
                {BlendEquation::ReverseSubtract, GL_FUNC_REVERSE_SUBTRACT},
                {BlendEquation::Min, GL_MIN},
                {BlendEquation::Max, GL_MAX}};
            static HashMap<BlendFunc, GLenum> blend_func_map = {
                {BlendFunc::Zero, GL_ZERO},
                {BlendFunc::One, GL_ONE},
                {BlendFunc::SrcColor, GL_SRC_COLOR},
                {BlendFunc::OneMinusSrcColor, GL_ONE_MINUS_SRC_COLOR},
                {BlendFunc::DstColor, GL_DST_COLOR},
                {BlendFunc::OneMinusDstColor, GL_ONE_MINUS_DST_COLOR},
                {BlendFunc::SrcAlpha, GL_SRC_ALPHA},
                {BlendFunc::OneMinusSrcAlpha, GL_ONE_MINUS_SRC_ALPHA},
                {BlendFunc::DstAlpha, GL_DST_ALPHA},
                {BlendFunc::OneMinusDstAlpha, GL_ONE_MINUS_DST_ALPHA},
                {BlendFunc::ConstantColor, GL_CONSTANT_COLOR},
                {BlendFunc::OneMinusConstantColor, GL_ONE_MINUS_CONSTANT_COLOR},
                {BlendFunc::ConstantAlpha, GL_CONSTANT_ALPHA},
                {BlendFunc::OneMinusConstantAlpha, GL_ONE_MINUS_CONSTANT_ALPHA},
                {BlendFunc::SrcAlphaSaturate, GL_SRC_ALPHA_SATURATE},
            };
            if (!previous || previous->cull_face_enabled != current->cull_face_enabled) {
                if (current->cull_face_enabled) {
                    glEnable(GL_CULL_FACE);
                } else {
                    glDisable(GL_CULL_FACE);
                }
            }
            if (!previous || previous->cull_front_face != current->cull_front_face) {
                glFrontFace(current->cull_front_face == CullFrontFace::CCW ? GL_CCW : GL_CW);
            }
            if (!previous || previous->depth_enabled != current->depth_enabled) {
                if (current->depth_enabled) {
                    glEnable(GL_DEPTH_TEST);
                } else {
                    glDisable(GL_DEPTH_TEST);
                }
            }
            if (!previous || previous->blend_enabled != current->blend_enabled) {
                if (current->blend_enabled) {
                    glEnable(GL_BLEND);
                } else {
                    glDisable(GL_BLEND);
                }
            }
            if (!previous || previous->blend_equation_rgb != current->blend_equation_rgb ||
                previous->blend_equation_a != current->blend_equation_a) {
                glBlendEquationSeparate(blend_equation_map.at(current->blend_equation_rgb),
                                        blend_equation_map.at(current->blend_equation_a));
            }
            if (!previous || previous->blend_src_rgb != current->blend_src_rgb ||
                previous->blend_src_a != current->blend_src_a ||
                previous->blend_dest_rgb != current->blend_dest_rgb ||
                previous->blend_dest_a != current->blend_dest_a) {
                glBlendFuncSeparate(blend_func_map.at(current->blend_src_rgb),
                                    blend_func_map.at(current->blend_dest_rgb),
                                    blend_func_map.at(current->blend_src_a),
                                    blend_func_map.at(current->blend_dest_a));
            }

            // Bind VAO.
            if (!previous || previous->vb != current->vb) {
                if (current->vb != VertexBufferHandle::invalid) {
                    glBindVertexArray(vertex_buffer_map_.at(current->vb));
                } else {
                    glBindVertexArray(0);
                }
                GL_CHECK();
            }

            // Bind EBO.
            if (!previous || previous->ib != current->ib) {
                if (current->ib != IndexBufferHandle::invalid) {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                 index_buffer_map_.at(current->ib).element_buffer);
                    GL_CHECK();
                } else {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                    GL_CHECK();
                }
            }

            // Bind Program.
            if (!previous || previous->program != current->program) {
                assert(current->program != ProgramHandle::invalid);
                ProgramData& program_data = program_map_.at(current->program);
                glUseProgram(program_data.program);
                GL_CHECK();

                // Bind uniforms.
                UniformBinder binder{context_};
                for (auto& uniform_pair : current->uniforms) {
                    auto location = program_data.uniform_location_map.find(uniform_pair.first);
                    GLint uniform_location;
                    if (location != program_data.uniform_location_map.end()) {
                        uniform_location = (*location).second;
                    } else {
                        uniform_location =
                            glGetUniformLocation(program_data.program, uniform_pair.first.c_str());
                        GL_CHECK();
                        program_data.uniform_location_map.emplace(uniform_pair.first,
                                                                  uniform_location);
                        if (uniform_location == -1) {
                            log().warn("[Frame] Unknown uniform '%s', skipping.",
                                       uniform_pair.first);
                        }
                    }
                    if (uniform_location == -1) {
                        continue;
                    }
                    binder.updateUniform(uniform_location, uniform_pair.second);
                }

                // Bind textures.
                for (uint j = 0; j < current->textures.size(); ++j) {
                    if (current->textures[j].handle == TextureHandle::invalid) {
                        break;
                    }
                    glActiveTexture(GL_TEXTURE0 + j);
                    glBindTexture(GL_TEXTURE_2D, texture_map_.at(current->textures[j].handle));
                    GL_CHECK();
                }
            }

            // Submit.
            if (current->primitive_count > 0) {
                if (current->ib != IndexBufferHandle::invalid) {
                    glDrawElements(GL_TRIANGLES, current->primitive_count * 3,
                                   index_buffer_map_[current->ib].type, 0);
                    GL_CHECK();
                } else {
                    glDrawArrays(GL_TRIANGLES, 0, current->primitive_count * 3);
                    GL_CHECK();
                }
            }
        }
    }

    // Swap buffers.
    glfwSwapBuffers(window_);

    // Continue rendering.
    return true;
}

void GLRenderContext::operator()(const cmd::CreateVertexBuffer& c) {
    // Create vertex array object.
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create vertex buffer object.
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, c.data.size(), c.data.data(), GL_STATIC_DRAW);
    GL_CHECK();

    // Set up vertex array attributes.
    static HashMap<VertexDecl::AttributeType, GLenum> attribute_type_map = {
        {VertexDecl::AttributeType::Float, GL_FLOAT},
        {VertexDecl::AttributeType::Uint8, GL_UNSIGNED_BYTE}};
    u16 attrib_counter = 0;
    for (auto& attrib : c.decl.attributes_) {
        // Decode attribute.
        VertexDecl::Attribute attribute;
        uint count;
        VertexDecl::AttributeType type;
        bool normalised;
        VertexDecl::decodeAttributes(attrib.first, attribute, count, type, normalised);

        // Convert type.
        auto gl_type = attribute_type_map.find(type);
        if (gl_type == attribute_type_map.end()) {
            log().warn("[CreateVertexBuffer] Unknown attribute type: %i", (uint)type);
            continue;
        }

        log().debug("[CreateVertexBuffer] Attrib %s: Count='%s' Type='%s' Stride='%s' Offset='%s'",
                    attrib_counter, count, static_cast<int>(gl_type->first), c.decl.stride_,
                    reinterpret_cast<intptr_t>(attrib.second));

        // Set attribute.
        glEnableVertexAttribArray(attrib_counter);
        glVertexAttribPointer(attrib_counter, count, gl_type->second,
                              static_cast<GLboolean>(normalised ? GL_TRUE : GL_FALSE),
                              c.decl.stride_, attrib.second);
        attrib_counter++;
    }
    vertex_buffer_map_.emplace(c.handle, vao);
}

void GLRenderContext::operator()(const cmd::DeleteVertexBuffer& c) {
    // TODO: implement.
}

void GLRenderContext::operator()(const cmd::CreateIndexBuffer& c) {
    // Create element buffer object.
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, c.data.size(), c.data.data(), GL_STATIC_DRAW);
    GL_CHECK();

    index_buffer_map_.emplace(
        c.handle, IndexBufferData{
                      ebo, static_cast<GLenum>(c.type == IndexBufferType::U16 ? GL_UNSIGNED_SHORT
                                                                              : GL_UNSIGNED_INT)});
}

void GLRenderContext::operator()(const cmd::DeleteIndexBuffer& c) {
    // TODO: implement.
}

void GLRenderContext::operator()(const cmd::CreateShader& c) {
    // Convert SPIR-V into GLSL.
    spirv_cross::CompilerGLSL glsl_out{reinterpret_cast<const u32*>(c.data.data()),
                                       c.data.size() / sizeof(u32)};
    spirv_cross::ShaderResources resources = glsl_out.get_shader_resources();

    // Compile to GLSL, ready to give to GL driver.
    spirv_cross::CompilerGLSL::Options options;
    options.version = 330;
    options.es = false;
    glsl_out.set_options(options);
    String source = glsl_out.compile();

    // Postprocess the GLSL to remove the 4.2 extension, which doesn't exist on macOS.
    source = str::replace(source, "#extension GL_ARB_shading_language_420pack : require",
                          "#extension GL_ARB_shading_language_420pack : disable");
    log().debug("Decompiled GLSL from SPIR-V: %s", source);

    // Compile shader.
    static HashMap<ShaderStage, GLenum> shader_type_map = {
        {ShaderStage::Vertex, GL_VERTEX_SHADER},
        {ShaderStage::Geometry, GL_GEOMETRY_SHADER},
        {ShaderStage::Fragment, GL_FRAGMENT_SHADER}};
    GLuint shader = glCreateShader(shader_type_map.at(c.stage));
    if (shader == 0) {
        GL_CHECK();
        // TODO: an error occurred.
    }
    const char* sources[] = {source.c_str()};
    glShaderSource(shader, 1, sources, nullptr);
    glCompileShader(shader);

    // Check compilation result.
    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int info_log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);

        char* error_message = new char[info_log_length];
        glGetShaderInfoLog(shader, info_log_length, NULL, error_message);
        log().error("[CreateShader] Shader Compile Error: %s", error_message);
        delete[] error_message;

        // TODO: Error
    }

    shader_map_.emplace(c.handle, shader);
}

void GLRenderContext::operator()(const cmd::DeleteShader& c) {
    auto it = shader_map_.find(c.handle);
    glDeleteShader(it->second);
    shader_map_.erase(it);
}

void GLRenderContext::operator()(const cmd::CreateProgram& c) {
    ProgramData program_data;
    program_data.program = glCreateProgram();
    program_map_.emplace(c.handle, program_data);
}

void GLRenderContext::operator()(const cmd::AttachShader& c) {
    glAttachShader(program_map_.at(c.handle).program, shader_map_.at(c.shader_handle));
}

void GLRenderContext::operator()(const cmd::LinkProgram& c) {
    GLuint program = program_map_.at(c.handle).program;
    glLinkProgram(program);

    // Check the result of the link process.
    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        int info_log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
        char* error_message = new char[info_log_length];
        glGetProgramInfoLog(program, info_log_length, NULL, error_message);
        log().error("[LinkProgram] Shader Link Error: %s", error_message);
        delete[] error_message;
    }
}

void GLRenderContext::operator()(const cmd::DeleteProgram& c) {
    auto it = program_map_.find(c.handle);
    if (it != program_map_.end()) {
        glDeleteProgram(it->second.program);
        program_map_.erase(it);
    } else {
        log().error("[DeleteProgram] Unable to find program with handle %s", c.handle.internal());
    }
}

void GLRenderContext::operator()(const cmd::CreateTexture2D& c) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Filtering.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Give image data to OpenGL.
    TextureFormatInfo format = s_texture_format[static_cast<int>(c.format)];
    log().debug(
        "[CreateTexture2D] format %s - internal fmt: 0x%.4X - internal fmt srgb: 0x%.4X "
        "- fmt: 0x%.4X - type: 0x%.4X",
        static_cast<u32>(c.format), format.internal_format, format.internal_format_srgb,
        format.format, format.type);
    glTexImage2D(GL_TEXTURE_2D, 0, format.internal_format, c.width, c.height, 0, format.format,
                 format.type, c.data.data());
    GL_CHECK();

    // Add texture.
    texture_map_.emplace(c.handle, texture);
}

void GLRenderContext::operator()(const cmd::DeleteTexture& c) {
    auto it = texture_map_.find(c.handle);
    glDeleteTextures(1, &it->second);
    texture_map_.erase(it);
}

void GLRenderContext::operator()(const cmd::CreateFrameBuffer& c) {
    FrameBufferData fb_data;
    fb_data.textures = c.textures;

    glGenFramebuffers(1, &fb_data.frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_data.frame_buffer);

    // Bind colour buffers.
    Vector<GLenum> draw_buffers;
    u8 attachment = 0;
    for (auto texture : fb_data.textures) {
        auto gl_texture = texture_map_.find(texture);
        assert(gl_texture != texture_map_.end());
        draw_buffers.emplace_back(GL_COLOR_ATTACHMENT0 + attachment++);
        glFramebufferTexture2D(GL_FRAMEBUFFER, draw_buffers.back(), GL_TEXTURE_2D,
                               gl_texture->second, 0);
    }
    glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data());

    // Create depth buffer.
    glGenRenderbuffers(1, &fb_data.depth_render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, fb_data.depth_render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, c.width, c.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              fb_data.depth_render_buffer);

    // Check frame buffer status.
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        log().error("[CreateFrameBuffer] The framebuffer is not complete. Status: 0x%.4X", status);
    }

    // Unbind.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Add to map.
    frame_buffer_map_.emplace(c.handle, fb_data);
}

void GLRenderContext::operator()(const cmd::DeleteFrameBuffer& c) {
    // TODO: unimplemented.
}

}  // namespace dw
