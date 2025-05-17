#pragma once

#include <memory>
#include <filesystem>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <embree4/rtcore.h>

#include "quad.h"
#include "gi/scene.h"
#include "gi/camera.h"
#include "gi/framebuffer.h"
#include "gi/json11.h"

class Context {
public:
    /**
     * @brief Construct with given resolution
     *
     * @param w Rendering width
     * @param h Rendering height
     * @param sppx Samples per pixel
     */
    Context(uint32_t w = 1280, uint32_t h = 720, uint32_t sppx = 10);

    /**
     * @brief Destructor
     */
    ~Context();

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    /**
     * @brief Enter the main rendering loop
     */
    void run();

    /**
     * @brief Load given file from disk, with handling based on file extension
     *
     * @param path Path to file on disk
     */
    void load(const std::filesystem::path& path);

    /**
     * @brief Resize the rendering (FBO and preview window also)
     *
     * @param w New rendering width
     * @param h New rendering height
     * @param sppx New samples per pixel
     */
    void resize(uint32_t w, uint32_t h, uint32_t sppx);

    /**
     * @brief Compute auto focus focal depth using a geometric average
     *
     * @return Filtered focal distance
     */
    float filter_focal_distance();

    /**
     * @brief Export current state to JSON
     *
     * @return Json holding the current state
     */
    json11::Json to_json() const;

    /**
     * @brief Import state from JSON
     *
     * @param cfg JSON config to fetch state from
     */
    void from_json(const json11::Json& cfg);

public:
    // settings
    bool AUTO_FOCUS = true;             ///< Apply auto focus?
    uint32_t MAX_CAM_PATH_LENGTH = 10;  ///< Maximum camera path length
    uint32_t MAX_LIGHT_PATH_LENGTH = 5; ///< Maximum light path length
    uint32_t RR_MIN_PATH_LENGTH = 1;    ///< Apply russian roulette after how many bounces?
    float RR_THRESHOLD = 0.25;          ///< Apply russian roulette if luma drops below this
    bool BEAUTY_RENDER = false;         ///< Render until converged and denoise if available?
    float ERROR_EPS = 0.05;             ///< Convergence criterion

    // data
    RTCDevice device;                   ///< Embree4 device
    Framebuffer fbo;                    ///< Framebuffer used for rendering
    Scene scene;                        ///< Scene used for rendering
    Camera cam;                         ///< Camera used for rendering
    std::string algorithm;              ///< Algorithm to use for rendering
    volatile bool abort = false;        ///< Flag to abort rendering if true
    volatile bool restart = false;      ///< Flag to restart rendering if true

private:
    // GL viewer stuff
    GLFWwindow *window;                 ///< OpenGL preview window
    std::shared_ptr<Quad> quad;         ///< Fullscreen quad to render preview

private:
    void render_join();
    std::thread worker;
};
