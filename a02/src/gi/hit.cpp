#include "hit.h"
#include "brdf.h"
#include "timer.h"

// ---------------------------------------------------------
// SurfaceHit

SurfaceHit::SurfaceHit(const SkyLight* sky) : Hit(false), light(sky) {}

SurfaceHit::SurfaceHit(const Ray& ray, const Mesh* mesh) : Hit(true), mesh(mesh), mat(mesh->mat.get()), light(0) {
    assert(mesh); assert(mat);
    STAT("hit point lerp");
    // fetch indices and baryzentric coords
    const glm::uvec3& tri = mesh->ibo[ray.primID];
    const float u = ray.u;
    const float v = ray.v;
    const float w = 1.f - u - v;
    // compute position
    P = ray.org + ray.tfar * ray.dir;
    // interpolate normal
    Ng = w * mesh->normals[tri[0]] + u * mesh->normals[tri[1]] + v * mesh->normals[tri[2]];
    // interpolate texcoord
    if (!mesh->tcs.empty())
        TC = w * mesh->tcs[tri[0]] + u * mesh->tcs[tri[1]] + v * mesh->tcs[tri[2]];
    // compute hit primitive area
    area = 0.5 * glm::length(glm::cross(mesh->vbo[tri[1]] - mesh->vbo[tri[0]], mesh->vbo[tri[2]] - mesh->vbo[tri[0]]));
    // apply normalmapping
    N = mat->normalmap(Ng, TC);
    // light source hit?
    if (mesh->is_light())
        light = mesh->area_light.get();
}

SurfaceHit::SurfaceHit(const glm::vec2& sample, uint32_t primID, const Mesh* mesh) : Hit(true), mesh(mesh), mat(mesh->mat.get()), light(0) {
    assert(mesh); assert(mat);
    STAT("mesh surface sample");
    // fetch indices and baryzentric coords
    const glm::uvec3& tri = mesh->ibo[primID];
    const glm::vec2 uv = uniform_sample_triangle(sample);
    const float u = uv.x;
    const float v = uv.y;
    const float w = 1.f - u - v;
    // interpolate position
    P = w * mesh->vbo[tri[0]] + u * mesh->vbo[tri[1]] + v * mesh->vbo[tri[2]];
    // interpolate normal
    Ng = w * mesh->normals[tri[0]] + u * mesh->normals[tri[1]] + v * mesh->normals[tri[2]];
    // interpolate texcoord
    if (!mesh->tcs.empty())
        TC = w * mesh->tcs[tri[0]] + u * mesh->tcs[tri[1]] + v * mesh->tcs[tri[2]];
    // compute hit primitive area
    area = 0.5 * glm::length(glm::cross(mesh->vbo[tri[1]] - mesh->vbo[tri[0]], mesh->vbo[tri[2]] - mesh->vbo[tri[0]]));
    // apply normalmapping
    N = mat->normalmap(Ng, TC);
    // light source hit?
    if (mesh->is_light())
        light = mesh->area_light.get();
}

SurfaceHit::SurfaceHit(const glm::vec3& position, const glm::vec3& normal) : Hit(true), P(position), Ng(normal), N(normal), TC(0), area(0), mesh(0), mat(0), light(0) {}

glm::vec3 SurfaceHit::f(const glm::vec3& w_o, const glm::vec3& w_i) const {
    assert(mat);
    STAT("BRDF eval");
    return mat->brdf->f(*this, w_o, w_i);
}

std::tuple<glm::vec3, glm::vec3, float> SurfaceHit::sample(const glm::vec3& w_o, const glm::vec2& sample) const {
    assert(mat);
    STAT("BRDF sample");
    const auto [brdf, w_i, pdf] = mat->brdf->sample(*this, w_o, sample);
    assert(std::isfinite(pdf));
    return { brdf, w_i, pdf };
}

float SurfaceHit::pdf(const glm::vec3& w_o, const glm::vec3& w_i) const {
    assert(mat);
    STAT("BRDF pdf");
    const float pdf = mat->brdf->pdf(*this, w_o, w_i);
    assert(std::isfinite(pdf));
    return pdf;
}

// ---------------------------------------------------------
// VolumeHit

VolumeHit::VolumeHit() : Hit(false) {}

VolumeHit::VolumeHit(const glm::vec3& position, const Volume* volume) : Hit(true), P(position), volume(volume) {}

glm::vec3 VolumeHit::f(const glm::vec3& w_o, const glm::vec3& w_i) const {
    assert(volume);
    STAT("Phase eval");
    return glm::vec3(phase_henyey_greenstein(glm::dot(w_o, w_i), volume->phase_g));
}

std::tuple<glm::vec3, glm::vec3, float> VolumeHit::sample(const glm::vec3& w_o, const glm::vec2& sample) const {
    assert(volume);
    STAT("Phase sample");
    const glm::vec3 w_i = sample_henyey_greenstein(w_o, sample, volume->phase_g);
    const float phase = phase_henyey_greenstein(glm::dot(w_o, w_i), volume->phase_g);
    return { glm::vec3(phase), w_i, phase };
}

float VolumeHit::pdf(const glm::vec3& w_o, const glm::vec3& w_i) const {
    assert(volume);
    STAT("Phase pdf");
    return phase_henyey_greenstein(glm::dot(w_o, w_i), volume->phase_g);
}
