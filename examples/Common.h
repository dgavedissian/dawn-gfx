/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include <dawn-gfx/Renderer.h>
#include <dawn-gfx/Shader.h>
#include <dawn-gfx/MeshBuilder.h>
#include <dawn-gfx-imgui/ImGuiBackend.h>

#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef DGA_EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

using namespace dw::gfx;

class StdoutLogger : public Logger {
public:
    void log(LogLevel level, const std::string& str) const override {
        (level == LogLevel::Error ? std::cerr : std::cout) << str << std::endl;
    }
};

class Example {
public:
    static int runMain(std::unique_ptr<Example> example, const char* window_title) {
        example->initRenderer(RendererType::Vulkan, 1024, 768, window_title);
        example->start();
#ifdef DGA_EMSCRIPTEN
        // void emscripten_set_main_loop(em_callback_func func, int fps, int
        // simulate_infinite_loop);
        emscripten_set_main_loop_arg([](void* arg) { reinterpret_cast<Example*>(arg)->tick(); },
                                     reinterpret_cast<void*>(example.get()), 0, 1);
        std::cout << "After loop." << std::endl;
#else
        while (example->running()) {
            example->tick();
        }
#endif
        example->stop();
        return 0;
    }

    Example() : r(logger_), frame_start_time_(std::chrono::steady_clock::now()) {
    }

    void initRenderer(RendererType type, u16 width, u16 height, const char* window_title) {
        r.init(type, width, height, window_title, InputCallbacks{}, true);

        ImGui::SetCurrentContext(ImGui::CreateContext());
        imgui_backend_ = std::make_unique<ImGuiBackend>(r, ImGui::GetIO());
    }

    void tick() {
        imgui_backend_->newFrame();
        ImGui::NewFrame();

        render(dt_);

        ImGui::SetNextWindowPos({10, 10});
        ImGui::SetNextWindowSize({150, 50});
        if (!ImGui::Begin("FPS", nullptr,
                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::End();
            return;
        }
        ImGui::Text("FPS:   %f", 1.0f / dt_);
        ImGui::Text("Frame: %.4f ms", dt_ * 1000.0f);
        ImGui::End();

        // Render frame.
        ImGui::Render();
        imgui_backend_->render(ImGui::GetDrawData());
        if (!r.frame()) {
            running_ = false;
        }

        // Update delta time.
        auto now = std::chrono::steady_clock::now();
        dt_ = std::chrono::duration_cast<std::chrono::duration<float>>(now - frame_start_time_)
                  .count();
        frame_start_time_ = now;
    }

    bool running() const {
        return running_;
    }

    u16 width() const {
        return r.backbufferSize().x;
    }

    u16 height() const {
        return r.backbufferSize().y;
    }

    float aspect() const {
        return static_cast<float>(r.backbufferSize().x) / static_cast<float>(r.backbufferSize().y);
    }

    virtual void start() = 0;
    virtual void render(float dt) = 0;
    virtual void stop() = 0;

    Renderer r;

private:
    StdoutLogger logger_;
    std::unique_ptr<ImGuiBackend> imgui_backend_;

    float dt_ = 1.0f / 60.0f;
    std::chrono::steady_clock::time_point frame_start_time_;

    bool running_ = true;
};

#define DEFINE_MAIN(_Example)                                             \
    int main(int, char**) {                                               \
        return Example::runMain(std::make_unique<_Example>(), #_Example); \
    }

namespace util {
inline Mat4 createProjMatrix(float n, float f, float fov_y, float aspect) {
    auto tangent = static_cast<float>(tan(fov_y * M_DEGTORAD_OVER_2));  // tangent of half fovY
    float v = n * tangent * 2;                                          // half height of near plane
    float h = v * aspect;                                               // half width of near plane
    return Mat4::OpenGLPerspProjRH(n, f, h, v);
}

inline uint createFullscreenQuad(Renderer& r, VertexBufferHandle& vb) {
    // clang-format off
    float vertices[] = {
        // Position       | UV
        -1.0f, -1.0f, 0.0f, 0.0f,
        3.0f,  -1.0f, 2.0f, 0.0f,
        -1.0f,  3.0f, 0.0f, 2.0f};
    // clang-format on
    VertexDecl decl;
    decl.begin()
        .add(VertexDecl::Attribute::Position, 2, VertexDecl::AttributeType::Float)
        .add(VertexDecl::Attribute::TexCoord0, 2, VertexDecl::AttributeType::Float)
        .end();
    vb = r.createVertexBuffer(Memory(vertices, sizeof(vertices)), decl);
    return 3;
}

inline ShaderHandle loadShader(Renderer& r, ShaderStage type, const std::string& source_file) {
    std::ifstream shader(source_file);
    std::string shader_source((std::istreambuf_iterator<char>(shader)),
                              std::istreambuf_iterator<char>());
    auto spv_result = compileGLSL(type, shader_source);
    if (!spv_result) {
        throw std::runtime_error("Compile error: " + spv_result.error().compile_error);
    }
    return r.createShader(type, spv_result.value().entry_point,
                          Memory(std::move(spv_result.value().spirv)));
}

inline TextureHandle loadTexture(Renderer& r, const std::string& texture) {
    int width, height, bpp;
    stbi_uc* buffer = stbi_load(texture.c_str(), &width, &height, &bpp, 4);
    if (!buffer) {
        throw std::runtime_error("Failed to load " + texture);
    }
    Memory data(buffer, static_cast<u32>(width * height * 4),
                [](byte* buffer) { stbi_image_free(buffer); });
    return r.createTexture2D(static_cast<u16>(width), static_cast<u16>(height),
                             TextureFormat::RGBA8, std::move(data));
}

inline std::string media(const std::string& media_name) {
#ifdef DGA_EMSCRIPTEN
    return "/media/" + media_name;
#else
    return "../../examples/media/" + media_name;
#endif
}
}  // namespace util