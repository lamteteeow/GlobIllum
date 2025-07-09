#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "texture.h"
#include "color.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

// -------------------------------------------
// Texture

Texture::Texture() : w(0), h(0), has_alpha(false) {}

Texture::Texture(const std::filesystem::path& path, bool sRGB) : Texture() {
    load(path, sRGB);
}

Texture::Texture(size_t w, size_t h, const glm::vec3* data) : Texture() {
    load(w, h, data);
}

Texture::Texture(const glm::vec3& col) : Texture() {
    load(col);
}

void Texture::load(const std::filesystem::path& path, bool sRGB) {
    src_path = path;
    // load image from disk
    int chan_file, chan_forced = 3; // number of color channels
	bool HDR = stbi_is_hdr(path.string().c_str());
    if (HDR) {
        float* img_data = stbi_loadf(path.string().c_str(), (int*)&w, (int*)&h, &chan_file, chan_forced);
        if (!img_data)
            throw std::runtime_error("Failed to load texture: " + path.string());
        texels.resize(w * h);
        for (size_t i = 0; i < w * h; ++i) {
            const glm::vec3 col = glm::vec3(img_data[i*3+0], img_data[i*3+1], img_data[i*3+2]);
            texels[i] = col; // always in linear color space
        }
        stbi_image_free(img_data);
    } else {
        uint8_t* img_data = stbi_load(path.string().c_str(), (int*)&w, (int*)&h, &chan_file, chan_forced);
        if (!img_data)
            throw std::runtime_error("Failed to load texture: " + path.string());
        texels.resize(w * h);
        for (size_t i = 0; i < w * h; ++i) {
            const glm::vec3 col = glm::vec3(img_data[i*3+0], img_data[i*3+1], img_data[i*3+2]) / 255.f;
            texels[i] = sRGB ? srgb_to_rgb(col) : col;
        }
        stbi_image_free(img_data);
    }
    has_alpha = chan_file == 4;
}

void Texture::load_alpha(const std::filesystem::path& path) {
    src_path = path;
    // load image from disk
    int chan_file, chan_forced = 4; // number of color channels
    bool HDR = stbi_is_hdr(path.string().c_str());
    if (HDR) {
        float* img_data = stbi_loadf(path.string().c_str(), (int*)&w, (int*)&h, &chan_file, chan_forced);
        if (!img_data)
            throw std::runtime_error("Failed to load texture: " + path.string());
        texels.resize(w * h);
        for (size_t i = 0; i < w * h; ++i) {
            const glm::vec3 col = glm::vec3(img_data[i*4+3]);
            texels[i] = col; // always in linear color space
        }
        stbi_image_free(img_data);
    } else {
        uint8_t* img_data = stbi_load(path.string().c_str(), (int*)&w, (int*)&h, &chan_file, chan_forced);
        if (!img_data)
            throw std::runtime_error("Failed to load texture: " + path.string());
        texels.resize(w * h);
        for (size_t i = 0; i < w * h; ++i) {
            const glm::vec3 col = glm::vec3(img_data[i*4+3]) / 255.f;
            texels[i] = col; // assume alpha to always be in linear space
        }
        stbi_image_free(img_data);
    }
    has_alpha = chan_file == 4;
}

void Texture::load(size_t w, size_t h, const glm::vec3* data) {
    src_path.clear();
    this->w = w;
    this->h = h;
    texels.resize(w * h);
    for (size_t i = 0; i < w * h; ++i)
        texels[i] = data[i];
}

void Texture::load(const glm::vec3& col) {
    src_path.clear();
    w = 1;
    h = 1;
    texels.resize(1);
    texels[0] = col;
}

void Texture::save_png(const std::filesystem::path& path) const {
    Texture::save_png(path, w, h, data());
}

void Texture::save_jpg(const std::filesystem::path& path) const {
    Texture::save_jpg(path, w, h, data());
}

void Texture::save_png(const std::filesystem::path& path, size_t w, size_t h, const glm::vec3* rgb, bool flip) {
    stbi_flip_vertically_on_write(flip ? 1 : 0);
    std::vector<uint8_t> pixels(w * h * 3u);
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
            for (size_t c = 0; c < 3; ++c)
                pixels[(y * w + x)*3 + c] = glm::clamp(int(round(rgb_to_srgb(rgb[y * w + x][c]) * 255)), 0, 255);
    stbi_write_png(path.string().c_str(), w, h, 3, pixels.data(), sizeof(uint8_t) * w * 3);
    printf("%s written.\n", path.string().c_str());
}

void Texture::save_jpg(const std::filesystem::path& path, size_t w, size_t h, const glm::vec3* rgb, bool flip) {
    stbi_flip_vertically_on_write(flip ? 1 : 0);
    std::vector<uint8_t> pixels(w * h * 3u);
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
            for (size_t c = 0; c < 3; ++c)
                pixels[(y * w + x)*3 + c] = glm::clamp(int(round(rgb_to_srgb(rgb[y * w + x][c]) * 255)), 0, 255);
    stbi_write_jpg(path.string().c_str(), w, h, 3, pixels.data(), 100);
    printf("%s written.\n", path.string().c_str());
}
