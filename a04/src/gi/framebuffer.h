#pragma once

#include <string>
#include <filesystem>
#include <glm/glm.hpp>
#include "json11.h"
#include "buffer.h"
#ifdef WITH_OIDN
    #include <OpenImageDenoise/oidn.hpp>
#endif

// Framebuffer (FBO), providing a preview buffer and postprocessing operations
// (0, 0) is assumed to be bottom left and (w - 1, h - 1) the top right
class Framebuffer {
public:
    Framebuffer(size_t w, size_t h, size_t sppx);

    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // "accessors"
    inline size_t width() const { return w; }
    inline size_t height() const { return h; }
    inline size_t samples() const { return sppx; }
    inline const glm::vec3* data() const { return fbo.data(); }

    // add new sample at pixel (x, y) and update preview
    void add_sample(size_t x, size_t y, const glm::vec3& irradiance);

    void clear();
    void resize(size_t w, size_t h, size_t sppx);

    void show_convergence();
    void show_num_samples();

    // postprocessing
    void tonemap();
#ifdef WITH_OIDN
    void denoise();
#endif

    // compute geometric mean of luminance
    float geo_mean_luma() const;

    // output image to disk
    void save(const std::filesystem::path& path) const;

    // JSON import/export
    json11::Json to_json() const;
    void from_json(const json11::Json& cfg);

    // GUI (return true on change)
    bool render_GUI();

    // settings
    enum TonemappingOperator {
        NONE,
        // REINHARD,
        HABLE,
        ACESFILM,
        // ACESFITTED,
    };
    TonemappingOperator TONEMAPPER = HABLE; ///< Tonemapping operator to use
    float EXPOSURE = 3.f;           ///< Exposure to use for the tonemapper
    bool PREVIEW_CONV = false;      ///< Show updated convergence or preview in add_sample()

    // data
    size_t w;                       ///< FBO width
    size_t h;                       ///< FBO height
    size_t sppx;                    ///< Targeted num samples per pixel overall
    Buffer<glm::vec3> color;        ///< Color sample buffer (in CIE XYZ color space)
    Buffer<size_t> num_samples;     ///< Current #samples per pixel
    Buffer<glm::vec3> even;         ///< Color sample buffer for variance estimate (in CIE XYZ color space)
    Buffer<glm::vec3> fbo;          ///< Front buffer, to present on screen or save to disk (in linear RGB color space)
#ifdef WITH_OIDN
    oidn::DeviceRef device;         ///< OpenImageDenoise device
#endif
};
