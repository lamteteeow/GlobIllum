#include "context.h"
#include "gi/ray.h"
#include "gi/hit.h"
#include "gi/material.h"
#include "gi/light.h"
#include "gi/random.h"
#include "gi/algorithm.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imfilebrowser.h"
#include "render.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "emscripten/emscripten_mainloop_stub.h"
#endif

// imgui glsl version
#ifdef __EMSCRIPTEN__
constexpr auto glslVersion = "#version 100";
#else
constexpr auto glslVersion = "#version 130";
#endif

// ---------------------------------------------------------------------------------
// callbacks

void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    // get source of error
    std::string src;
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            src = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            src = "WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            src = "SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            src = "THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            src = "APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            src = "OTHER";
            break;
    }

    // get type of error
    std::string typ;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            typ = "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typ = "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typ = "UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typ = "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typ = "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_OTHER:
            typ = "OTHER";
            break;
        case GL_DEBUG_TYPE_MARKER:
            typ = "MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            typ = "PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            typ = "POP_GROUP";
            break;
    }

    // get severity
    std::string sev;
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            sev = "NOTIFICATION";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            sev = "LOW";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            sev = "MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            sev = "HIGH";
            break;
    }

    fprintf(stderr, "GL_DEBUG: Severity: %s, Source: %s, Type: %s.\nMessage: %s\n", sev.c_str(), src.c_str(), typ.c_str(), message);
}

void embreeErrorFunc(void *, const RTCError code, const char *str) {
    if (code == RTC_ERROR_NONE) return;
    fprintf(stderr, "Embree Error: ");
    switch (code) {
        case RTC_ERROR_UNKNOWN:
            fprintf(stderr, "RTC_ERROR_UNKNOWN");
            break;
        case RTC_ERROR_INVALID_ARGUMENT:
            fprintf(stderr, "RTC_ERROR_INVALID_ARGUMENT");
            break;
        case RTC_ERROR_INVALID_OPERATION:
            fprintf(stderr, "RTC_ERROR_INVALID_OPERATION");
            break;
        case RTC_ERROR_OUT_OF_MEMORY:
            fprintf(stderr, "RTC_ERROR_OUT_OF_MEMORY");
            break;
        case RTC_ERROR_UNSUPPORTED_CPU:
            fprintf(stderr, "RTC_ERROR_UNSUPPORTED_CPU");
            break;
        case RTC_ERROR_CANCELLED:
            fprintf(stderr, "RTC_ERROR_CANCELLED");
            break;
        default:
            fprintf(stderr, "invalid error code");
    }
    if (str)
        fprintf(stderr, " (%s)\n", str);
    exit(1);
}

static std::atomic<uint64_t> num_bytes_embree(0);
bool embreeMemFunc(void *userPtr, ssize_t bytes, bool post) {
    num_bytes_embree += bytes;
    return true;
}

void glfwErrorFunc(int error, const char *description) {
    fprintf(stderr, "GLFW Error No. %i: %s\n", error, description);
}

void glfwDropCallback(GLFWwindow* window, int path_count, const char* paths[]) {
    Context* context = (Context*)glfwGetWindowUserPointer(window);
    for (int i = 0; i < path_count; ++i)
        context->load(paths[i]);
}

// ---------------------------------------------------------------------------------
// input handling

// returns if a new rendering should be started
static float cam_move_speed = 5.f;
bool keyboard_handler(GLFWwindow *window, Context& ctx, float dt) {
    if (ImGui::GetIO().WantCaptureKeyboard)
        return false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);

    Camera &cam = ctx.cam;
    float amount = cam_move_speed * dt;
    bool new_render = false;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cam.pos += cam.dir * amount;
        new_render = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cam.pos -= cam.dir * amount;
        new_render = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cam.pos += cross(normalize(cam.dir), normalize(cam.up)) * amount;
        new_render = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cam.pos -= cross(normalize(cam.dir), normalize(cam.up)) * amount;
        new_render = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        cam.pos += normalize(cam.up) * amount;
        new_render = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        cam.pos -= normalize(cam.up) * amount;
        new_render = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        fprintf(stdout, "Embree memory: %.1f MB\n", num_bytes_embree / 1000000.f);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        new_render = true;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        json11::Json::object json{{ "camera", cam }};
        fprintf(stdout, "\"Camera\": %s\n", json["camera"].dump().c_str());
    }
    return new_render;
}

// returns if a new rendering should be started
bool mouse_handler(GLFWwindow *window, Context& ctx, float dt) {
    if (ImGui::GetIO().WantCaptureMouse)
        return false;

    static bool init = false;
    static double last_x = 0, last_y = 0;
    static float speed = .1f;

    // fetch and init mouse pos
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (!init) {
        last_x = xpos;
        last_y = ypos;
        init = true;
    }

    Camera& cam = ctx.cam;
    uint32_t w = ctx.fbo.width(), h = ctx.fbo.height();
    bool new_render = false;

    // only move cam when mouse button pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float pitch = -speed * (ypos - last_y);
        float yaw = -speed * (xpos - last_x);
        // ignore very small movements
        if (fabsf(pitch) + fabsf(yaw) > .01f) {
            // compute new dir
            const glm::mat4 rot = glm::rotate(yaw * float(M_PI) / 180.f, cam.up) *
                glm::rotate(pitch * float(M_PI) / 180.f, glm::normalize(cross(cam.dir, cam.up)));
            cam.dir = glm::vec3(rot * glm::vec4(cam.dir, 0));
            // fix up vector to avoid roll
            cam.up = glm::vec3(0, 1, 0);
            new_render = true;
        }
    }

    // set camera focal point with right click
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        Ray pick = cam.view_ray(xpos, h - 1 - ypos, w, h);
        const SurfaceHit hit = ctx.scene.intersect(pick);
        if (hit.valid) {
            cam.focal_depth = pick.tfar;
            new_render = true;
            std::cout << "new focal dist: " << pick.tfar << std::endl;
            std::cout << "mat name: " << hit.mat->name << std::endl;
            std::cout << "mat type: " << hit.mat->type << std::endl;
        }
    }

    last_x = xpos;
    last_y = ypos;
    return new_render;
}

// ---------------------------------------------------------------------------------
// Context

Context::Context(uint32_t w, uint32_t h, uint32_t sppx)
    : device(rtcNewDevice(0)), fbo(w, h, sppx), scene(device), cam(), algorithm(), window(0), quad(0) {
    // check embree device on errors
    RTCError embree_error = rtcGetDeviceError(device);
    if (embree_error != RTC_ERROR_NONE) {
        printf("Embree setup failed!\n");
        embreeErrorFunc(0, embree_error, "Setup");
        exit(1);
    }

    // set embree error and memory callback
    rtcSetDeviceErrorFunction(device, embreeErrorFunc, 0);
    rtcSetDeviceMemoryMonitorFunction(device, embreeMemFunc, 0);

    // try to init GLFW
    if (!glfwInit()) {
        printf("No OpenGL context -> rendering offline.\n");
        return;
    }
    glfwSetErrorCallback(glfwErrorFunc);
    // Use OpenGL 3.3 for compat
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, 0);
    window = glfwCreateWindow(w, h, "gi", NULL, NULL);
    if (!window) {
        printf("GLFWCreateWindow failed.\n");
        exit(1);
    }
    glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__
    // 30fps preview is more than enough
    glfwSwapInterval(2);
#endif
    // install drag and drop callback
    glfwSetWindowUserPointer(window, this);
    glfwSetDropCallback(window, glfwDropCallback);

    // init GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        glfwDestroyWindow(window);
        window = 0;
        printf("GLEWInit failed: %s\n", glewGetErrorString(err));
        exit(1);
    }

    // full screen quad for drawing
    quad = std::make_shared<Quad>(fbo);

    // init GUI
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

Context::~Context() {
    if (scene.scene) {
        rtcReleaseScene(scene.scene);
        scene.scene = 0;
    }
    if (device) {
        rtcReleaseDevice(device);
        device = 0;
    }
    if (window) {
        quad.reset();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void Context::load(const std::filesystem::path& path) {
    if (path.extension() == ".json") {
        // load json config file
        const std::filesystem::path resolved_path = std::filesystem::exists(path) ? path : std::filesystem::path(GI_CONF_DIR) / path;
        json11::Json cfg = read_json_config(resolved_path.string().c_str());
        from_json(cfg);
    } else if (path.extension() == ".hdr" || path.extension() == ".png" || path.extension() == ".jpg") {
        // load environment map
        scene.load_sky(path);
    } else {
        // try to load mesh file via assimp
        try {
            scene.load_mesh(path);
        } catch (std::runtime_error err) {
            fprintf(stderr, "WARN: Don't know how to load file: \"%s\"\n.", path.c_str());
        }
    }
    restart = true;
}

void Context::resize(uint32_t w, uint32_t h, uint32_t sppx) {
    fbo.resize(w, h, sppx);
    if (window) {
        glfwSetWindowSize(window, w, h);
        quad->resize(fbo);
    }
}

void Context::render_join() {
    abort = true;
    if (worker.joinable()) worker.join();
    restart = true;
}

void Context::run() {
    if (!window) {
        // no GL context, render offline in main thread
        render(*this);
        return;
    }

    // render in seperate thread
    worker = std::thread(render, std::ref(*this));
    restart = false;

    // run viewer
    float time = glfwGetTime();

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_BEGIN {
#else
    while (!glfwWindowShouldClose(window)) {
#endif
        // ---------------------------------
        // process input

        float now = glfwGetTime();
        float dt = now - time;
        time = now;

        glfwPollEvents();
        if (keyboard_handler(window, *this, dt) || mouse_handler(window, *this, dt)) {
            restart = true;
        }

        // ---------------------------------
        // render live preview
 
        quad->draw(fbo, fbo.PREVIEW_EXPOSURE);

        // ---------------------------------
        // render UI

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        cam_move_speed = fmaxf(0.01f, cam_move_speed + 0.25f * ImGui::GetIO().MouseWheel);

        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("Camera")) {
                if (ImGui::DragFloat3("pos", &cam.pos.x, .001f)) {
                    restart = true;
                }
                if (ImGui::DragFloat3("dir", &cam.dir.x, .001f)) {
                    cam.dir = normalize(cam.dir);
                    restart = true;
                }
                if (ImGui::DragFloat3("up", &cam.up.x, .001f)) {
                    cam.up = normalize(cam.up);
                    restart = true;
                }
                if (ImGui::DragFloat("lens radius", &cam.lens_radius, 0.001f, 0.f, .5f)) {
                    restart = true;
                }
                if (ImGui::DragFloat("focal depth", &cam.focal_depth, 0.01f, 0.f, 2 * scene.radius)) {
                    restart = true;
                }
                if (ImGui::Checkbox("Auto focal depth", &AUTO_FOCUS)) {
                    restart = true;
                }
                if (ImGui::Checkbox("Perspective", &cam.perspective)) {
                    restart = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Scene")) {
                if (!scene.meshes.empty()) {
                    ImGui::Text("bb_min: (%.2f, %.2f, %.2f)", scene.bb_min.x, scene.bb_min.y, scene.bb_min.z);
                    ImGui::Text("bb_max: (%.2f, %.2f, %.2f)", scene.bb_max.x, scene.bb_max.y, scene.bb_max.z);
                    ImGui::Text("radius: %.2f", scene.radius);
                    ImGui::Text("total light power: %.2f", scene.total_light_source_power());
                    ImGui::Separator();
                }

                if (!scene.meshes.empty() && ImGui::BeginMenu("Lights")) {
                    // iterate over all lights
                    for (uint32_t i = 0; i < scene.lights.size(); ++i) {
                        if (ImGui::BeginMenu((std::string("Light #") + std::to_string(i)).c_str())) {
                            // check on light type
                            if (dynamic_cast<AreaLight*>(scene.lights[i].get())) {
                                ImGui::Text("AreaLight");
                                AreaLight* l = static_cast<AreaLight*>(scene.lights[i].get());
                                ImGui::Text("Material name: %s", l->mesh.mat->name.c_str());
                                if (ImGui::ColorEdit3("color", &l->mesh.mat->albedo_col.x)) {
                                    restart = true;
                                }
                                if (ImGui::DragFloat("power", &l->mesh.mat->emissive_strength, 0.1f, 0.1f, FLT_MAX)) {
                                    restart = true;
                                }
                                if (ImGui::Button("Extinguish")) {
                                    l->mesh.mat->emissive_strength = 0.f;
                                    restart = true;
                                }
                            }
                            if (dynamic_cast<SkyLight*>(scene.lights[i].get())) {
                                ImGui::Text("SkyLight");
                                SkyLight* l = static_cast<SkyLight*>(scene.lights[i].get());
                                ImGui::Text("Texture: %s", l->texture->path().c_str());
                                if (ImGui::DragFloat("intensity", &l->intensity, 0.1f, 0.1f, FLT_MAX)) {
                                    restart = true;
                                }
                                if (ImGui::Button("Extinguish")) {
                                    l->intensity = 0.f;
                                    restart = true;
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenu();
                }

                if (!scene.materials.empty() && ImGui::BeginMenu("Materials")) {
                    for (auto& mat_ptr : Material::instances) {
                        if (ImGui::BeginMenu(mat_ptr->name.c_str())) {
                            if (!mat_ptr->albedo_tex) {
                                if (ImGui::ColorEdit3("albedo", &mat_ptr->albedo_col.x)) {
                                    restart = true;
                                }
                            } else {
                                ImGui::Text("albedo map: %s", mat_ptr->albedo_tex.src_path.c_str());
                            }

                            if (mat_ptr->normal_tex) {
                                ImGui::Text("normal map: %s", mat_ptr->normal_tex.src_path.c_str());
                            }
                            if (mat_ptr->alpha_tex) {
                                ImGui::Text("alpha map: %s", mat_ptr->alpha_tex.src_path.c_str());
                            }
                            if (mat_ptr->roughness_tex) {
                                ImGui::Text("roughness map: %s", mat_ptr->roughness_tex.src_path.c_str());
                            }
                            if (mat_ptr->emissive_tex) {
                                ImGui::Text("emissive map: %s", mat_ptr->emissive_tex.src_path.c_str());
                            }
                            if (ImGui::SliderFloat("ior", &mat_ptr->ior, 1.f, 3.f)) {
                                restart = true;
                            }
                            if (ImGui::SliderFloat("absorb", &mat_ptr->absorb, 0.f, 3.f)) {
                                restart = true;
                            }
                            if (ImGui::SliderFloat("roughness", &mat_ptr->roughness_val, 0.001f, 1.f, "%.3f")) {
                                restart = true;
                            }
                            ImGui::Separator();
                            if (ImGui::DragFloat("emissive strength", &mat_ptr->emissive_strength, 1.f)) {
                                restart = true;
                            }

                            ImGui::Separator();
                            ImGui::Text("Set BRDF:");
                            if (ImGui::Button("LambertianReflection")) {
                                render_join();
                                mat_ptr->brdf.reset(new LambertianReflection);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("LambertianTransmission")) {
                                render_join();
                                mat_ptr->brdf.reset(new LambertianTransmission);
                            }
                            if (ImGui::Button("SpecularReflection")) {
                                render_join();
                                mat_ptr->brdf.reset(new SpecularReflection);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("SpecularTransmission")) {
                                render_join();
                                mat_ptr->brdf.reset(new SpecularTransmission);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("SpecularFresnel")) {
                                render_join();
                                mat_ptr->brdf.reset(new SpecularFresnel);
                            }
                            if (ImGui::Button("SpecularPhong")) {
                                render_join();
                                mat_ptr->brdf.reset(new SpecularPhong);
                            }
                            if (ImGui::Button("MicrofacetReflection")) {
                                render_join();
                                mat_ptr->brdf.reset(new MicrofacetReflection);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("MicrofacetTransmission")) {
                                render_join();
                                mat_ptr->brdf.reset(new MicrofacetTransmission);
                            }
                            if (ImGui::Button("LayeredMicrofacet")) {
                                render_join();
                                mat_ptr->brdf.reset(new LayeredSurface);
                            }
                            if (ImGui::Button("MetallicSurface")) {
                                render_join();
                                mat_ptr->brdf.reset(new MetallicSurface);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("GlassSurface")) {
                                render_join();
                                mat_ptr->brdf.reset(new GlassSurface);
                            }
                            ImGui::Separator();
                            // BRDF selection
                            ImGui::Text("Material presets:");
                            if (ImGui::Button("Diffuse")) {
                                render_join();
                                mat_ptr->set_diffuse();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Translucent")) {
                                render_join();
                                mat_ptr->set_translucent();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Specular")) {
                                render_join();
                                mat_ptr->set_specular();
                            }

                            if (ImGui::Button("Phong")) {
                                render_join();
                                mat_ptr->set_phong();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Microfacet")) {
                                render_join();
                                mat_ptr->set_microfacet();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Layered GGX")) {
                                render_join();
                                mat_ptr->set_layered_ggx();
                            }

                            if (ImGui::Button("Glass")) {
                                render_join();
                                mat_ptr->set_glass();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Water")) {
                                render_join();
                                mat_ptr->set_water();
                            }

                            if (ImGui::Button("Metal")) {
                                render_join();
                                mat_ptr->set_metal();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Gold")) {
                                render_join();
                                mat_ptr->set_gold();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Silver")) {
                                render_join();
                                mat_ptr->set_silver();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Copper")) {
                                render_join();
                                mat_ptr->set_copper();
                            }

                            if (ImGui::Button("Default")) {
                                render_join();
                                mat_ptr->set_default();
                            }
                            // end BRDF menu
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenu();
                }

                if (scene.volume && ImGui::BeginMenu("Volume")) {
                    ImGui::Text("NVDB grid: %s", scene.volume_path.c_str());
                    if (ImGui::DragFloat("density scale", &scene.volume->grid.density_scale, 0.001f, 0.001f, 1000.f)) {
                        restart = true;
                    }
                    if (ImGui::DragFloat("absorption cross section", &scene.volume->absorption_cross_section, 0.0001f, 0.0001f, 100.f)) {
                        restart = true;
                    }
                    if (ImGui::DragFloat("scattering cross section", &scene.volume->scattering_cross_section, 0.0001f, 0.0001f, 100.f)) {
                        restart = true;
                    }
                    if (ImGui::SliderFloat("henyey greenstein phase g", &scene.volume->phase_g, -0.99f, 0.99f)) {
                        restart = true;
                    }
                    if (ImGui::Checkbox("Unbiased estimators", &scene.volume->unbiased_estimators)) {
                        restart = true;
                    }
                    if (ImGui::DragFloat("raymarch step size", &scene.volume->raymarch_dt, 0.001f, 0.001f, 100.f)) {
                        restart = true;
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                static ImGui::FileBrowser mesh_browser(ImGuiFileBrowserFlags_CloseOnEsc);
                if (ImGui::Button("Add mesh")) {
                    mesh_browser.SetTitle("Select Mesh");
                    mesh_browser.Open();
                }
                mesh_browser.Display();
                if (mesh_browser.HasSelected()) {
                    render_join();
                    scene.load_mesh(mesh_browser.GetSelected().string());
                    mesh_browser.ClearSelected();
                }
                static ImGui::FileBrowser sky_browser(ImGuiFileBrowserFlags_CloseOnEsc);
                if (ImGui::Button("Add envmap")) {
                    sky_browser.SetTitle("Select envmap");
                    sky_browser.Open();
                }
                sky_browser.Display();
                if (sky_browser.HasSelected()) {
                    render_join();
                    scene.load_sky(sky_browser.GetSelected().string());
                    sky_browser.ClearSelected();
                }
                static ImGui::FileBrowser volume_browser(ImGuiFileBrowserFlags_CloseOnEsc);
                if (ImGui::Button("Add volume")) {
                    volume_browser.SetTitle("Select volume");
                    volume_browser.Open();
                }
                volume_browser.Display();
                if (volume_browser.HasSelected()) {
                    render_join();
                    scene.load_volume(volume_browser.GetSelected().string());
                    volume_browser.ClearSelected();
                }

                if (ImGui::Button("CLEAR")) {
                    render_join();
                    scene.clear();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Algorithms")) {
                ImGui::Text("Use rendering algorithm:");
                for (const auto& [name, ptr] : Algorithm::algorithms) {
                    if (ptr && ImGui::Button(name.c_str())) {
                        render_join();
                        algorithm = name;
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Preview")) {
                ImGui::DragFloat("Preview exposure", &fbo.PREVIEW_EXPOSURE, 0.01f, 0.01f, 100.f);
                ImGui::Separator();
                ImGui::Text("Display");
                ImGui::Indent();
                if (ImGui::Button("Image ")) {
                    fbo.tonemap();
                }
                ImGui::SameLine();
                if (ImGui::Button("Convergence ")) {
                    fbo.show_convergence();
                }
                ImGui::SameLine();
                if (ImGui::Button("#Samples ")) {
                    fbo.show_num_samples();
                }
                ImGui::Unindent();
                ImGui::Separator();
                if (ImGui::Button("Restart rendering")) {
                    restart = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Abort rendering")) {
                    abort = true;
                }
#ifdef WITH_OIDN
                ImGui::Separator();
                if (ImGui::Button("Denoise (OIDN)")) {
                    fbo.denoise();
                }
#endif
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Input/Output")) {
                static char filename[256] = { "output.png" };
                if (ImGui::Button("Save rendering")) {
                    fbo.save(filename);
                }
                ImGui::SameLine();
                ImGui::InputText("filename", filename, 256);

                ImGui::Separator();
                static char out_filename[256] = { "cfg.json" };
                if (ImGui::Button("Save config")) {
                    write_json_config(out_filename, to_json());
                }
                ImGui::SameLine();
                ImGui::InputText("out_filename", out_filename, 256);

                static ImGui::FileBrowser IO_browser(ImGuiFileBrowserFlags_CloseOnEsc);
                if (ImGui::Button("Load config")) {
                    IO_browser.SetDirectory("configs");
                    IO_browser.SetTitle("Select config file");
                    IO_browser.Open();
                }
                IO_browser.Display();
                if (IO_browser.HasSelected()) {
                    render_join();
                    from_json(read_json_config(IO_browser.GetSelected().string().c_str()));
                    IO_browser.ClearSelected();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::DragFloat("Exposure", &fbo.EXPOSURE, 0.01f, 0.01f, 100.f))
                    fbo.tonemap();
                ImGui::Separator();
                if (ImGui::Checkbox("Beauty render?", &BEAUTY_RENDER))
                    restart = true;
                if (ImGui::DragFloat("Error", &ERROR_EPS, 0.0001f, 0.001f, 0.5f))
                    restart = true;

                ImGui::Separator();

                bool should_resize = false;
                uint32_t w = fbo.width(), h = fbo.height(), sppx = fbo.samples();
                if (ImGui::InputInt("width", (int *)&w, 1, 100))
                    should_resize |= fbo.width() != w;
                if (ImGui::InputInt("height", (int *)&h, 1, 100))
                    should_resize |= fbo.height() != h;
                if (ImGui::InputInt("sppx", (int *)&sppx, 1, 10))
                    should_resize |= fbo.samples() != sppx;
                if (should_resize) {
                    render_join();
                    resize(std::max(256u, w), std::max(256u, h), std::max(1u, sppx));
                }

                ImGui::Separator();

                if (ImGui::SliderInt("RR min path length", (int *)&RR_MIN_PATH_LENGTH, 0, 10))
                    restart = true;
                if (ImGui::SliderFloat("RR threshold", &RR_THRESHOLD, 0.f, 1.f))
                    restart = true;
                if (ImGui::SliderInt("Max CAM path length", (int *)&MAX_CAM_PATH_LENGTH, 1, 128))
                    restart = true;
                if (ImGui::SliderInt("Max LIGHT path length", (int *)&MAX_LIGHT_PATH_LENGTH, 1, 32))
                    restart = true;

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // ---------------------------------
        // present stuff on screen

        glDisable(GL_DEPTH_TEST);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glEnable(GL_DEPTH_TEST);
        glfwSwapBuffers(window);

        // ---------------------------------
        // restart rendering?

        if (restart) {
            render_join();
            abort = false;

            fbo.clear();
            worker = std::thread(&render, std::ref(*this));

            restart = false;
        }
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // stop rendering and exit
    render_join();
}

float Context::filter_focal_distance() {
    float logAccum = 0;
    #pragma omp parallel for reduction(+ : logAccum)
    for (uint32_t y = 0; y < fbo.height() / 4; ++y) {
    //for (int32_t y = 0; y < fbo.height() / 4; ++y) {
        for (uint32_t x = 0; x < fbo.width() / 4; ++x) {
        //for (int32_t x = 0; x < fbo.width() / 4; ++x) {
            uint32_t xs = fbo.width() / 2 - fbo.width() / 8, ys = fbo.height() / 2 - fbo.height() / 8;
            Ray ray = cam.view_ray(xs + x, ys + y, fbo.width(), fbo.height());
            const SurfaceHit hit = scene.intersect(ray);
            const VolumeHit hit_vol = scene.intersect_volume(ray);
            if (hit.valid || hit_vol.valid) {
                const float f = fmaxf(1e-4f, logf(ray.tfar));
                if (!std::isnan(f) && !std::isinf(f))
                    logAccum += f;
            }
        }
    }
    return expf(logAccum / (float)(fbo.width() * fbo.height() / 16));
}

json11::Json Context::to_json() const {
    return json11::Json::object {
        { "algorithm", algorithm },
        { "framebuffer", fbo },
        { "scene", scene },
        { "camera", cam },
        { "auto_focus", AUTO_FOCUS },
        { "max_cam_path_length", int(MAX_CAM_PATH_LENGTH) },
        { "max_light_path_length", int(MAX_LIGHT_PATH_LENGTH) },
        { "rr_min_path_length", int(RR_MIN_PATH_LENGTH) },
        { "rr_threshold", RR_THRESHOLD },
        { "beauty_render", BEAUTY_RENDER },
        { "error_eps", ERROR_EPS }
    };
}

void Context::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        // parse settings
        json_set_bool(cfg, "auto_focus", AUTO_FOCUS);
        json_set_uint(cfg, "max_cam_path_length", MAX_CAM_PATH_LENGTH);
        json_set_uint(cfg, "max_light_path_length", MAX_LIGHT_PATH_LENGTH);
        json_set_uint(cfg, "rr_min_path_length", RR_MIN_PATH_LENGTH);
        json_set_float(cfg, "rr_threshold", RR_THRESHOLD);
        json_set_bool(cfg, "beauty_render", BEAUTY_RENDER);
        json_set_float(cfg, "error_eps", ERROR_EPS);
        // parse algorithm, fbo, scene and cam
        if (cfg["algorithm"].is_string()) {
            algorithm = cfg["algorithm"].string_value();
            auto algo = Algorithm::algorithms[algorithm];
            if (algo) algo->read_config(cfg);
        }
        if (cfg["framebuffer"].is_object())
            fbo.from_json(cfg["framebuffer"]);
        if (cfg["scene"].is_object())
            scene.from_json(cfg["scene"]);
        if (cfg["camera"].is_object())
            cam.from_json(cfg["camera"]);
        // update preview window if available
        if (window) {
            glfwSetWindowSize(window, fbo.width(), fbo.height());
            quad->resize(fbo);
        }
    }
}
