#pragma once

#include <tuple>
#include <glm/glm.hpp>

// ------------------------------------------------
// utility functions
// vectors are all assumed to be normalized!

#define PI float(M_PI)
#define INVPI 1.f / PI
#define INV2PI 1.f / (2*PI)
#define INV4PI 1.f / (4*PI)

inline float sqr(float x) { return x*x; }

inline float deg_to_rad(float deg) { return deg * PI / 180.f; }
inline float rad_to_deg(float rad) { return rad * 180.f / PI; }

// orientation helper functions
inline bool same_hemisphere(const glm::vec3& N, const glm::vec3& v) {
    return glm::dot(N, v) > 0;
}
inline glm::vec3 faceforward(const glm::vec3& N, const glm::vec3& v) {
    return same_hemisphere(N, v) ? v : -v;
}

// trigonometric helper functions
inline float cos_theta(float cos_t) {
    return cos_t;
}
inline float cos2_theta(float cos_t) {
    return sqr(cos_t);
}
inline float abs_cos_theta(float cos_t) {
    return fabsf(cos_t);
}
inline float sin2_theta(float cos_t) {
    return fmaxf(0.f, 1.f - sqr(cos_t));
}
inline float sin_theta(float cos_t) {
    return sqrtf(sin2_theta(cos_t));
}
inline float tan_theta(float cos_t) {
    return sin_theta(cos_t) / cos_theta(cos_t);
}
inline float tan2_theta(float cos_t) {
    return sin2_theta(cos_t) / cos2_theta(cos_t);
}
inline float cos_theta(const glm::vec3& N, const glm::vec3& w) {
    return glm::dot(N, w);
}
inline float abs_cos_theta(const glm::vec3& N, const glm::vec3& w) {
    return fabsf(cos_theta(N, w));
}
inline float cos2_theta(const glm::vec3& N, const glm::vec3& w) {
    return sqr(cos_theta(N, w));
}
inline float sin2_theta(const glm::vec3& N, const glm::vec3& w) {
    return fmaxf(0.f, 1.f - sqr(cos_theta(N, w)));
}
inline float sin_theta(const glm::vec3& N, const glm::vec3& w) {
    return sqrtf(sin2_theta(N, w));
}
inline float tan_theta(const glm::vec3& N, const glm::vec3& w) {
    return sin_theta(N, w) / cos_theta(N, w);
}
inline float tan2_theta(const glm::vec3& N, const glm::vec3& w) {
    return sin2_theta(N, w) / cos2_theta(N, w);
}

// ------------------------------------------------
// primitive sampling routines

// disk (mapping from unit square to unit cirlce)
inline glm::vec2 uniform_sample_disk(const glm::vec2& sample) {
    const float r = sqrtf(sample.x);
    const float theta = 2 * PI * sample.y;
    return r * glm::vec2(cosf(theta), sinf(theta));
}
inline glm::vec2 concentric_sample_disk(const glm::vec2& sample) {
    const glm::vec2 mapped = glm::vec2(2) * sample - glm::vec2(1);
    if (mapped.x == 0.f && mapped.y == 0.f)
        return glm::vec2(0.f);
    float theta, r;
    if (fabsf(mapped.x) > fabsf(mapped.y)) {
        r = mapped.x;
        theta = (PI / 4.f) * (mapped.y / mapped.x);
    } else {
        r = mapped.y;
        theta = (PI / 2.f) - (PI / 4.f) * (mapped.x / mapped.y);
    }
    return r * glm::vec2(cosf(theta), sinf(theta));
}

// triangles (returns baryzentric coordinates)
inline glm::vec2 uniform_sample_triangle(const glm::vec2& sample) {
    const float su0 = sqrtf(sample.x);
    return glm::vec2(1 - su0, sample.y * su0);
}

// hemisphere (uniform distributed tangent space direction)
inline glm::vec3 uniform_sample_hemisphere(const glm::vec2& sample) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
inline float uniform_hemisphere_pdf() {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// hemisphere (cosine distributed tangent space direction)
inline glm::vec3 cosine_sample_hemisphere(const glm::vec2& sample) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
inline float cosine_hemisphere_pdf(float cos_t) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// sphere (uniform distributed tangent space direction)
inline glm::vec3 uniform_sample_sphere(const glm::vec2& sample) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
inline float uniform_sphere_pdf() {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// cone -> uniform distributed tangent space direction
inline glm::vec3 uniform_sample_cone(const glm::vec2& sample, float cos_tMax) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
inline float uniform_cone_pdf(float cos_tMax) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ------------------------------------------------
// refraction

/**
 * @brief Calculate (perfect) refraction direction
 *
 * @param I Incident vector
 * @param N Surface normal
 * @param ior Index of refraction
 * @param out_dir Refracted direction
 *
 * @return false in case of total internal reflection (out_dir = glm::vec3(0)), true otherwise
 */
inline bool refract(const glm::vec3& I, glm::vec3 N, const float ior, glm::vec3& out_dir) {
    float cos_t = dot(N, I);
    float eta;
    if (cos_t > 0.f) {
        eta = ior;
        N = -N;
        cos_t = -cos_t;
    } else
        eta = 1.f / ior;
    const float k = 1.f - sqr(eta) * (1.f - sqr(cos_t));
    if (k < 0.f) { // TIR
        out_dir = glm::vec3(0.f);
        return false;
    } else {
        out_dir = normalize(eta*I - (eta * cos_t + sqrtf(k)) * N);
        return true;
    }
}

inline std::tuple<bool, glm::vec3> refract(const glm::vec3& I, glm::vec3 N, const float ior) {
    float cos_t = dot(N, I);
    float eta;
    if (cos_t > 0.f) {
        eta = ior;
        N = -N;
        cos_t = -cos_t;
    } else
        eta = 1.f / ior;
    const float k = 1.f - sqr(eta) * (1.f - sqr(cos_t));
    if (k < 0.f) // TIR
        return { false, glm::vec3(0.f) };
    else
        return { true, normalize(eta*I - (eta * cos_t + sqrtf(k)) * N) };
}

// calculate (perfect) mirror direction around N
inline glm::vec3 reflect(const glm::vec3& I, const glm::vec3& N) { return glm::reflect(I, N); }
inline glm::vec3 mirror(const glm::vec3& I, const glm::vec3& N) { return -glm::reflect(I, N); }

// ------------------------------------------------
// MIS

/**
 * @brief Balance heuristic for combining two sampling strategies via MIS
 *
 * @param f pdf of current sampling strategy
 * @param g pdf of other sampling strategy
 *
 * @return MIS weight for current sample
 */
inline float balance_heuristic(float f, float g) {
    return f / (f + g);
}

/**
 * @brief Power heuristic (with exponent 2) for combining two sampling strategies via MIS
 *
 * @param f pdf of current sampling strategy
 * @param g pdf of other sampling strategy
 *
 * @return MIS weight for current sample
 */
inline float power_heuristic(float f, float g) {
    return sqr(f) / (sqr(f) + sqr(g));
}

// ------------------------------------------------
// Coordinate frame transformations

// build tangent frame around (normalized) N
inline std::tuple<glm::vec3, glm::vec3> build_tangent_frame(const glm::vec3& N) {
    const glm::vec3 T = fabsf(N.x) > fabsf(N.y) ?
        glm::vec3(-N.z, 0, N.x) / sqrtf(N.x * N.x + N.z * N.z) :
        glm::vec3(0, N.z, -N.y) / sqrtf(N.y * N.y + N.z * N.z);
    return { T, cross(N, T) };
}

// transform world space direction dir into tangent space
inline glm::vec3 world_to_tangent(const glm::vec3& N, const glm::vec3& T, const glm::vec3& B, const glm::vec3& dir) {
    return glm::vec3(dot(dir, T), dot(dir, B), dot(dir, N));
}

inline glm::vec3 world_to_tangent(const glm::vec3& N, const glm::vec3& dir) {
    const auto [T, B] = build_tangent_frame(N);
    return world_to_tangent(N, T, B, dir);
}

// transform tangent space direction dir into world space
inline glm::vec3 tangent_to_world(const glm::vec3& N, const glm::vec3& T, const glm::vec3& B, const glm::vec3& dir) {
    return normalize(dir.x * T + dir.y * B + dir.z * N);
}

inline glm::vec3 tangent_to_world(const glm::vec3& N, const glm::vec3& dir) {
    const auto [T, B] = build_tangent_frame(N);
    return tangent_to_world(N, T, B, dir);
}

// align vector v with given axis (e.g. to transform a tangent space sample along a world normal)
inline glm::vec3 align(const glm::vec3& axis, const glm::vec3& dir) {
    const float s = copysign(1.f, axis.z);
    const glm::vec3 w = glm::vec3(dir.x, dir.y, dir.z * s);
    const glm::vec3 h = glm::vec3(axis.x, axis.y, axis.z + s);
    const float k = dot(w, h) / (1.f + fabsf(axis.z));
    return k * h - w;
}

// ----------------------------------------------------
// Cartesian (x, y, z) <-> spherical (theta, phi) coordinate system conversions

inline glm::vec2 to_spherical(const glm::vec3& w) {
    const float theta = acosf(w.y);
    const float phi = atan2f(w.z, w.x);
    return glm::vec2(glm::clamp(theta, 0.f, PI), phi < 0.f ? phi + 2 * PI : phi);
}

inline glm::vec3 to_cartesian(const glm::vec2& w) {
    const float sin_t = sinf(w.x);
    return glm::vec3(sin_t * cosf(w.y), sin_t * sinf(w.y), cosf(w.x));
}

// ----------------------------------------------------
// Phase functions

inline float phase_isotropic() {
    return 1.f / (4.f * PI);
}
inline float phase_rayleigh(float cos_t) {
    return  3.f / (16.f * PI) * (1 + sqr(cos_t));
}
inline float phase_mie_hazy(float cos_t) {
    return (0.5f + 4.5f * powf(0.5f * (1.f + cos_t), 8)) / (4.f * PI);
}
inline float phase_mie_murky(float cos_t) {
    return (0.5f + 16.5f * powf(0.5f * (1.f + cos_t), 32)) / (4.f * PI);
}

/**
 * @brief Henyey Greenstein phase function
 *
 * @param cos_t Cosine of the angle between w_o and w_i
 * @param g Anisotropy parameter in [-1, 1]
 */
// inline float phase_henyey_greenstein(const glm::vec3& w_o, const glm::vec3& w_i, float g) {
    // const float cos_t = dot(w_o, w_i);
inline float phase_henyey_greenstein(float cos_t, float g) {
    const float denom = 1 + sqr(g) + 2 * g * cos_t;
    return INV4PI * (1 - sqr(g)) / (denom * sqrt(denom));
}

/**
 * @brief Draw direction according to Henyey Greenstein phase function
 *
 * @param w_o Outgoing direction
 * @param sample Random samples in [0, 1)
 * @param g Anisotropy parameter in [-1, 1]
 *
 * @return Incident direction according to phase function
 */
inline glm::vec3 sample_henyey_greenstein(const glm::vec3& w_o, const glm::vec2& sample, float g) {
    const float cos_t = abs(g) < 1e-4f ? 1.f - 2.f * sample.x :
        (1 + sqr(g) - sqr((1 - sqr(g)) / (1 - g + 2 * g * sample.x))) / (2 * g);
    const float sin_t = sqrtf(fmaxf(0.f, 1.f - sqr(cos_t)));
    const float phi = 2.f * M_PI * sample.y;
    return tangent_to_world(-w_o, glm::vec3(sin_t * cosf(phi), sin_t * sinf(phi), cos_t));
}
