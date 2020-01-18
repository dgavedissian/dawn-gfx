/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Base.h"
#include "SPIRV.h"
#include "gl/RenderContextGL.h"
#include "Input.h"

#include <dga/string_algorithms.h>
#include <fmt/format.h>
#include <locale>
#include <exception>
#include <codecvt>

/**
 * RenderContextGL. A render context implementation which targets GL
#define DW_GL_410 1 4.0 on desktop platforms,
 * and WebGL 2 on HTML5.
 */

#define DW_GLES_300 2

#ifndef DGA_EMSCRIPTEN
#define DW_GL_VERSION DW_GL_410
#else
#define DW_GL_VERSION DW_GLES_300
#endif

#ifndef NDEBUG
#define GL_CHECK(call)                    \
    do {                                  \
        call;                             \
        checkGLError(__FILE__, __LINE__); \
    } while (false)
#else
#define GL_CHECK(call) call
#endif

namespace dw {
namespace gfx {
namespace {
// Debugging
void checkGLError(const char* file, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::ostringstream error_str;
        error_str << "glGetError returned:" << std::endl;
        while (err != GL_NO_ERROR) {
            std::string error;
            switch (err) {
                case GL_INVALID_OPERATION:
                    error = "GL_INVALID_OPERATION";
                    break;
                case GL_INVALID_ENUM:
                    error = "GL_INVALID_ENUM";
                    break;
                case GL_INVALID_VALUE:
                    error = "GL_INVALID_VALUE";
                    break;
                case GL_OUT_OF_MEMORY:
                    error = "GL_OUT_OF_MEMORY";
                    break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    error = "GL_INVALID_FRAMEBUFFER_OPERATION";
                    break;
                default:
                    error = fmt::format("(unknown: {:#x})", err);
                    break;
            }
            error_str << error << " - " << file << ":" << line << std::endl;
            err = glGetError();
        }
        throw std::runtime_error(error_str.str());
    }
}

void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                     GLsizei length, const GLchar* message,
                                     const void* user_param) {
    const auto& logger = *static_cast<const Logger*>(user_param);
    std::string message_str{message};
    if (message_str.substr(message_str.size() - 2) == "\r\n") {
        message_str = message_str.substr(0, message_str.size() - 2);
    } else {
        message_str = message_str.substr(0, message_str.size() - 1);
    }
    if (type == GL_DEBUG_TYPE_ERROR) {
        logger.debug("GL error: severity = {:#x}, message = {}", severity, message_str);
    } else {
        logger.debug("GL message: type = {:#x}, severity = {:#x}, message = {}", type, severity,
                     message_str);
    }
}

// Buffer usage.
GLenum mapBufferUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:
            return GL_STATIC_DRAW;
        case BufferUsage::Dynamic:
            return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:
            return GL_STREAM_DRAW;
        default:
            assert(false);
            return GL_STATIC_DRAW;
    }
}

struct TextureFormatGL {
    GLenum internal_format;
    GLenum internal_format_srgb;
    GLenum format;
    GLenum type;
    bool supported;
};
// clang-format off
const std::array<TextureFormatGL, usize(TextureFormat::Count)> kTextureFormatMap = {{
    {GL_ALPHA,              GL_ZERO,         GL_ALPHA,            GL_UNSIGNED_BYTE,     false}, // A8
    {GL_R8,                 GL_ZERO,         GL_RED,              GL_UNSIGNED_BYTE,     false}, // R8
    {GL_R8I,                GL_ZERO,         GL_RED,              GL_BYTE,              false}, // R8I
    {GL_R8UI,               GL_ZERO,         GL_RED,              GL_UNSIGNED_BYTE,     false}, // R8U
    {GL_R8_SNORM,           GL_ZERO,         GL_RED,              GL_BYTE,              false}, // R8S
    {GL_R16,                GL_ZERO,         GL_RED,              GL_UNSIGNED_SHORT,    false}, // R16
    {GL_R16I,               GL_ZERO,         GL_RED,              GL_SHORT,             false}, // R16I
    {GL_R16UI,              GL_ZERO,         GL_RED,              GL_UNSIGNED_SHORT,    false}, // R16U
    {GL_R16F,               GL_ZERO,         GL_RED,              GL_HALF_FLOAT,        false}, // R16F
    {GL_R16_SNORM,          GL_ZERO,         GL_RED,              GL_SHORT,             false}, // R16S
    {GL_R32I,               GL_ZERO,         GL_RED,              GL_INT,               false}, // R32I
    {GL_R32UI,              GL_ZERO,         GL_RED,              GL_UNSIGNED_INT,      false}, // R32U
    {GL_R32F,               GL_ZERO,         GL_RED,              GL_FLOAT,             false}, // R32F
    {GL_RG8,                GL_ZERO,         GL_RG,               GL_UNSIGNED_BYTE,     false}, // RG8
    {GL_RG8I,               GL_ZERO,         GL_RG,               GL_BYTE,              false}, // RG8I
    {GL_RG8UI,              GL_ZERO,         GL_RG,               GL_UNSIGNED_BYTE,     false}, // RG8U
    {GL_RG8_SNORM,          GL_ZERO,         GL_RG,               GL_BYTE,              false}, // RG8S
    {GL_RG16,               GL_ZERO,         GL_RG,               GL_UNSIGNED_SHORT,    false}, // RG16
    {GL_RG16I,              GL_ZERO,         GL_RG,               GL_SHORT,             false}, // RG16I
    {GL_RG16UI,             GL_ZERO,         GL_RG,               GL_UNSIGNED_SHORT,    false}, // RG16U
    {GL_RG16F,              GL_ZERO,         GL_RG,               GL_FLOAT,             false}, // RG16F
    {GL_RG16_SNORM,         GL_ZERO,         GL_RG,               GL_SHORT,             false}, // RG16S
    {GL_RG32I,              GL_ZERO,         GL_RG,               GL_INT,               false}, // RG32I
    {GL_RG32UI,             GL_ZERO,         GL_RG,               GL_UNSIGNED_INT,      false}, // RG32U
    {GL_RG32F,              GL_ZERO,         GL_RG,               GL_FLOAT,             false}, // RG32F
    {GL_RGB8,               GL_SRGB8,        GL_RGB,              GL_UNSIGNED_BYTE,     false}, // RGB8
    {GL_RGB8I,              GL_ZERO,         GL_RGB,              GL_BYTE,              false}, // RGB8I
    {GL_RGB8UI,             GL_ZERO,         GL_RGB,              GL_UNSIGNED_BYTE,     false}, // RGB8U
    {GL_RGB8_SNORM,         GL_ZERO,         GL_RGB,              GL_BYTE,              false}, // RGB8S
    {GL_RGBA8,              GL_SRGB8_ALPHA8, GL_BGRA,             GL_UNSIGNED_BYTE,     false}, // BGRA8
    {GL_RGBA8,              GL_SRGB8_ALPHA8, GL_RGBA,             GL_UNSIGNED_BYTE,     false}, // RGBA8
    {GL_RGBA8I,             GL_ZERO,         GL_RGBA,             GL_BYTE,              false}, // RGBA8I
    {GL_RGBA8UI,            GL_ZERO,         GL_RGBA,             GL_UNSIGNED_BYTE,     false}, // RGBA8U
    {GL_RGBA8_SNORM,        GL_ZERO,         GL_RGBA,             GL_BYTE,              false}, // RGBA8S
    {GL_RGBA16,             GL_ZERO,         GL_RGBA,             GL_UNSIGNED_SHORT,    false}, // RGBA16
    {GL_RGBA16I,            GL_ZERO,         GL_RGBA,             GL_SHORT,             false}, // RGBA16I
    {GL_RGBA16UI,           GL_ZERO,         GL_RGBA,             GL_UNSIGNED_SHORT,    false}, // RGBA16U
    {GL_RGBA16F,            GL_ZERO,         GL_RGBA,             GL_HALF_FLOAT,        false}, // RGBA16F
    {GL_RGBA16_SNORM,       GL_ZERO,         GL_RGBA,             GL_SHORT,             false}, // RGBA16S
    {GL_RGBA32I,            GL_ZERO,         GL_RGBA,             GL_INT,               false}, // RGBA32I
    {GL_RGBA32UI,           GL_ZERO,         GL_RGBA,             GL_UNSIGNED_INT,      false}, // RGBA32U
    {GL_RGBA32F,            GL_ZERO,         GL_RGBA,             GL_FLOAT,             false}, // RGBA32F
    {GL_DEPTH_COMPONENT16,  GL_ZERO,         GL_DEPTH_COMPONENT,  GL_UNSIGNED_SHORT,    false}, // D16
    {GL_DEPTH_COMPONENT24,  GL_ZERO,         GL_DEPTH_COMPONENT,  GL_UNSIGNED_INT,      false}, // D24
    {GL_DEPTH24_STENCIL8,   GL_ZERO,         GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8, false}, // D24S8
    {GL_DEPTH_COMPONENT32,  GL_ZERO,         GL_DEPTH_COMPONENT,  GL_UNSIGNED_INT,      false}, // D32
    {GL_DEPTH_COMPONENT32F, GL_ZERO,         GL_DEPTH_COMPONENT,  GL_FLOAT,             false}, // D16F
    {GL_DEPTH_COMPONENT32F, GL_ZERO,         GL_DEPTH_COMPONENT,  GL_FLOAT,             false}, // D24F
    {GL_DEPTH_COMPONENT32F, GL_ZERO,         GL_DEPTH_COMPONENT,  GL_FLOAT,             false}, // D32F
    {GL_STENCIL_INDEX8,     GL_ZERO,         GL_STENCIL_INDEX,    GL_UNSIGNED_BYTE,     false}, // D0S8
}};

const std::unordered_map<BlendEquation, GLenum> kBlendEquationMap = {
    {BlendEquation::Add, GL_FUNC_ADD},
    {BlendEquation::Subtract, GL_FUNC_SUBTRACT},
    {BlendEquation::ReverseSubtract, GL_FUNC_REVERSE_SUBTRACT},
    {BlendEquation::Min, GL_MIN},
    {BlendEquation::Max, GL_MAX}
};
const std::unordered_map<BlendFunc, GLenum> kBlendFuncMap = {
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

// GLFW key map.
const std::unordered_map<int, Key::Enum> kGlfwKeyMap = {
    {GLFW_KEY_SPACE, Key::Space},
    {GLFW_KEY_APOSTROPHE, Key::Apostrophe},
    {GLFW_KEY_COMMA, Key::Comma},
    {GLFW_KEY_MINUS, Key::Minus},
    {GLFW_KEY_PERIOD, Key::Period},
    {GLFW_KEY_SLASH, Key::Slash},
    {GLFW_KEY_0, Key::Key0},
    {GLFW_KEY_1, Key::Key1},
    {GLFW_KEY_2, Key::Key2},
    {GLFW_KEY_3, Key::Key3},
    {GLFW_KEY_4, Key::Key4},
    {GLFW_KEY_5, Key::Key5},
    {GLFW_KEY_6, Key::Key6},
    {GLFW_KEY_7, Key::Key7},
    {GLFW_KEY_8, Key::Key8},
    {GLFW_KEY_9, Key::Key9},
    {GLFW_KEY_SEMICOLON, Key::Semicolon},
    {GLFW_KEY_EQUAL, Key::Equal},
    {GLFW_KEY_A, Key::A},
    {GLFW_KEY_B, Key::B},
    {GLFW_KEY_C, Key::C},
    {GLFW_KEY_D, Key::D},
    {GLFW_KEY_E, Key::E},
    {GLFW_KEY_F, Key::F},
    {GLFW_KEY_G, Key::G},
    {GLFW_KEY_H, Key::H},
    {GLFW_KEY_I, Key::I},
    {GLFW_KEY_J, Key::J},
    {GLFW_KEY_K, Key::K},
    {GLFW_KEY_L, Key::L},
    {GLFW_KEY_M, Key::M},
    {GLFW_KEY_N, Key::N},
    {GLFW_KEY_O, Key::O},
    {GLFW_KEY_P, Key::P},
    {GLFW_KEY_Q, Key::Q},
    {GLFW_KEY_R, Key::R},
    {GLFW_KEY_S, Key::S},
    {GLFW_KEY_T, Key::T},
    {GLFW_KEY_U, Key::U},
    {GLFW_KEY_V, Key::V},
    {GLFW_KEY_W, Key::W},
    {GLFW_KEY_X, Key::X},
    {GLFW_KEY_Y, Key::Y},
    {GLFW_KEY_Z, Key::Z},
    {GLFW_KEY_LEFT_BRACKET, Key::LeftBracket},
    {GLFW_KEY_BACKSLASH, Key::Backslash},
    {GLFW_KEY_RIGHT_BRACKET, Key::RightBracket},
    {GLFW_KEY_GRAVE_ACCENT, Key::Backtick},
    {GLFW_KEY_ESCAPE, Key::Escape},
    {GLFW_KEY_ENTER, Key::Enter},
    {GLFW_KEY_TAB, Key::Tab},
    {GLFW_KEY_BACKSPACE, Key::Backspace},
    {GLFW_KEY_INSERT, Key::Insert},
    {GLFW_KEY_DELETE, Key::Delete},
    {GLFW_KEY_RIGHT, Key::Right},
    {GLFW_KEY_LEFT, Key::Left},
    {GLFW_KEY_DOWN, Key::Down},
    {GLFW_KEY_UP, Key::Up},
    {GLFW_KEY_PAGE_UP, Key::PageUp},
    {GLFW_KEY_PAGE_DOWN, Key::PageDown},
    {GLFW_KEY_HOME, Key::Home},
    {GLFW_KEY_END, Key::End},
    {GLFW_KEY_CAPS_LOCK, Key::CapsLock},
    {GLFW_KEY_SCROLL_LOCK, Key::ScrollLock},
    {GLFW_KEY_NUM_LOCK, Key::NumLock},
    {GLFW_KEY_PRINT_SCREEN, Key::PrintScreen},
    {GLFW_KEY_PAUSE, Key::Pause},
    {GLFW_KEY_F1, Key::F1},
    {GLFW_KEY_F2, Key::F2},
    {GLFW_KEY_F3, Key::F3},
    {GLFW_KEY_F4, Key::F4},
    {GLFW_KEY_F5, Key::F5},
    {GLFW_KEY_F6, Key::F6},
    {GLFW_KEY_F7, Key::F7},
    {GLFW_KEY_F8, Key::F8},
    {GLFW_KEY_F9, Key::F9},
    {GLFW_KEY_F10, Key::F10},
    {GLFW_KEY_F11, Key::F11},
    {GLFW_KEY_F12, Key::F12},
    {GLFW_KEY_KP_0, Key::NumPad0},
    {GLFW_KEY_KP_1, Key::NumPad1},
    {GLFW_KEY_KP_2, Key::NumPad2},
    {GLFW_KEY_KP_3, Key::NumPad3},
    {GLFW_KEY_KP_4, Key::NumPad4},
    {GLFW_KEY_KP_5, Key::NumPad5},
    {GLFW_KEY_KP_6, Key::NumPad6},
    {GLFW_KEY_KP_7, Key::NumPad7},
    {GLFW_KEY_KP_8, Key::NumPad8},
    {GLFW_KEY_KP_9, Key::NumPad9},
    {GLFW_KEY_KP_DECIMAL, Key::KeyPadDecimal},
    {GLFW_KEY_KP_DIVIDE, Key::KPDivide},
    {GLFW_KEY_KP_MULTIPLY, Key::KPMultiply},
    {GLFW_KEY_KP_SUBTRACT, Key::KPSubtract},
    {GLFW_KEY_KP_ADD, Key::KPAdd},
    {GLFW_KEY_KP_ENTER, Key::KPEnter},
    {GLFW_KEY_KP_EQUAL, Key::KPEqual},
    {GLFW_KEY_LEFT_SHIFT, Key::LeftShift},
    {GLFW_KEY_LEFT_CONTROL, Key::LeftCtrl},
    {GLFW_KEY_LEFT_ALT, Key::LeftAlt},
    {GLFW_KEY_LEFT_SUPER, Key::LeftSuper},
    {GLFW_KEY_RIGHT_SHIFT, Key::RightShift},
    {GLFW_KEY_RIGHT_CONTROL, Key::RightCtrl},
    {GLFW_KEY_RIGHT_ALT, Key::RightAlt},
    {GLFW_KEY_RIGHT_SUPER, Key::RightSuper},
};

// GLFW mouse button map.
std::unordered_map<int, MouseButton::Enum> s_mouse_button_map = {
    {GLFW_MOUSE_BUTTON_LEFT, MouseButton::Left},
    {GLFW_MOUSE_BUTTON_MIDDLE, MouseButton::Middle},
    {GLFW_MOUSE_BUTTON_RIGHT, MouseButton::Right}
};
// clang-format on
static_assert(static_cast<int>(TextureFormat::Count) ==
                  sizeof(kTextureFormatMap) / sizeof(kTextureFormatMap[0]),
              "Texture format mapping mismatch.");

// Uniform binder.
class UniformBinder {
public:
    UniformBinder() : uniform_location_{0} {
    }

    void operator()(const int& value) {
        GL_CHECK(glUniform1i(uniform_location_, value));
    }

    void operator()(const float& value) {
        GL_CHECK(glUniform1f(uniform_location_, value));
    }

    void operator()(const Vec2& value) {
        GL_CHECK(glUniform2f(uniform_location_, value.x, value.y));
    }

    void operator()(const Vec3& value) {
        GL_CHECK(glUniform3f(uniform_location_, value.x, value.y, value.z));
    }

    void operator()(const Vec4& value) {
        GL_CHECK(glUniform4f(uniform_location_, value.x, value.y, value.z, value.w));
    }

    void operator()(const Mat3& value) {
        GL_CHECK(glUniformMatrix3fv(uniform_location_, 1, GL_FALSE, value.ptr()));
    }

    void operator()(const Mat4& value) {
        GL_CHECK(glUniformMatrix4fv(uniform_location_, 1, GL_FALSE, value.ptr()));
    }

    void updateUniform(GLint location, const UniformData& data) {
        uniform_location_ = location;
        visit(*this, data);
    }

private:
    GLint uniform_location_;
};
}  // namespace

int last_error_code = 0;
std::string last_error_description;

void SamplerCacheGL::setMaxSupportedAnisotropy(float max_anisotropy) {
    max_supported_anisotropy_ = max_anisotropy;
}

GLuint SamplerCacheGL::findOrCreate(RenderItem::SamplerInfo info) {
    auto it = cache_.find(info);
    if (it != cache_.end()) {
        return it->second;
    }

    GLuint sampler_object;
    GL_CHECK(glGenSamplers(1, &sampler_object));

    const std::unordered_map<u32, GLuint> wrapping_mode_map = {
        {0b01, GL_REPEAT}, {0b10, GL_MIRRORED_REPEAT}, {0b11, GL_CLAMP_TO_EDGE}};
    const std::unordered_map<u32, GLuint> mag_filter_map = {{0b01, GL_NEAREST}, {0b10, GL_LINEAR}};
    const std::unordered_map<u32, GLuint> min_filter_map = {{0b0101, GL_NEAREST_MIPMAP_NEAREST},
                                                            {0b0110, GL_NEAREST_MIPMAP_LINEAR},
                                                            {0b1001, GL_LINEAR_MIPMAP_NEAREST},
                                                            {0b1010, GL_LINEAR_MIPMAP_LINEAR}};

    // Parse sampler flags.
    auto flags = info.sampler_flags;
    auto u_wrapping_mode =
        (flags & SamplerFlag::maskUWrappingMode) >> SamplerFlag::shiftUWrappingMode;
    auto v_wrapping_mode =
        (flags & SamplerFlag::maskVWrappingMode) >> SamplerFlag::shiftVWrappingMode;
    auto w_wrapping_mode =
        (flags & SamplerFlag::maskWWrappingMode) >> SamplerFlag::shiftWWrappingMode;
    auto min_filter = (flags & SamplerFlag::maskMinFilter) >> SamplerFlag::shiftMinFilter;
    auto mag_filter = (flags & SamplerFlag::maskMagFilter) >> SamplerFlag::shiftMagFilter;
    auto mip_filter = (flags & SamplerFlag::maskMipFilter) >> SamplerFlag::shiftMipFilter;

    // Setup sampler object.
    GL_CHECK(glSamplerParameteri(sampler_object, GL_TEXTURE_WRAP_S,
                                 wrapping_mode_map.at(u_wrapping_mode)));
    GL_CHECK(glSamplerParameteri(sampler_object, GL_TEXTURE_WRAP_T,
                                 wrapping_mode_map.at(v_wrapping_mode)));
    GL_CHECK(glSamplerParameteri(sampler_object, GL_TEXTURE_WRAP_R,
                                 wrapping_mode_map.at(w_wrapping_mode)));
    GL_CHECK(
        glSamplerParameteri(sampler_object, GL_TEXTURE_MAG_FILTER, mag_filter_map.at(mag_filter)));
    GL_CHECK(glSamplerParameteri(sampler_object, GL_TEXTURE_MIN_FILTER,
                                 min_filter_map.at(min_filter << 2u | mip_filter)));

    if (GLAD_GL_EXT_texture_filter_anisotropic && info.max_anisotropy >= 1.0f) {
        GL_CHECK(glSamplerParameterf(sampler_object, GL_TEXTURE_MAX_ANISOTROPY,
                                     std::min(info.max_anisotropy, max_supported_anisotropy_)));
    }

    cache_.emplace(info, sampler_object);

    return sampler_object;
}

void SamplerCacheGL::clear() {
    for (const auto& sampler : cache_) {
        GL_CHECK(glDeleteSamplers(1, &sampler.second));
    }
    cache_.clear();
}

RenderContextGL::RenderContextGL(Logger& logger)
    : RenderContext(logger), max_supported_anisotropy_(0.0f) {
}

RenderContextGL::~RenderContextGL() {
    // TODO: detect resource leaks.
}

Mat4 RenderContextGL::adjustProjectionMatrix(Mat4 projection_matrix) const {
    // To map from a D3D projection matrix to an OpenGL projection matrix, we need to make the following transformations:
    // p[2][2]: f / (n-f) -> (n+f) / (n-f)
    // p[2][3]: nf / (n-f) -> 2nf / (n-f)
    float n = projection_matrix[2][3] / projection_matrix[2][2];
    float f = projection_matrix[2][3] / (1.0f + projection_matrix[2][2]);
    projection_matrix[2][2] += n / (n - f);
    projection_matrix[2][3] *= 2.0f;
    return projection_matrix;
}

bool RenderContextGL::hasFlippedViewport() const {
    return false;
}

tl::expected<void, std::string> RenderContextGL::createWindow(u16 width, u16 height,
                                                              const std::string& title,
                                                              InputCallbacks input_callbacks) {
    logger_.info("Creating window.");

    // Initialise GLFW.
    logger_.info("GLFW Version: {}", glfwGetVersionString());
#if DGA_PLATFORM == DGA_MACOS
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif
    if (!glfwInit()) {
#ifdef DGA_EMSCRIPTEN
        return tl::make_unexpected("Failed to initialise GLFW.");
#else
        const char* last_error;
        int error_code = glfwGetError(&last_error);
        return tl::make_unexpected(fmt::format(
            "Failed to initialise GLFW. Code: {:#x}. Description: {}", error_code, last_error));
#endif
    }

#ifndef DGA_EMSCRIPTEN
    glfwSetErrorCallback([](int error, const char* description) {
        last_error_code = error;
        last_error_description = description;
    });
#endif

    // Set window hints.
#if DW_GL_VERSION == DW_GL_410
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
#elif !defined(DGA_EMSCRIPTEN)
#error Unsupported: GLES 3.0 on non Web platform.
#endif
    // TODO: Support resizing.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Select monitor.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    // Get DPI settings.
#ifndef DGA_EMSCRIPTEN
    glfwGetMonitorContentScale(monitor, &window_scale_.x, &window_scale_.y);
#else
    // Unsupported in emscripten.
    window_scale_ = {1.0f, 1.0f};
#endif

    // Create the window.
    window_ = glfwCreateWindow(static_cast<int>(width * window_scale_.x),
                               static_cast<int>(height * window_scale_.y), title.c_str(), nullptr,
                               nullptr);
    if (!window_) {
        // Failed to create window.
        return tl::make_unexpected(
            fmt::format("glfwCreateWindow failed. Code: {:#x}. Description: {}", last_error_code,
                        last_error_description));
    }
    Vec2i fb_size = framebufferSize();
    backbuffer_width_ = static_cast<u16>(fb_size.x);
    backbuffer_height_ = static_cast<u16>(fb_size.y);
    glfwMakeContextCurrent(window_);
#ifndef DGA_EMSCRIPTEN
    glfwSwapInterval(0);
#endif
    glfwSetWindowUserPointer(window_, static_cast<void*>(this));

    // Setup callbacks.
    on_key_ = input_callbacks.on_key;
    on_char_input_ = input_callbacks.on_char_input;
    on_mouse_button_ = input_callbacks.on_mouse_button;
    on_mouse_move_ = input_callbacks.on_mouse_move;
    on_mouse_scroll_ = input_callbacks.on_mouse_scroll;
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int, int action, int) {
        auto& ctx = *static_cast<RenderContextGL*>(glfwGetWindowUserPointer(window));
        if (!ctx.on_key_) {
            return;
        }

        // Look up key.
        auto key_it = kGlfwKeyMap.find(key);
        if (key_it == kGlfwKeyMap.end()) {
            ctx.logger_.warn("Unknown key code {}", key);
            return;
        }

        // If we are repeating a key, ignore.
        if (action == GLFW_REPEAT) {
            return;
        }

        if (action == GLFW_PRESS) {
            ctx.on_key_(key_it->second, Modifier::None, true);
        } else if (action == GLFW_RELEASE) {
            ctx.on_key_(key_it->second, Modifier::None, false);
        }
    });
    glfwSetCharCallback(window_, [](GLFWwindow* window, unsigned int c) {
        auto& ctx = *static_cast<RenderContextGL*>(glfwGetWindowUserPointer(window));
        if (!ctx.on_char_input_) {
            return;
        }
#ifdef DGA_MSVC
        std::wstring_convert<std::codecvt_utf8<i32>, i32> conv;
#else
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
#endif
        ctx.on_char_input_(conv.to_bytes(static_cast<char32_t>(c)));
    });
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* window, int button, int action, int) {
        auto& ctx = *static_cast<RenderContextGL*>(glfwGetWindowUserPointer(window));
        if (!ctx.on_mouse_button_) {
            return;
        }

        auto mouse_button = s_mouse_button_map.find(button);
        if (mouse_button == s_mouse_button_map.end()) {
            ctx.logger_.warn("Unknown mouse button {}", button);
            return;
        }
        if (action == GLFW_PRESS) {
            ctx.on_mouse_button_(mouse_button->second, true);
        } else if (action == GLFW_RELEASE) {
            ctx.on_mouse_button_(mouse_button->second, false);
        }
    });
    glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double x, double y) {
        auto& ctx = *static_cast<RenderContextGL*>(glfwGetWindowUserPointer(window));
        if (!ctx.on_mouse_move_) {
            return;
        }

        ctx.on_mouse_move_({static_cast<int>(x), static_cast<int>(y)});
    });
    glfwSetScrollCallback(window_, [](GLFWwindow* window, double xoffset, double yoffset) {
        auto& ctx = *static_cast<RenderContextGL*>(glfwGetWindowUserPointer(window));
        if (!ctx.on_mouse_scroll_) {
            return;
        }

        ctx.on_mouse_scroll_(Vec2(static_cast<float>(xoffset), static_cast<float>(yoffset)));
    });

    // Initialise GL function pointers.
    if (!gladLoadGL(glfwGetProcAddress)) {
        return tl::make_unexpected("gladLoadGL failed.");
    }

    // Set debug callbacks.
#ifndef NDEBUG
    if (GLAD_GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, &logger_);
    }
#endif

    GL_CHECK(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_supported_anisotropy_));
    sampler_cache_.setMaxSupportedAnisotropy(max_supported_anisotropy_);

    // Print GL information.
    logger_.info("OpenGL: {} - GLSL: {}", glGetString(GL_VERSION),
                 glGetString(GL_SHADING_LANGUAGE_VERSION));
    logger_.info("OpenGL Renderer: {}", glGetString(GL_RENDERER));
    logger_.info("Extensions:");
    logger_.info("- GL_KHR_debug: {}", static_cast<bool>(GLAD_GL_KHR_debug));
    logger_.info("- GLAD_GL_ARB_gl_spirv: {}", static_cast<bool>(GLAD_GL_ARB_gl_spirv));
    logger_.info("- GLAD_GL_EXT_texture_filter_anisotropic: {}",
                 static_cast<bool>(GLAD_GL_EXT_texture_filter_anisotropic));
    logger_.info("Capabilities:");
    logger_.info("- Max supported anisotropy: {}", max_supported_anisotropy_);

    // Hand off context to render thread.
    glfwMakeContextCurrent(nullptr);

    return {};
}

void RenderContextGL::destroyWindow() {
    if (window_) {
        sampler_cache_.clear();

        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

void RenderContextGL::processEvents() {
    glfwPollEvents();
}

bool RenderContextGL::isWindowClosed() const {
    return glfwWindowShouldClose(window_) != 0;
}

Vec2i RenderContextGL::windowSize() const {
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    return Vec2i{window_width, window_height};
}

Vec2 RenderContextGL::windowScale() const {
    return window_scale_;
}

Vec2i RenderContextGL::framebufferSize() const {
    int fb_width, fb_height;
    glfwGetFramebufferSize(window_, &fb_width, &fb_height);
    return Vec2i{fb_width, fb_height};
}

void RenderContextGL::startRendering() {
    glfwMakeContextCurrent(window_);

    GL_CHECK(glGenVertexArrays(1, &vao_));
    GL_CHECK(glBindVertexArray(vao_));
}

void RenderContextGL::stopRendering() {
    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glDeleteVertexArrays(1, &vao_));
}

void RenderContextGL::prepareFrame() {
}

void RenderContextGL::processCommandList(std::vector<RenderCommand>& command_list) {
    assert(window_);
    for (auto& command : command_list) {
        visit(*this, command);
    }
}

bool RenderContextGL::frame(const Frame* frame) {
    assert(window_);

    // Upload transient vertex/element buffer data.
    auto& tvb = frame->transient_vb_storage;
    if (tvb.handle && tvb.size > 0) {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_map_.at(*tvb.handle).vertex_buffer));
        GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, tvb.size, tvb.data.data()));
    }
    auto& tib = frame->transient_ib_storage;
    if (tib.handle && tib.size > 0) {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                              index_buffer_map_.at(*tib.handle).element_buffer));
        GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, tib.size, tib.data.data()));
    }

    // Process render queues.
    for (auto& q : frame->render_queues) {
        // Set up framebuffer.
        u16 fb_width, fb_height;
        if (q.frame_buffer) {
            FrameBufferData& fb_data = frame_buffer_map_.at(*q.frame_buffer);
            fb_width = fb_data.width;
            fb_height = fb_data.height;
            GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fb_data.frame_buffer));
        } else {
            fb_width = backbuffer_width_;
            fb_height = backbuffer_height_;
            GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        }

        // TODO: Not sure what to use these for yet.
        (void)fb_width;
        (void)fb_height;

        // Clear frame buffer.
        if (q.clear_parameters) {
            auto colour = q.clear_parameters->colour;
            GL_CHECK(glClearColor(colour.r(), colour.g(), colour.b(), colour.a()));
            GLbitfield clear_mask = 0;
            if (q.clear_parameters->clear_colour) {
                clear_mask |= GL_COLOR_BUFFER_BIT;
            }
            if (q.clear_parameters->clear_depth) {
                clear_mask |= GL_DEPTH_BUFFER_BIT;
            }
            GL_CHECK(glClear(clear_mask));
        }

        // Render items.
        for (uint i = 0; i < q.render_items.size(); ++i) {
            auto* previous = i > 0 ? &q.render_items[i - 1] : nullptr;
            auto* current = &q.render_items[i];

            // Update render state.
            if (!previous || previous->cull_face_enabled != current->cull_face_enabled) {
                if (current->cull_face_enabled) {
                    GL_CHECK(glEnable(GL_CULL_FACE));
                } else {
                    GL_CHECK(glDisable(GL_CULL_FACE));
                }
            }
            if (!previous || previous->cull_front_face != current->cull_front_face) {
                GL_CHECK(
                    glFrontFace(current->cull_front_face == CullFrontFace::CCW ? GL_CCW : GL_CW));
            }
            if (!previous || previous->polygon_mode != current->polygon_mode) {
                GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, current->polygon_mode == PolygonMode::Fill
                                                              ? GL_FILL
                                                              : GL_LINE));
            }
            if (!previous || previous->depth_enabled != current->depth_enabled) {
                if (current->depth_enabled) {
                    GL_CHECK(glEnable(GL_DEPTH_TEST));
                } else {
                    GL_CHECK(glDisable(GL_DEPTH_TEST));
                }
            }
            if (!previous || previous->blend_enabled != current->blend_enabled) {
                if (current->blend_enabled) {
                    GL_CHECK(glEnable(GL_BLEND));
                } else {
                    GL_CHECK(glDisable(GL_BLEND));
                }
            }
            if (!previous || previous->blend_equation_rgb != current->blend_equation_rgb ||
                previous->blend_equation_a != current->blend_equation_a) {
                GL_CHECK(glBlendEquationSeparate(kBlendEquationMap.at(current->blend_equation_rgb),
                                                 kBlendEquationMap.at(current->blend_equation_a)));
            }
            if (!previous || previous->blend_src_rgb != current->blend_src_rgb ||
                previous->blend_src_a != current->blend_src_a ||
                previous->blend_dest_rgb != current->blend_dest_rgb ||
                previous->blend_dest_a != current->blend_dest_a) {
                GL_CHECK(glBlendFuncSeparate(kBlendFuncMap.at(current->blend_src_rgb),
                                             kBlendFuncMap.at(current->blend_dest_rgb),
                                             kBlendFuncMap.at(current->blend_src_a),
                                             kBlendFuncMap.at(current->blend_dest_a)));
            }

            // Bind Program.
            ProgramData& program_data = program_map_.at(*current->program);
            if (!previous || previous->program != current->program) {
                GL_CHECK(glUseProgram(program_data.program));
            }

            // Bind uniforms.
            UniformBinder binder;
            for (auto& entry : current->uniforms) {
                auto uniform_name = entry.first;
                auto location = program_data.uniform_location_map.find(uniform_name);
                GLint uniform_location;
                if (location != program_data.uniform_location_map.end()) {
                    uniform_location = (*location).second;
                } else {
                    // A uniform inside a (converted) uniform block may have been remapped to a
                    // location inside a struct uniform caled _<id>. When looking up the uniform
                    // location, take this into account.
                    auto uniform_remap_id = program_data.uniform_remap_ids.find(uniform_name);
                    std::string remapped_uniform_name =
                        uniform_remap_id == program_data.uniform_remap_ids.end()
                            ? uniform_name
                            : fmt::format("_{}.{}", uniform_remap_id->second, uniform_name);

                    // Look up the uniform location.
                    GL_CHECK(uniform_location = glGetUniformLocation(
                                 program_data.program, remapped_uniform_name.c_str()));
                    program_data.uniform_location_map.emplace(uniform_name, uniform_location);
                    if (uniform_location == -1) {
                        logger_.warn("[Frame] Unknown or optimised out uniform '{}', skipping.",
                                     uniform_name);
                    }
                }
                if (uniform_location == -1) {
                    continue;
                }
                binder.updateUniform(uniform_location, entry.second);
            }

            // Bind textures.
            uint texture_count =
                std::max(previous ? previous->textures.size() : 0, current->textures.size());
            for (uint j = 0; j < texture_count; ++j) {
                const auto& texture = current->textures[j];

                GL_CHECK(glActiveTexture(GL_TEXTURE0 + j));
                if (j >= current->textures.size()) {
                    // If the iterator is greater than the number of current textures, this means we
                    // are hitting texture units used by the previous render item. Unbind it as it's
                    // no longer in use.
                    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
                    GL_CHECK(glBindSampler(j, 0));
                } else if (!texture) {
                    // Unbind the texture if the handle is invalid.
                    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
                    GL_CHECK(glBindSampler(j, 0));
                } else {
                    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_map_.at(texture->handle)));
                    if (texture->sampler_info.sampler_flags != 0) {
                        GLuint sampler = sampler_cache_.findOrCreate(texture->sampler_info);
                        GL_CHECK(glBindSampler(j, sampler));
                    }
                }
            }

            // Bind vertex data.
            if (!previous || previous->vb != current->vb) {
                if (current->vb) {
                    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                                          vertex_buffer_map_.at(*current->vb).vertex_buffer));
                }
            }

            // Bind attributes.
            for (uint attrib = 0; attrib < current_vertex_decl.attributes_.size(); ++attrib) {
                GL_CHECK(glDisableVertexAttribArray(attrib));
            }
            if (current->vb) {
                current_vertex_decl = current->vertex_decl_override.empty()
                                          ? vertex_buffer_map_.at(*current->vb).decl
                                          : current->vertex_decl_override;
                setupVertexArrayAttributes(current_vertex_decl, current->vb_offset);
            } else {
                current_vertex_decl = VertexDecl{};
            }

            // Bind element data.
            if (!previous || previous->ib != current->ib) {
                if (current->ib) {
                    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                          index_buffer_map_.at(*current->ib).element_buffer));
                } else {
                    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                }
            }

            // Set viewport masks. These will need to be unset after processing the command to avoid
            // clobbering the next glClear call.
            if (!current->colour_write) {
                GL_CHECK(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
            }
            if (!current->depth_write) {
                GL_CHECK(glDepthMask(GL_FALSE));
            }
            if (current->scissor_enabled) {
                GL_CHECK(glEnable(GL_SCISSOR_TEST));
                GL_CHECK(glScissor(current->scissor_x,
                                   fb_height - current->scissor_y - current->scissor_height,
                                   current->scissor_width, current->scissor_height));
            }

            // Submit.
            if (current->primitive_count > 0) {
                if (current->ib) {
                    GLenum element_type = index_buffer_map_.at(*current->ib).type;
                    GL_CHECK(glDrawElements(
                        GL_TRIANGLES, current->primitive_count * 3, element_type,
                        reinterpret_cast<void*>(static_cast<std::intptr_t>(current->ib_offset))));
                } else {
                    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, current->primitive_count * 3));
                }
            }

            // Restore viewport masks.
            if (!current->colour_write) {
                GL_CHECK(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
            }
            if (!current->depth_write) {
                GL_CHECK(glDepthMask(GL_TRUE));
            }
            if (current->scissor_enabled) {
                GL_CHECK(glDisable(GL_SCISSOR_TEST));
            }
        }
    }

    // Swap buffers.
    glfwSwapBuffers(window_);

    // Continue rendering.
    return true;
}

void RenderContextGL::operator()(const cmd::CreateVertexBuffer& c) {
    // Create vertex buffer object.
    GLenum usage = mapBufferUsage(c.usage);
    GLuint vbo;
    GL_CHECK(glGenBuffers(1, &vbo));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    if (c.data.data()) {
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, c.data.size(), c.data.data(), usage));
    } else {
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, c.size, nullptr, usage));
    }
    vertex_buffer_map_.insert({c.handle, VertexBufferData{vbo, c.decl, usage, c.size}});
}

void RenderContextGL::operator()(const cmd::UpdateVertexBuffer& c) {
    auto& vb_data = vertex_buffer_map_.at(c.handle);
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vb_data.vertex_buffer));
    if (c.data.size() > vb_data.size) {
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, c.data.size(), c.data.data(), vb_data.usage));
        vb_data.size = c.data.size();
    } else {
        GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, c.offset, c.data.size(), c.data.data()));
    }
}

void RenderContextGL::operator()(const cmd::DeleteVertexBuffer& c) {
    auto it = vertex_buffer_map_.find(c.handle);
    GL_CHECK(glDeleteBuffers(1, &it->second.vertex_buffer));
    vertex_buffer_map_.erase(it);
}

void RenderContextGL::operator()(const cmd::CreateIndexBuffer& c) {
    // Create element buffer object.
    GLenum usage = mapBufferUsage(c.usage);
    GLuint ebo;
    GL_CHECK(glGenBuffers(1, &ebo));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    if (c.data.data()) {
        GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, c.data.size(), c.data.data(), usage));
    } else {
        GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, c.size, nullptr, usage));
    }
    index_buffer_map_.insert(
        {c.handle,
         IndexBufferData{ebo,
                         static_cast<GLenum>(c.type == IndexBufferType::U16 ? GL_UNSIGNED_SHORT
                                                                            : GL_UNSIGNED_INT),
                         usage, c.size}});
}

void RenderContextGL::operator()(const cmd::UpdateIndexBuffer& c) {
    auto& ib_data = index_buffer_map_.at(c.handle);
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib_data.element_buffer));
    if (c.data.size() > ib_data.size) {
        GL_CHECK(
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, c.data.size(), c.data.data(), ib_data.usage));
        ib_data.size = c.data.size();
    } else {
        GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, c.offset, c.data.size(), c.data.data()));
    }
}

void RenderContextGL::operator()(const cmd::DeleteIndexBuffer& c) {
    auto it = index_buffer_map_.find(c.handle);
    GL_CHECK(glDeleteBuffers(1, &it->second.element_buffer));
    index_buffer_map_.erase(it);
}

void RenderContextGL::operator()(const cmd::CreateShader& c) {
    static std::unordered_map<ShaderStage, GLenum> shader_type_map = {
        {ShaderStage::Vertex, GL_VERTEX_SHADER}, {ShaderStage::Fragment, GL_FRAGMENT_SHADER}};
    GLuint shader = 0;
    GL_CHECK(shader = glCreateShader(shader_type_map.at(c.stage)));

    // Convert SPIR-V into GLSL.
    spirv_cross::CompilerGLSL glsl{reinterpret_cast<const u32*>(c.data.data()),
                                   c.data.size() / sizeof(u32)};
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    // Remap texture binding locations.
    u32 next_texture_binding_location = 0;
    for (const auto& resource : resources.sampled_images) {
        u32 set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        u32 binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
        u32 new_binding = next_texture_binding_location++;
        logger_.debug(
            "Remapping sampled image with location(set={}, binding={}) to location(binding={})",
            set, binding, new_binding);
        glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);
        glsl.set_decoration(resource.id, spv::DecorationBinding, new_binding);
    }

    // If we use the 'emit_uniform_buffer_as_plain_uniforms' option on an empty uniform block,
    // variables are going to be prefixed with _<id>, where <id> is the resource ID in SPIR-V.
    // In those cases, we need to store that information so it can be looked up during frame().
    std::unordered_map<std::string, u32> uniform_remap_ids;
    for (const auto& resource : resources.uniform_buffers) {
        if (!glsl.get_name(resource.id).empty()) {
            continue;
        }

        const spirv_cross::SPIRType& type = glsl.get_type(resource.base_type_id);
        usize member_count = type.member_types.size();
        for (usize i = 0; i < member_count; ++i) {
            uniform_remap_ids[glsl.get_member_name(type.self, i)] = resource.id;
        }
    }

    // Compile to GLSL, ready to give to GL driver.
    spirv_cross::CompilerGLSL::Options options;
    options.emit_push_constant_as_uniform_buffer = true;
    options.emit_uniform_buffer_as_plain_uniforms = true;
#if DW_GL_VERSION == DW_GL_410
    options.version = 410;
    options.es = false;
#elif DW_GL_VERSION == DW_GLES_300
    options.version = 300;
    options.es = true;
#else
#error "Unsupported DW_GL_VERSION"
#endif
    glsl.set_common_options(options);
    std::string source = glsl.compile();

    // Postprocess the GLSL to remove a GL 4.2 extension, which doesn't exist on macOS.
#if DGA_PLATFORM == DGA_MACOS
    source = dga::strReplaceAll(source, "#extension GL_ARB_shading_language_420pack : require",
                                "#extension GL_ARB_shading_language_420pack : disable");
#endif
    /*
    // Post process source to extract uniform buffers.
    for (std::size_t next_ubo = source.find(", std140) uniform"); next_ubo != std::string::npos;
         next_ubo = source.find(", std140) uniform")) {
        std::size_t ubo_begin = source.rfind("layout", next_ubo);
        std::size_t ubo_brace = source.find('{', next_ubo);
        std::size_t ubo_end = source.find('}', ubo_begin);

        // Extract UBO name
        std::size_t ubo_semicolon = source.find(';', ubo_end);
        std::string ubo_name = source.substr(ubo_end + 2, ubo_semicolon - (ubo_end + 2));
        std::string ubo_prefix = ubo_name + "_";

        // Erase footer.
        source.erase(ubo_end, ubo_semicolon + 1 - ubo_end);

        // Erase header.
        source.erase(ubo_begin, ubo_brace - ubo_begin + 1);  // include \n character

        // Update uniforms.
        for (std::size_t uniform_begin = source.find("    ", ubo_begin + 1);
             uniform_begin < ubo_end; uniform_begin = source.find("    ", ubo_begin + 1)) {
            source.replace(uniform_begin, 4, "uniform ");
            std::size_t space_before_identifier = source.find(' ', uniform_begin + 9);

            // Extract uniform name.
            std::size_t semicolon_after_identifier = source.find(';', space_before_identifier + 1);
            std::string uniform_name =
                source.substr(space_before_identifier + 1,
                              semicolon_after_identifier - (space_before_identifier + 1));

            // Append prefix.
            source.insert(space_before_identifier + 1, ubo_prefix);

            // Replace usage in the rest of the shader.
            source = dga::strReplaceAll(source, ubo_name + "." + uniform_name,
                                        ubo_prefix + uniform_name);
        }
    }
     */

    // Compile the shader.
    logger_.debug("Decompiled GLSL from SPIR-V: {}", source);
    const char* sources[] = {source.c_str()};
    GL_CHECK(glShaderSource(shader, 1, sources, nullptr));
    GL_CHECK(glCompileShader(shader));

    // Check compilation result.
    GLint result;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &result));
    if (result == GL_FALSE) {
        int info_log_length;
        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length));
        std::string error_message(info_log_length, '\0');
        GL_CHECK(glGetShaderInfoLog(shader, info_log_length, nullptr, error_message.data()));
        logger_.error("[CreateShader] Shader Compile Error: {}", error_message);
        return;
    }

    shader_map_.emplace(c.handle, ShaderData{shader, std::move(uniform_remap_ids)});
}

void RenderContextGL::operator()(const cmd::DeleteShader& c) {
    auto it = shader_map_.find(c.handle);
    GL_CHECK(glDeleteShader(it->second.shader));
    shader_map_.erase(it);
}

void RenderContextGL::operator()(const cmd::CreateProgram& c) {
    ProgramData program_data;
    program_data.program = glCreateProgram();
    assert(program_data.program != 0);
    program_map_.emplace(c.handle, program_data);
}

void RenderContextGL::operator()(const cmd::AttachShader& c) {
    const auto& shader_data = shader_map_.at(c.shader_handle);
    auto& program_data = program_map_.at(c.handle);
    GL_CHECK(glAttachShader(program_data.program, shader_data.shader));
    program_data.uniform_remap_ids.insert(shader_data.uniform_remap_ids.begin(),
                                          shader_data.uniform_remap_ids.end());
}

void RenderContextGL::operator()(const cmd::LinkProgram& c) {
    GLuint program = program_map_.at(c.handle).program;
    GL_CHECK(glLinkProgram(program));

    // Check the result of the link process.
    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        int info_log_length;
        GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length));
        std::string error_message(info_log_length, '\0');
        GL_CHECK(glGetProgramInfoLog(program, info_log_length, nullptr, error_message.data()));
        logger_.error("[CreateShader] Shader Compile Error: {}", error_message);
    }
}

void RenderContextGL::operator()(const cmd::DeleteProgram& c) {
    auto it = program_map_.find(c.handle);
    if (it != program_map_.end()) {
        GL_CHECK(glDeleteProgram(it->second.program));
        program_map_.erase(it);
    } else {
        logger_.error("[DeleteProgram] Unable to find program with handle {}", u32(c.handle));
    }
}

void RenderContextGL::operator()(const cmd::CreateTexture2D& c) {
    GLuint texture;
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));

    // Give image data to OpenGL.
    TextureFormatGL format = kTextureFormatMap[static_cast<int>(c.format)];
    logger_.debug(
        "[CreateTexture2D] format {} - internal fmt: {:#x} - internal fmt srgb: {:#x} - fmt: {:#x} "
        "- type: {:#x}",
        static_cast<u32>(c.format), format.internal_format, format.internal_format_srgb,
        format.format, format.type);
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, format.internal_format, c.width, c.height, 0,
                          format.format, format.type, c.data.data()));

    // Generate mipmaps.
    if (c.generate_mipmaps) {
        GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
    }

    // Add texture.
    texture_map_.emplace(c.handle, texture);
}

void RenderContextGL::operator()(const cmd::DeleteTexture& c) {
    auto it = texture_map_.find(c.handle);
    GL_CHECK(glDeleteTextures(1, &it->second));
    texture_map_.erase(it);
}

void RenderContextGL::operator()(const cmd::CreateFrameBuffer& c) {
    FrameBufferData fb_data;
    fb_data.textures = c.textures;
    fb_data.width = c.width;
    fb_data.height = c.height;

    GL_CHECK(glGenFramebuffers(1, &fb_data.frame_buffer));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fb_data.frame_buffer));

    // Bind colour buffers.
    std::vector<GLenum> draw_buffers;
    u8 attachment = 0;
    for (auto texture : fb_data.textures) {
        auto gl_texture = texture_map_.find(texture);
        assert(gl_texture != texture_map_.end());
        draw_buffers.emplace_back(GL_COLOR_ATTACHMENT0 + attachment++);
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, draw_buffers.back(), GL_TEXTURE_2D,
                                        gl_texture->second, 0));
    }
    GL_CHECK(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));

    // Create depth buffer.
    GL_CHECK(glGenRenderbuffers(1, &fb_data.depth_render_buffer));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, fb_data.depth_render_buffer));
    GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, c.width, c.height));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                       fb_data.depth_render_buffer));

    // Check frame buffer status.
    GLenum status;
    GL_CHECK(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        logger_.error("[CreateFrameBuffer] The framebuffer is not complete. Status: {:#x}", status);
    }

    // Unbind.
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    // Add to map.
    frame_buffer_map_.emplace(c.handle, fb_data);
}

void RenderContextGL::operator()(const cmd::DeleteFrameBuffer&) {
    // TODO: unimplemented.
}

void RenderContextGL::setupVertexArrayAttributes(const VertexDecl& decl, uint vb_offset) {
    static std::unordered_map<VertexDecl::AttributeType, GLenum> attribute_type_map = {
        {VertexDecl::AttributeType::Float, GL_FLOAT},
        {VertexDecl::AttributeType::Uint8, GL_UNSIGNED_BYTE}};
    u16 attrib_counter = 0;
    for (auto& attrib : decl.attributes_) {
        // Decode attribute.
        VertexDecl::Attribute attribute;
        usize count;
        VertexDecl::AttributeType type;
        bool normalised;
        VertexDecl::decodeAttributes(attrib.first, attribute, count, type, normalised);

        // Convert type.
        auto gl_type = attribute_type_map.find(type);
        if (gl_type == attribute_type_map.end()) {
            logger_.warn("[SetupVertexArrayAttributes] Unknown attribute type: {}", (uint)type);
            continue;
        }

        //        logger_.debug("[SetupVertexArrayAttributes] Attrib {}: Count='{}' Type='{}'
        //        Stride='{}' Offset='{}'",
        //                    attrib_counter, count, static_cast<int>(gl_type->first), decl.stride_,
        //                    reinterpret_cast<uintptr_t>(attrib.second));

        // Set attribute.
        GL_CHECK(glEnableVertexAttribArray(attrib_counter));
        GL_CHECK(glVertexAttribPointer(attrib_counter, count, gl_type->second,
                                       static_cast<GLboolean>(normalised ? GL_TRUE : GL_FALSE),
                                       decl.stride_, attrib.second + vb_offset));
        attrib_counter++;
    }
}
}  // namespace gfx
}  // namespace dw
