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
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> LambertianReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float LambertianReflection::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Diffuse lambertian transmission

vec3 LambertianTransmission::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> LambertianTransmission::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float LambertianTransmission::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Perfect specular reflection

vec3 SpecularReflection::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> SpecularReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float SpecularReflection::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Perfect specular transmission

vec3 SpecularTransmission::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> SpecularTransmission::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float SpecularTransmission::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Specular fresnel

vec3 SpecularFresnel::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> SpecularFresnel::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float SpecularFresnel::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Phong

vec3 SpecularPhong::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
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
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

inline float GGX_G1(const float NdotV, float roughness) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

vec3 GGX_sample(const vec2& sample, float roughness) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

inline float GGX_pdf(float D, float NdotH, float HdotV) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Microfacet reflection

vec3 MicrofacetReflection::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> MicrofacetReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float MicrofacetReflection::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
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
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> LayeredSurface::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float LayeredSurface::pdf(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Metal

vec3 MetallicSurface::f(const SurfaceHit& hit, const vec3& w_o, const vec3& w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// ----------------------------------------------------------------------------------------------
// Glass

vec3 GlassSurface::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<vec3, vec3, float> GlassSurface::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float GlassSurface::pdf(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
