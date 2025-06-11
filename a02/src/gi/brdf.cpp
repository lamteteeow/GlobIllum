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
    // TODO ASSIGNMENT2
    // evaluate the (normalized!) lambertian diffuse BRDF
	// Note: no roughness parameter is needed here, as the BRDF is constant
    return hit.albedo() * vec3(INVPI);
}

std::tuple<vec3, vec3, float> LambertianReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    // TODO ASSIGNMENT2
    // importance sample and evaluate the lambertian diffuse BRDF
    // set w_i to the sampled (world-space!) direction, pdf to the respective PDF and brdf to the evaluated BRDF
    const vec3 w_i_local = cosine_sample_hemisphere(sample);

    // Transform incident direction to world space
    vec3 w_i = hit.to_world(w_i_local);

    const vec3 brdf = hit.albedo() * vec3(INVPI); // no function call needed

	float cos_term = max(0.0f, dot(hit.N, w_i));;

    const float pdf = cosine_hemisphere_pdf(cos_term);
    return { brdf, w_i, pdf };
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
    // TODO ASSIGNMENT2
    // evaluate the (normalized!) phong BRDF for the given in- and outgoing (world-space) directions
    // you may use hit.albedo() as the specular color here
    const float exponent = Material::exponent_from_roughness(hit.roughness());
    const float index_of_refraction = hit.mat->ior;

	// Normalization factor for Phong BRDF to ensure energy conservation
    float normalization = (exponent + 1.0f) / (2.0f * M_PI);

    // Half-way vector for Blinn-Phong BRDF
    vec3 h = normalize(w_o + w_i);

    // Cosine term
	float cosine_term = dot(hit.N, h);

    // Calculate BRDF
    return hit.albedo() * fresnel_schlick(cosine_term, index_of_refraction) * normalization * pow(cosine_term, exponent);
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
    // TODO ASSIGNMENT2 (optional)
    // compute the GGX D term here
    // GGX normal distribution function
    float m2 = roughness * roughness;
    float cos2 = NdotH * NdotH;
	float tan2 = (1.0f - cos2) / cos2;
	float cos4 = cos2 * cos2;
    return m2 / (M_PI * cos4 * pow((m2 + tan2), 2.0f));
}

inline float GGX_G1(const float NdotV, float roughness) {
    // TODO ASSIGNMENT2 (optional)
    // compute the GGX G1 term here
    float m2 = roughness * roughness;
    float cos2 = NdotV * NdotV;
	float tan2 = (1.0f - cos2) / cos2;
    return 2.0f / (1.0f + sqrt(1.0f + m2 * tan2));
}

vec3 GGX_sample(const vec2& sample, float roughness) {
    // TODO ASSIGNMENT2 (optional)
    // implement sampling the GGX distribution here
    // return a mircofacet normal in tangent space
    const float phi = sample.y * 2.f * M_PI;
    const float tan_t = (roughness * sqrt(sample.x)) / sqrt(1.f - sample.x);
    const float cos_t = 1.f / sqrt(1.f + sqr(tan_t));
    const float sin_t = sqrt(1.f - sqr(cos_t));
    return vec3(sin_t * cos(phi), sin_t * sin(phi), cos_t);
}

inline float GGX_pdf(float D, float NdotH, float HdotV) {
    // TODO ASSIGNMENT2 (optional)
    // compute the microfacet PDF here
    const float h_differential = 1.f / (4.f * abs(HdotV));
    const float m_probability = D * abs(NdotH);
    return m_probability * h_differential;
}

// ----------------------------------------------------------------------------------------------
// Microfacet reflection

vec3 MicrofacetReflection::f(const SurfaceHit &hit, const vec3 &w_o, const vec3 &w_i) const {
    // TODO ASSIGNMENT2
    // evaluate the full microfacet BRDF here, optionally relying on the above functions for the D and G1 terms
    // note: use schlick's approximation for the F term
    const float alpha = hit.roughness();
    float microfacet = 0.f;

    vec3 h = normalize(w_o + w_i);

    float cosine_term = dot(hit.N, h);

    const float index_of_refraction = hit.mat->ior;
	float F = fresnel_schlick(cosine_term, index_of_refraction);

	float nominator = F * GGX_D(cosine_term, alpha) * GGX_G1(dot(hit.N, w_o), alpha);
	float denominator = 4.0f * abs(dot(hit.N, w_i)) * abs(dot(hit.N, w_o));

	microfacet = nominator / denominator;

    return coated ? vec3(microfacet) : hit.albedo() * microfacet;
}

std::tuple<vec3, vec3, float> MicrofacetReflection::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    // TODO ASSIGNMENT2
    // importance sample and evaluate this microfacet BRDF
    // set w_i to the sampled (world-space!) direction, pdf to the respective PDF and brdf to the evaluated BRDF
    //const vec3 w_i = vec3(0);
    //const vec3 brdf = vec3(0);
    //const float pdf = 0.f;
    //return { brdf, w_i, pdf };

    // Get roughness parameter
    const float alpha = hit.roughness();

    // Transform view direction to local space
    vec3 w_o_local = hit.to_tangent(w_o);

    vec3 m = GGX_sample(sample, alpha);

    vec3 w_i_local = reflect(-w_o_local, m);

    // Ensure the incident direction is above the surface
    if (w_i_local.z <= 0.0f) {
        return { vec3(0.0f), vec3(0.0f), 0.0f };
    }

    vec3 w_i = hit.to_world(w_i_local);

	vec3 h = normalize(w_i + w_o);

    float NdotH = dot(m, h);
    float HdotV = dot(h, w_o); 

    float D = GGX_D(NdotH, alpha);

	vec3 brdf = f(hit, w_o, w_i);

    // Calculate PDF
    float pdf = GGX_pdf(D, NdotH, HdotV);

	return { brdf, w_i, pdf };
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
    const float F = fresnel_dielectric(abs(dot(hit.N, w_o)), 1.f, hit.mat->ior);
    return mix(diff.f(hit, w_o, w_i), spec.f(hit, w_o, w_i), F);
}

std::tuple<vec3, vec3, float> LayeredSurface::sample(const SurfaceHit& hit, const vec3& w_o, const vec2& sample) const {
    const float F = fresnel_dielectric(abs(dot(hit.N, w_o)), 1.f, hit.mat->ior);
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
    const float F = fresnel_dielectric(abs(dot(hit.N, w_o)), 1.f, hit.mat->ior);
    return mix(diff.pdf(hit, w_o, w_i), spec.pdf(hit, w_o, w_i), F);
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
