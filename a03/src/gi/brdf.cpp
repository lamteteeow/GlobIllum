#include "brdf.h"
#include "hit.h"
#include "fresnel.h"
#include "material.h"
#include "sampling.h"
#include "color.h"
#include <cmath>

using namespace glm;

// ----------------------------------------------------------------------------------------------
// Diffuse lambertian reflection

vec3 LambertianReflection::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    if (dot(hit.N, w_o) <= 0 || dot(hit.N, w_i) <= 0) return vec3(0);
    return hit.albedo() * INVPI;
}

std::tuple<vec3, vec3, float> LambertianReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    vec3 w_i = hit.to_world(cosine_sample_hemisphere(sample));
    if (dot(hit.N, w_i) < 0.f) w_i *= -1.f;
    return { f(hit, w_o, w_i), w_i, pdf(hit, w_o, w_i) };
}

float LambertianReflection::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    return max(0.f, dot(hit.N, w_i)) * INVPI;
}

// ----------------------------------------------------------------------------------------------
// Diffuse lambertian transmission

vec3 LambertianTransmission::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    return hit.albedo() * INVPI;
}

std::tuple<vec3, vec3, float> LambertianTransmission::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    vec3 w_i = hit.to_world(cosine_sample_hemisphere(sample));
    if (dot(hit.N, w_i) > 0.f) w_i *= -1.f;
    return { f(hit, w_o, w_i), w_i, pdf(hit, w_o, w_i) };
}

float LambertianTransmission::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    return abs(dot(hit.N, w_i)) * INVPI;
}

// ----------------------------------------------------------------------------------------------
// Perfect specular reflection

vec3 SpecularReflection::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    return vec3(0);
}

std::tuple<vec3, vec3, float> SpecularReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    const vec3 w_i = reflect(-w_o, hit.N);
    const vec3 brdf = fresnel_dielectric(dot(hit.N, w_i), 1.f, hit.mat->ior) * hit.albedo() / abs(dot(hit.N, w_i));
    return { brdf, w_i, 1.f };
}

float SpecularReflection::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    return 0.f;
}

// ----------------------------------------------------------------------------------------------
// Perfect specular transmission

vec3 SpecularTransmission::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    return vec3(0);
}

std::tuple<vec3, vec3, float> SpecularTransmission::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    const auto [valid, w_i] = refract(-w_o, hit.N, hit.mat->ior);
    if (!valid) return { vec3(0), w_i, 0.f };
    const vec3 brdf = (1.f - fresnel_dielectric(dot(hit.N, w_i), 1.f, hit.mat->ior)) * hit.albedo() / abs(dot(hit.N, w_i));
    // TODO: account for non-symmetric transport
    return { brdf, w_i, 1.f };
}

float SpecularTransmission::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    return 0.f;
}

// ----------------------------------------------------------------------------------------------
// Specular fresnel

vec3 SpecularFresnel::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    return vec3(0);
}

std::tuple<vec3, vec3, float> SpecularFresnel::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    const float p_r = fresnel_dielectric(dot(hit.N, w_o), 1.f, hit.mat->ior);
    if (sample.x < p_r) {   // sample reflection
        const vec3 w_i = reflect(-w_o, hit.N);
        const vec3 brdf = fresnel_dielectric(dot(hit.N, w_i), 1.f, hit.mat->ior) * hit.albedo() / abs(dot(hit.N, w_i));
        return { brdf, w_i, p_r };
    } else {                // sample refraction
        const auto [valid, w_i] = refract(-w_o, hit.N, hit.mat->ior);
        if (!valid) return { vec3(0), w_i, 0.f }; // TIR
        const vec3 brdf = (1.f - fresnel_dielectric(dot(hit.N, w_i), 1.f, hit.mat->ior)) * hit.albedo() / abs(dot(hit.N, w_i));
        // TODO: account for non-symmetric transport
        return { brdf, w_i, 1 - p_r };
    }
}

float SpecularFresnel::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    return 0.f;
}

// ----------------------------------------------------------------------------------------------
// Phong

vec3 SpecularPhong::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    if (!same_hemisphere(hit.N, w_i)) return vec3(0);
    const float exponent = Material::exponent_from_roughness(hit.roughness());
    const float NdotH = fmaxf(0.f, dot(hit.N, normalize(w_o + w_i)));
    const float norm_f = (exponent + 1) * INV2PI;
    const float F = fresnel_dielectric(dot(hit.N, w_i), 1.f, hit.mat->ior);
    return F * hit.albedo() * powf(NdotH, exponent) * norm_f;
}

std::tuple<vec3, vec3, float> SpecularPhong::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float SpecularPhong::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Microfacet distribution helper functions

inline float GGX_D(const float NdotH, float roughness) {
    const float tan2 = tan2_theta(NdotH);
    if (!std::isfinite(tan2)) return 0.f;
    const float a2 = sqr(roughness);
    assert(M_PI * sqr(sqr(NdotH)) * sqr(a2 + tan2) != 0.f);
    return a2 / (M_PI * sqr(sqr(NdotH)) * sqr(a2 + tan2));
}

inline float GGX_G1(const float NdotV, float roughness) {
    const float tan2 = tan2_theta(NdotV);
    if (!std::isfinite(tan2)) return 0.f;
    assert(1.f + sqrtf(1.f + sqr(roughness) * tan2) != 0.f);
    return 2.f / (1.f + sqrtf(1.f + sqr(roughness) * tan2));
}

vec3 GGX_sample(const vec2& sample, float roughness) {
    assert(sample.x != 0.f);
    const float theta = atanf((roughness * sqrtf(sample.x)) / sqrtf(1.f - sample.x));
    if (!std::isfinite(theta)) return vec3(0, 0, 1);
    const float phi = 2.f * M_PI * sample.y;
    const float sin_t = sinf(theta);
    return vec3(sin_t * cosf(phi), sin_t * sinf(phi), cosf(theta));
}

inline float GGX_pdf(float D, float NdotH, float HdotV) {
    assert(4.f * abs(HdotV) != 0.f);
    return (D * abs(NdotH)) / (4.f * abs(HdotV));
}

// ----------------------------------------------------------------------------------------------
// Microfacet reflection

vec3 MicrofacetReflection::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    const float NdotV = abs(dot(hit.N, w_o));
    const float NdotL = abs(dot(hit.N, w_i));
    if (NdotV == 0.f || NdotL == 0.f) return vec3(0);
    const vec3 H = normalize(w_o + w_i);
    const float roughness = hit.roughness();
    const float F = fresnel_dielectric(dot(H, w_i), 1.f, hit.mat->ior);
    const float D = GGX_D(abs(dot(hit.N, H)), roughness);
    const float G = GGX_G1(NdotV, roughness) * GGX_G1(NdotL, roughness);
    const float microfacet = (F * D * G) / (4 * NdotV * NdotL);
    return coated ? vec3(microfacet) : hit.albedo() * microfacet;
}

std::tuple<vec3, vec3, float> MicrofacetReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    // reflect around sampled macro normal w_h
    const vec3 w_h = hit.to_world(GGX_sample(sample, hit.roughness()));
    const vec3 w_i = reflect(-w_o, w_h);
    if (!same_hemisphere(hit.Ng, w_i)) return { vec3(0), w_i, 0.f };
    const float sample_pdf = pdf(hit, w_o, w_i);
    assert(std::isfinite(sample_pdf));
    return { f(hit, w_o, w_i), w_i, sample_pdf };
}

float MicrofacetReflection::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    const vec3 H = normalize(w_o + w_i);
    const float NdotH = dot(hit.N, H);
    const float HdotV = dot(H, w_o);
    const float D = GGX_D(NdotH, hit.roughness());
    return GGX_pdf(D, NdotH, HdotV);
}

// ------------------------------------------------
// Microfacet transmission

vec3 MicrofacetTransmission::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> MicrofacetTransmission::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float MicrofacetTransmission::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// -------------------------------------------------------------------------------------------
// Layered

vec3 LayeredSurface::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    const float F = fresnel_dielectric(dot(hit.N, w_o), 1.f, hit.mat->ior);
    return mix(diff.f(hit, w_o, w_i), spec.f(hit, w_o, w_i), F);
}

std::tuple<vec3, vec3, float> LayeredSurface::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    const float F = fresnel_dielectric(dot(hit.N, w_o), 1.f, hit.mat->ior);
    vec3 brdf;
    if (sample.x < F) {
        // sample specular
        const vec2 sample_mapped = vec2((F - sample.x) / F, sample.y);
        const auto [specular, w_i, sample_pdf] = spec.sample(hit, w_o, sample_mapped);
        if (!same_hemisphere(hit.Ng, w_i)) return { vec3(0), w_i, 0.f };
        assert(std::isfinite(sample_pdf));
        return { mix(diff.f(hit, w_o, w_i), specular, F), w_i, mix(diff.pdf(hit, w_o, w_i), sample_pdf, F) };
    } else {
        // sample diffuse
        const vec2 sample_mapped = vec2((sample.x - F) / (1 - F), sample.y);
        const auto [diffuse, w_i, sample_pdf] = diff.sample(hit, w_o, sample_mapped);
        if (!same_hemisphere(hit.Ng, w_i)) return { vec3(0), w_i, 0.f };
        assert(std::isfinite(sample_pdf));
        return { mix(diffuse, spec.f(hit, w_o, w_i), F), w_i, mix(sample_pdf, spec.pdf(hit, w_o, w_i), F) };
    }
}

float LayeredSurface::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    const float F = fresnel_dielectric(dot(hit.N, w_o), 1.f, hit.mat->ior);
    return mix(diff.pdf(hit, w_o, w_i), spec.pdf(hit, w_o, w_i), F);
}

// ----------------------------------------------------------------------------------------------
// Metal

vec3 MetallicSurface::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    const float NdotV = dot(hit.N, w_o);
    const float NdotL = dot(hit.N, w_i);
    if (NdotV <= 0.f || NdotL <= 0.f) return vec3(0);
    const vec3 H = normalize(w_o + w_i);
    const float NdotH = dot(hit.N, H);
    const float HdotL = dot(H, w_i);
    const float roughness = hit.roughness();
    const float F = fresnel_conductor(HdotL, hit.mat->ior, hit.mat->absorb);
    const float D = GGX_D(NdotH, roughness);
    const float G = GGX_G1(NdotV, roughness) * GGX_G1(NdotL, roughness);
    const float microfacet = (F * D * G) / (4 * abs(NdotV) * abs(NdotL));
    return hit.albedo() * microfacet;
}
