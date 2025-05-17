#pragma once

#include "hit.h"

// -------------------------------------------------------------------
// Approximations

inline float fresnel_schlick(float cos_i, float index_of_refraction) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// -------------------------------------------------------------------
// Dielectric materials

inline float fresnel_dielectric(float cos_wi, float ior_medium, float ior_material) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// -------------------------------------------------------------------
// Conductor materials

inline float fresnel_conductor(float cos_wi, float ior_material, float absorb) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
