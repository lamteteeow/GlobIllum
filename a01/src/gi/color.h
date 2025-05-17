#pragma once

#include <glm/glm.hpp>

// ---------------------------------------------
// luminosity

inline float luma(const glm::vec3& rgb) {
    return glm::dot(glm::vec3(0.212671f, 0.715160f, 0.072169f), rgb);
}

// ---------------------------------------------
// color space transformations

// linear RGB color space to CIE XYZ
inline glm::vec3 rgb_to_xyz(const glm::vec3& rgb) {
    const glm::mat3 transform(glm::vec3(0.4124, 0.2126, 0.0193), glm::vec3(0.3576, 0.7152, 0.1192), glm::vec3(0.1805, 0.0722, 0.9505));
    return transform * rgb;
}

// CIE XYZ to linear RGB color space
inline glm::vec3 xyz_to_rgb(const glm::vec3& xyz) {
    const glm::mat3 transform(glm::vec3(3.2406, -0.9689,  0.0557), glm::vec3(-1.5372, 1.8758, -0.2040), glm::vec3(-0.4986, 0.0415, 1.0570));
    return transform * xyz;
}

// linear RGB color space to sRGB
inline float rgb_to_srgb(float val) {
    if (val <= 0.0031308f) return 12.92f * val;
    return 1.055f * powf(val, 1.f / 2.4f) - 0.055f;
}
inline glm::vec3 rgb_to_srgb(const glm::vec3& rgb) {
    return glm::vec3(rgb_to_srgb(rgb.x), rgb_to_srgb(rgb.y), rgb_to_srgb(rgb.z));
}

// sRGB to linear RGB color space 
inline float srgb_to_rgb(float val) {
    if (val <= 0.04045) return val / 12.92f;
    return powf((val + 0.055) / 1.055f, 2.4f);
}
inline glm::vec3 srgb_to_rgb(const glm::vec3& srgb) {
    return glm::vec3(srgb_to_rgb(srgb.x), srgb_to_rgb(srgb.y), srgb_to_rgb(srgb.z));
}

// CIE XYZ to sRGB color space 
inline glm::vec3 xyz_to_srgb(const glm::vec3& xyz) {
    return rgb_to_srgb(xyz_to_rgb(xyz));
}

// sRGB to CIE XYZ color space 
inline glm::vec3 srgb_to_xyz(const glm::vec3& srgb) {
    return rgb_to_xyz(srgb_to_rgb(srgb));
}

// ---------------------------------------------
// tonemapping

inline glm::vec3 saturate(const glm::vec3& col) { return clamp(col, glm::vec3(0), glm::vec3(1)); }

inline glm::vec3 reinhardTonemap(const glm::vec3& rgb, float exposure, float alpha) {
    const float Y = luma(rgb);
    const float L = (alpha / exposure) * Y;
    const float Ld = L / (L + 1);
    return rgb * Ld / Y;
}

inline glm::vec3 hable(const glm::vec3& rgb) {
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    return ((rgb * (A * rgb + C * B) + D * E) / (rgb * (A * rgb + B) + D * F)) - E / F;
}
inline glm::vec3 hableTonemap(const glm::vec3& rgb, float exposure = 1.f) {
    const float W = 11.2f;
    return hable(exposure * rgb) / hable(glm::vec3(W));
}

inline glm::vec3 ACESFilm(const glm::vec3& x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
	return (x * (a * x + b)) / (x * (c * x + d) + e);
}

inline glm::vec3 RRTAndODTFit(const glm::vec3& v) {
    const glm::vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    const glm::vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}
inline glm::vec3 ACESFitted(const glm::vec3& rgb) {
    // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
    static const glm::mat3 ACESInputMat(glm::vec3(0.59719, 0.35458, 0.04823), glm::vec3(0.07600, 0.90834, 0.01566), glm::vec3(0.02840, 0.13383, 0.83777));
    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    static const glm::mat3 ACESOutputMat(glm::vec3(1.60475, -0.53108, -0.07367), glm::vec3(-0.10208, 1.10813, -0.00605), glm::vec3(-0.00327, -0.07276, 1.07602));
    return saturate(ACESOutputMat * RRTAndODTFit(ACESInputMat * rgb));
}

// ---------------------------------------------
// utility heatmap (blue to red) from given value in [0, 1]

inline glm::vec3 heatmap(float val) {
    const float hue = 251.1 / 360.f; // blue
    const glm::vec3 hsv = glm::vec3(hue + glm::clamp(val, 0.f, 1.f) * -hue, 1, val < 1e-4f ? 0 : 1); // from blue to red
    // map hsv to rgb
    const glm::vec4 K = glm::vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    const glm::vec3 p = glm::abs(glm::fract(glm::vec3(hsv.x) + glm::vec3(K)) * 6.f - glm::vec3(K.w));
    return hsv.z * glm::mix(glm::vec3(K.x), clamp(p - glm::vec3(K.x), glm::vec3(0.f), glm::vec3(1.f)), hsv.y);
}
