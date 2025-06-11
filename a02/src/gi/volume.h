#pragma once
#include <vector>
#include <filesystem>

#include "ray.h"
#include "json11.h"
#include "glm/glm.hpp"

// ----------------------------------------------------
// NVDBGrid: load and sample a NanoVDB density grid (in index-space)

class NVDBGrid {
public:
    NVDBGrid(const std::filesystem::path& path, float density_scale = 1.f);
    virtual ~NVDBGrid();

    float min_value() const;
    float max_value() const;

    float lookup_density(const glm::uvec3& ipos) const;             // input in index-space!
    float lookup_density_trilinear(const glm::vec3& ipos) const;    // input in index-space!
    float lookup_density_stochastic(const glm::vec3& ipos) const;   // input in index-space!

    // data
    std::vector<char> nvdb_data;    // raw nanovdb grid data
    glm::vec3 ibb_min, ibb_max;     // index-space bounding box
    glm::mat4 transform;            // affine transform from index to world space
    float density_scale;            // linearly scale density
};

// ----------------------------------------------------
// Volume: query volume for intersection and transmittance

class Volume {
public:
    Volume(const std::filesystem::path& path, float density_scale = 1.f);
    virtual ~Volume();

    // coordinate space helpers
    glm::vec3 world_to_index(const glm::vec4& wpos) const;
    glm::vec3 index_to_world(const glm::vec4& ipos) const;

    // volume parameter helpers
    float albedo() const;
    float extinction_cross_section() const;

    // AABB intersection helpers (in world-space)
    std::tuple<glm::vec3, glm::vec3> compute_AABB() const;          // returns: <aabb_min, aabb_max>
    std::tuple<bool, float, float> intersect(const Ray& ray) const; // returns: <is_hit, near, far>

    // volume query functions (switches between raymarching and tracking based on unbiased_estimators flag)
    float transmittance(const Ray& ray) const;                      // returns: transmittance in [0, 1]
    std::tuple<bool, float> sample(const Ray& ray) const;           // returns: <is_hit, distance>

    // transmittance estimators and sampling routines
    float transmittance_raymarching(const Ray& ray) const;
    float transmittance_ratio_tracking(const Ray& ray) const;
    std::tuple<bool, float> sample_raymarching(const Ray& ray) const;
    std::tuple<bool, float> sample_delta_tracking(const Ray& ray) const;

    json11::Json to_json() const;
    void from_json(const json11::Json& cfg);

    // data
    NVDBGrid grid;                      // Sparse voxel grid containing density values (in 1/m^3)
    glm::mat4 model;                    // Volume model matrix
    float absorption_cross_section;     // Isotropic cross-sectional area of absorbing particles (in m^2)
    float scattering_cross_section;     // Isotropic cross-sectional area of scattering particles (in m^2)
    float phase_g;                      // Henyey-Greenstein phase function anisotropy parameter
    bool unbiased_estimators;           // Use unbiased (delta/ratio tracking) or biased (raymarching) estimators
    float raymarch_dt;                  // Raymarching step size in m
};
