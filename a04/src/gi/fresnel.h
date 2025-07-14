#pragma once

#include "hit.h"

// -------------------------------------------------------------------
// Approximations

inline float fresnel_schlick(float cos_i, float index_of_refraction) {
    const float R = sqr((1 - index_of_refraction) / (1 + index_of_refraction));
    const float revcos = 1.f - cos_i;
    const float revcos2 = revcos * revcos;
    return R + (1 - R) * revcos2 * revcos2 * revcos;
}

// -------------------------------------------------------------------
// Dielectric materials

inline float fresnel_dielectric(float cos_wi, float ior_medium, float ior_material) {
    // check if entering or leaving material
    const float ei = cos_wi >= 0.f ? ior_medium : ior_material;
    const float et = cos_wi >= 0.f ? ior_material : ior_medium;
    cos_wi = glm::clamp(glm::abs(cos_wi), 0.f, 1.f);
    // snell's law
    const float sin_t = (ei / et) * sqrtf(1.f - cos_wi * cos_wi);
    // handle TIR
    if (sin_t >= 1.f) return 1.f;
    const float cos_t = sqrtf(fmaxf(0.f, 1.f - sin_t * sin_t));
    const float Rparl = ((et * cos_wi) - (ei * cos_t)) / ((et * cos_wi) + (ei * cos_t));
    const float Rperp = ((ei * cos_wi) - (et * cos_t)) / ((ei * cos_wi) + (et * cos_t));
    return (Rparl * Rparl + Rperp * Rperp) / 2;
}

// -------------------------------------------------------------------
// Conductor materials

inline float fresnel_conductor(float cos_wi, float ior_material, float absorb) {
    const float cosi = fabsf(cos_wi);
    const float eta = ior_material;
    const float eta2 = eta * eta;
    const float absorb2 = absorb * absorb;
    const float cosi2 = cosi * cosi;
    const float tmp = (eta2 + absorb2) * cosi2;
    const float Rparl2 = (tmp - (2 * eta * cosi) + 1) / (tmp + (2 * eta * cosi) + 1);
    const float tmp_f = eta2 + absorb2;
    const float Rperp2 = (tmp_f - (2 * eta * cosi) + cosi2) / (tmp_f + (2 * eta * cosi) + cosi2);
    return (Rparl2 + Rperp2) * .5f;
}
