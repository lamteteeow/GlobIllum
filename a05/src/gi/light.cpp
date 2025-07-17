#include "light.h"
#include "ray.h"
#include "scene.h"
#include "texture.h"
#include "buffer.h"
#include "mesh.h"
#include "material.h"
#include "sampling.h"
#include "distribution.h"
#include "timer.h"
#include "color.h"
#include <iostream>

// ------------------------------------------------
// Mesh area light

AreaLight::AreaLight(const Mesh& mesh) : mesh(mesh) {}

std::tuple<glm::vec3, Ray, float> AreaLight::sample_Li(const glm::vec3& position, const glm::vec2& sample) const {
    assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
    STAT("sampleLi");
    // sample area light source (triangle mesh)
    const auto [light, sample_pdf] = mesh.sample(sample);
    if (sample_pdf <= 0.f) return { glm::vec3(0.f), Ray(), 0.f };
    glm::vec3 l = light.P - position;
    const float r = length(l);
    l = normalize(l);
    const float cos_t_light = dot(light.N, -l);
    if (cos_t_light <= 0.f) return { glm::vec3(0.f), Ray(), 0.f };
    const float pdf = sample_pdf * (r * r) / (cos_t_light * light.area);
    assert(std::isfinite(pdf));
    return { light.Le(), Ray(position, l, r), pdf };
}

float AreaLight::pdf_Li(const SurfaceHit& light, const Ray& ray) const {
    assert(light.valid && light.mesh && light.area > 0);
    const float cos_t = dot(light.N, -ray.dir);
    if (cos_t <= 0.f) return 0.f;
    return (ray.tfar * ray.tfar) / (cos_t * light.area);
}

std::tuple<glm::vec3, Ray, glm::vec3, float, float> AreaLight::sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const {
    STAT("sampleLe");
    const auto [light, pdf_sample] = mesh.sample(sample_pos);
    if (pdf_sample <= 0.f) return { glm::vec3(0), Ray(), glm::vec3(0), 0.f, 0.f };
    const glm::vec3 dir = light.to_world(cosine_sample_hemisphere(sample_dir));
    const float pdf_pos = pdf_sample / light.area;
    const float pdf_dir = cosine_hemisphere_pdf(glm::dot(light.N, dir));
    assert(std::isfinite(pdf_pos) && std::isfinite(pdf_dir));
    return { light.Le(), Ray(light.P, dir), light.N, pdf_pos, pdf_dir };
}

std::tuple<float, float> AreaLight::pdf_Le(const SurfaceHit& light, const glm::vec3& dir) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 AreaLight::power() const {
    return glm::vec3(mesh.mat->emissive_strength * mesh.surface_area() * PI);
}

// ------------------------------------------------
// Sky light

SkyLight::SkyLight() {}

SkyLight::SkyLight(const std::filesystem::path& path, const Scene& scene, float intensity) {
    load(path, scene.center, scene.radius, intensity);
}

void SkyLight::load(const std::filesystem::path& path, const glm::vec3& scene_center, float scene_radius, float intensity) {
    // init variables and load texture from disk
    const std::filesystem::path resolved_path = std::filesystem::exists(path) ? path : std::filesystem::path(GI_DATA_DIR) / path;
    std::cout << "loading: " << path << " (" << resolved_path << ")..." << std::endl;
    texture = std::make_shared<Texture>(resolved_path);
    this->intensity = intensity;
    this->scene_center = scene_center;
    this->scene_radius = scene_radius;
}

void SkyLight::build_distribution() {
    // init distribution for importance sampling
    Buffer<float> importance(texture->w, texture->h);
    for (size_t y = 0; y < texture->h; ++y) {
        // counteract distortion
        float sin_theta = sinf(PI * float(y + .5f) / float(texture->h));
        for (size_t x = 0; x < texture->w; ++x)
            importance(x, y) = luma(texture->operator()(static_cast<uint64_t>(x), static_cast<uint64_t>(y))) * sin_theta;
    }
    distribution = std::make_shared<Distribution2D>(importance.data(), importance.width(), importance.height());
}

std::tuple<glm::vec3, Ray, float> SkyLight::sample_Li(const glm::vec3& position, const glm::vec2& sample) const {
    assert(texture && distribution);
    assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
    STAT("sampleLi");
    // importance sample sky light
    const auto [uv, sample_pdf] = distribution->sample_01(sample);
    if (sample_pdf <= 0.f) return { glm::vec3(0.f), Ray(), 0.f };
    // convert to spherical coordinates
    const float theta = uv.y * PI, phi = uv.x * 2 * PI;
    // map to unit sphere
    const float cos_theta = cosf(theta), sin_theta = sinf(theta);
    if (sin_theta <= 0.f) return { glm::vec3(0), Ray(), 0.f };
    const glm::vec3 dir = glm::vec3(sin_theta * cosf(phi), cos_theta, sin_theta * sinf(phi));
    const float pdf = sample_pdf / (2.f * PI * PI * sin_theta);
    assert(std::isfinite(pdf));
    return { texture->env(dir) * intensity, Ray(position, dir), pdf };
}

float SkyLight::pdf_Li(const SurfaceHit& light, const Ray& ray) const {
    const glm::vec2 theta_phi = to_spherical(ray.dir);
    const float sin_t = sinf(theta_phi.x);
    if (sin_t <= 0.f) return 0.f;
    return distribution->pdf(glm::vec2(theta_phi.y * INV2PI, theta_phi.x * INVPI)) / (2.f * PI * PI * sin_t);
}

glm::vec3 SkyLight::Le(const Ray& ray) const {
    assert(texture && distribution);
    return texture->env(ray.dir) * intensity;
}

std::tuple<glm::vec3, Ray, glm::vec3, float, float> SkyLight::sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const {
    assert(texture && distribution);
    STAT("sampleLe");
    // draw sample and convert to direction
    const auto [uv, sample_pdf] = distribution->sample_01(sample_pos);
    if (sample_pdf <= 0.f) return { glm::vec3(0), Ray(), glm::vec3(0), 0.f, 0.f };
    // convert to spherical coordinates
    const float theta = uv.y * PI, phi = uv.x * 2 * PI;
    // map to unit sphere
    const float cos_theta = cosf(theta), sin_theta = sinf(theta);
    if (sin_theta <= 0.f) return { glm::vec3(0), Ray(), glm::vec3(0), 0.f, 0.f };
    const glm::vec3 dir(sin_theta * cosf(phi), cos_theta, sin_theta * sinf(phi));
    // sample disk approximation of scene
    const auto [T, B] = build_tangent_frame(-dir);
    const glm::vec2 disk_sample = concentric_sample_disk(sample_dir);
    const glm::vec3 out = scene_center + scene_radius * dir;
    const glm::vec3 orig = out + scene_radius * (disk_sample.x * T + disk_sample.y * B);
    // compute pdfs
    const float pdf_pos = sample_pdf / (2.f * PI * PI * sin_theta);
    const float pdf_dir = 1.f / (PI * scene_radius * scene_radius);
    assert(std::isfinite(pdf_pos) && std::isfinite(pdf_dir));
    return { texture->env(dir) * intensity, Ray(orig, -dir), -dir, pdf_pos, pdf_dir };
}

std::tuple<float, float> SkyLight::pdf_Le(const SurfaceHit& light, const glm::vec3& dir) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 SkyLight::power() const {
    assert(texture && distribution);
    return glm::vec3(PI * scene_radius * scene_radius * intensity * distribution->unit_integral());
}

inline std::string fix_data_path(std::string path) {
    // remove GI_DATA_DIR from path to avoid absolute paths
    if (path.find(GI_DATA_DIR) != std::string::npos)
        path = path.substr(path.find(GI_DATA_DIR) + std::strlen(GI_DATA_DIR) + 1);
    return path;
}

json11::Json SkyLight::to_json() const {
    return json11::Json::object{
        { "envmap", fix_data_path(texture->path().string()) },
        { "intensity", intensity },
        { "scene_center", json11::Json::array{scene_center.x, scene_center.y, scene_center.z} },
        { "scene_radius", scene_radius },
    };
}

void SkyLight::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        json_set_float(cfg, "intensity", intensity);
        json_set_vec3(cfg, "scene_center", scene_center);
        json_set_float(cfg, "scene_radius", scene_radius);
        if (cfg["envmap"].is_string())
            load(cfg["envmap"].string_value(), scene_center, scene_radius, intensity);
        else
            texture = std::make_shared<Texture>(glm::vec3(1));
        build_distribution();
    }
}
