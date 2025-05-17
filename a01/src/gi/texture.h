#pragma once

#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include "timer.h"

class Texture {
public:
    // construct invalid texture
    Texture();
    // construct from file on disk
    Texture(const std::filesystem::path& path, bool sRGB = true);
    // construct from given texel data
    Texture(size_t w, size_t h, const glm::vec3* data);
    // construct as 1x1 texture from given color
    Texture(const glm::vec3& col);

    // load texture from disk (rgb only)
    void load(const std::filesystem::path& path, bool sRGB = true);
    // load alpha channel of texture from disk
    void load_alpha(const std::filesystem::path& path);
    // load texture from given texel data
    void load(size_t w, size_t h, const glm::vec3* data);
    // load 1x1 texture with given color
    void load(const glm::vec3& col);

    // texture lookups
    inline glm::vec3 fetch(const glm::uvec2& xy) const;
    inline glm::vec3 bilin(const glm::vec2& uv) const;
    inline glm::vec3 env(const glm::vec3& dir) const;

    // check if texture is valid and it is save to perform texture lookups
    inline explicit operator bool() const { return w != 0 && h != 0; }

    // operators for more convenient texture lookups
    inline glm::vec3 operator()(const glm::uvec2& xy) const { return fetch(xy); }
    inline glm::vec3 operator()(int x, int y) const { return fetch(glm::uvec2(x, y)); }
    inline glm::vec3 operator()(uint32_t x, uint32_t y) const { return fetch(glm::uvec2(x, y)); }
    inline glm::vec3 operator()(uint64_t x, uint64_t y) const { return fetch(glm::uvec2(x, y)); }
    inline glm::vec3 operator()(const glm::vec2& uv) const { return bilin(uv); }
    inline glm::vec3 operator()(float u, float v) const { return bilin(glm::vec2(u, v)); }

    // "accessors"
    inline size_t width() const { return w; }
    inline size_t height() const { return h; }
    inline glm::uvec2 dim() const { return glm::uvec2(w, h); }
    inline const glm::vec3* data() const { return texels.data(); }
    inline std::filesystem::path path() const { return src_path; }

    // save as PNG / JPG
    void save_png(const std::filesystem::path& path) const;
    void save_jpg(const std::filesystem::path& path) const;

    // generalization for writing rgb data to disk as png or jpg
    static void save_png(const std::filesystem::path& path, size_t w, size_t h, const glm::vec3* rgb, bool flip = true);
    static void save_jpg(const std::filesystem::path& path, size_t w, size_t h, const glm::vec3* rgb, bool flip = true);

    // data
    size_t w;                       ///< Texture width
    size_t h;                       ///< Texture height
    std::vector<glm::vec3> texels;  ///< Raw texel data
    std::filesystem::path src_path; ///< Filename, if loaded from disk
    bool has_alpha;                 ///< Image from disk had alpha channel (which was discarded on loading)
};

// --------------------------------------
// inline implementations

glm::vec3 Texture::fetch(const glm::uvec2& xy) const {
    STAT("Texture lookup");
    return texels[(xy.y % h) * w + (xy.x % w)];
}

glm::vec3 Texture::bilin(const glm::vec2& uv) const {
    assert(std::isfinite(uv.x) && std::isfinite(uv.y));
    STAT("Texture lookup");
    const glm::vec2 uv_map = glm::fract(uv);
    const glm::vec2 xy = uv_map * glm::vec2(w, h);
    const glm::vec3 bl = fetch(glm::uvec2(xy) + glm::uvec2(0, 0));
    const glm::vec3 br = fetch(glm::uvec2(xy) + glm::uvec2(1, 0));
    const glm::vec3 tl = fetch(glm::uvec2(xy) + glm::uvec2(0, 1));
    const glm::vec3 tr = fetch(glm::uvec2(xy) + glm::uvec2(1, 1));
    const glm::vec2 f = glm::fract(xy);
    return glm::mix(glm::mix(bl, br, f.x), glm::mix(tl, tr, f.x), f.y);
}

glm::vec3 Texture::env(const glm::vec3& dir) const {
    STAT("Texture lookup");
    const float u = atan2f(dir.z, dir.x) / (2.f * M_PI);
    const float v = acosf(dir.y) / M_PI;
    assert(std::isfinite(u));
    assert(std::isfinite(v));
    return bilin(glm::vec2(u, v));
}
