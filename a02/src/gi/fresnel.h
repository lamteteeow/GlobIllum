#pragma once

#include "hit.h"

// -------------------------------------------------------------------
// Approximations

inline float fresnel_schlick(float cos_i, float index_of_refraction) {
    // TODO ASSIGNMENT2
    // implement Schlick's approximation
    // Calculate reflectance at normal incidence)
    float n1 = 1.0f;  // Air
    float n2 = index_of_refraction;
    float r0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));

    // Compute Schlick's approximation
    return r0 + (1.0f - r0) * glm::pow((1.0f - cos_i), 5.0f);
}

// -------------------------------------------------------------------
// Dielectric materials

inline float fresnel_dielectric(float cos_wi, float ior_medium, float ior_material) {
    // simply use schlick's approximation for now (for the LayeredSurface BRDF to work)
    return fresnel_schlick(cos_wi, ior_material);
}

// -------------------------------------------------------------------
// Conductor materials

inline float fresnel_conductor(float cos_wi, float ior_material, float absorb) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
