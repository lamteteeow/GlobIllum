#include "volume.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

#include "rng.h"
#include "NanoVDB.h"
#include "sampling.h"

using namespace glm;

// -----------------------------------------------
// NanoVDB loading helper (adapted from: https://github.com/NVlabs/instant-ngp/blob/master/src/testbed_volume.cu)

#define NANOVDB_MAGIC_NUMBER 0x304244566f6e614eUL // "NanoVDB0" in hex - little endian (uint64_t)
struct NanoVDBFileHeader {
    uint64_t magic;     // 8 bytes
    uint32_t version;   // 4 bytes version numbers
    uint16_t gridCount; // 2 bytes
    uint16_t codec;     // 2 bytes - must be 0
};
static_assert(sizeof(NanoVDBFileHeader) == 16, "nanovdb padding error");

struct NanoVDBMetaData {
    uint64_t gridSize, fileSize, nameKey, voxelCount; // 4 * 8 = 32B.
    uint32_t gridType;      // 4B.
    uint32_t gridClass;     // 4B.
    double worldBBox[2][3]; // 2 * 3 * 8 = 48B.
    int indexBBox[2][3];    // 2 * 3 * 4 = 24B.
    double voxelSize[3];    // 24B.
    uint32_t nameSize;      // 4B.
    uint32_t nodeCount[4];  // 4 x 4 = 16B
    uint32_t tileCount[3];  // 3 x 4 = 12B
    uint16_t codec;         // 2B
    uint16_t padding;       // 2B, due to 8B alignment from uint64_t
    uint32_t version;       // 4B
};
static_assert(sizeof(NanoVDBMetaData) == 176, "nanovdb padding error");

NVDBGrid::NVDBGrid(const std::filesystem::path& path, float density_scale) : density_scale(density_scale) {
    if (!std::filesystem::exists(path))
        throw std::runtime_error{path.string() + " does not exist."};
    std::ifstream f{path, std::ios::in | std::ios::binary};
    NanoVDBFileHeader header;
    NanoVDBMetaData metadata;
    f.read(reinterpret_cast<char*>(&header), sizeof(header));
    f.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));

    if (header.magic != NANOVDB_MAGIC_NUMBER)
        throw std::runtime_error{"Not a nanovdb file: " + path.string()};
    if (header.gridCount == 0)
        throw std::runtime_error{"No grids in file: " + path.string()};
    if (header.gridCount > 1)
        std::cerr << "Warning: Only loading first grid in file";
    if (metadata.codec != 0)
        throw std::runtime_error{"Can not use compressed nanovdb files."};
    char name[256] = {};
    if (metadata.nameSize > 256)
        throw std::runtime_error{"Nanovdb name too long."};
    f.read(name, metadata.nameSize);

    nvdb_data.resize(metadata.gridSize);
    f.read(nvdb_data.data(), metadata.gridSize);
    const nanovdb::FloatGrid* grid = reinterpret_cast<const nanovdb::FloatGrid*>(nvdb_data.data());
    if (!grid || !grid->isValid() || !grid->isFogVolume())
        throw std::runtime_error("Empty or invalid NanoVDB grid!");

    // compute index bounding box (lower-left corner and extent)
    const nanovdb::CoordBBox ibb = grid->indexBBox();
    ibb_min = grid->isEmpty() ? vec3(0) : vec3(ibb.min()[0], ibb.min()[1], ibb.min()[2]);
    ibb_max = grid->isEmpty() ? vec3(0) : vec3(ibb.max()[0] + 1, ibb.max()[1] + 1, ibb.max()[2] + 1);
    // extract transform
    transform = mat4(1);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            transform[i][j] = grid->map().mMatF[i * 3 + j];
        transform[3][i] = grid->map().mVecF[i];
    }
}

NVDBGrid::~NVDBGrid() {}

float NVDBGrid::min_value() const {
    const nanovdb::FloatGrid* grid = reinterpret_cast<const nanovdb::FloatGrid*>(nvdb_data.data());
    return density_scale * grid->tree().root().minimum();
}

float NVDBGrid::max_value() const {
    const nanovdb::FloatGrid* grid = reinterpret_cast<const nanovdb::FloatGrid*>(nvdb_data.data());
    return density_scale * grid->tree().root().maximum();
}

float NVDBGrid::lookup_density(const uvec3& ipos) const {
    const nanovdb::FloatGrid* grid = reinterpret_cast<const nanovdb::FloatGrid*>(nvdb_data.data());
    const auto acc = grid->getAccessor();
    return density_scale * acc.getValue(nanovdb::Coord(ipos.x, ipos.y, ipos.z));
}

float NVDBGrid::lookup_density_trilinear(const vec3& ipos) const {
    const vec3 f = fract(ipos - .5f);
    const ivec3 iipos = ivec3(floor(ipos - .5f));
    const float lx0 = mix(lookup_density(iipos + ivec3(0, 0, 0)), lookup_density(iipos + ivec3(1, 0, 0)), f.x);
    const float lx1 = mix(lookup_density(iipos + ivec3(0, 1, 0)), lookup_density(iipos + ivec3(1, 1, 0)), f.x);
    const float hx0 = mix(lookup_density(iipos + ivec3(0, 0, 1)), lookup_density(iipos + ivec3(1, 0, 1)), f.x);
    const float hx1 = mix(lookup_density(iipos + ivec3(0, 1, 1)), lookup_density(iipos + ivec3(1, 1, 1)), f.x);
    return mix(mix(lx0, lx1, f.y), mix(hx0, hx1, f.y), f.z);
}

float NVDBGrid::lookup_density_stochastic(const vec3& ipos) const {
    return lookup_density(uvec3(ipos + RNG::uniform<vec3>() - .5f));
}

// -----------------------------------------------
// Volume

Volume::Volume(const std::filesystem::path& path, float density_scale) :
    grid(path, density_scale),
    model(1),
    scattering_cross_section(0.001),
    absorption_cross_section(0.001),
    phase_g(0),
    raymarch_dt(0.1) {}

Volume::~Volume() {}

vec3 Volume::world_to_index(const vec4& wpos) const {
    return vec3(inverse(model * grid.transform) * wpos);
}

vec3 Volume::index_to_world(const vec4& ipos) const {
    return vec3(model * grid.transform * ipos);
}

float Volume::albedo() const {
    return scattering_cross_section / (absorption_cross_section + scattering_cross_section);
}

float Volume::extinction_cross_section() const {
    return absorption_cross_section + scattering_cross_section;
}

std::tuple<vec3, vec3> Volume::compute_AABB() const {
    return { index_to_world(vec4(grid.ibb_min, 1.f)), index_to_world(vec4(grid.ibb_max, 1.f)) };
}

std::tuple<bool, float, float> Volume::intersect(const Ray& ray) const {
    const auto [aabb_min, aabb_max] = compute_AABB();
    const vec3 inv_dir = 1.f / ray.dir;
    const vec3 lo = (aabb_min - ray.org) * inv_dir;
    const vec3 hi = (aabb_max - ray.org) * inv_dir;
    const vec3 tmin = min(lo, hi), tmax = max(lo, hi);
    const float near = fmaxf(ray.tnear, fmaxf(tmin.x, fmaxf(tmin.y, tmin.z)));
    const float far = fminf(ray.tfar, fminf(tmax.x, fminf(tmax.y, tmax.z)));
    return { near < far && std::isfinite(near) && far != FLT_MAX, near, far };
}

float Volume::transmittance_raymarching(const Ray& ray) const {
    // clip ray against volume AABB
    const auto [hit, near, far] = intersect(ray);
    if (!hit) return 1.f;
    // transform ray to index-space
    const vec3 ipos = world_to_index(vec4(ray.org, 1.f));
    const vec3 idir = world_to_index(vec4(ray.dir, 0.f)); // non-normalized!

    // TODO: ASSIGNMENT5: implement transmittance estimation using ray marching in the ray segment [near, far]
    // Hint: see the class members raymarch_dt, absorption_cross_section and scattering_cross_section
    // Hint: use NVDBGrid::lookup_density*() to fetch density values from the index-space grid
    // Hint: randomly offset the start position using RNG::uniform<float>() to avoid banding artifacts

    return 1.f;
}

float Volume::transmittance_ratio_tracking(const Ray& ray) const {
    // clip ray against volume AABB
    const auto [hit, near, far] = intersect(ray);
    if (!hit) return 1.f;
    // transform ray to index-space
    const vec3 ipos = world_to_index(vec4(ray.org, 1.f));
    const vec3 idir = world_to_index(vec4(ray.dir, 0.f)); // non-normalized!

    // TODO: ASSIGNMENT5: implement transmittance estimation using ratio tracking in the ray segment [near, far]
    // Hint: see the class members absorption_cross_section and scattering_cross_section
    // Hint: use NVDBGrid::max_value() to fetch the maximum density value in volume

    return 1.f;
}

std::tuple<bool, float> Volume::sample_raymarching(const Ray& ray) const {
    // clip ray against volume AABB
    const auto [hit, near, far] = intersect(ray);
    if (!hit) return { false, ray.tnear };
    // transform ray to index-space
    const vec3 ipos = world_to_index(vec4(ray.org, 1.f));
    const vec3 idir = world_to_index(vec4(ray.dir, 0.f)); // non-normalized!

    // TODO: ASSIGNMENT5: implement volume sampling using ray marching in the ray segment [near, far]
    // Hint: see the class members raymarch_dt, absorption_cross_section and scattering_cross_section
    // Hint: use NVDBGrid::lookup_density*() to fetch density values from the index-space grid
    // Hint: randomly offset the start position using RNG::uniform<float>() to avoid banding artifacts

    return { false, ray.tfar };
}

std::tuple<bool, float> Volume::sample_delta_tracking(const Ray& ray) const {
    // clip ray against volume AABB
    const auto [hit, near, far] = intersect(ray);
    if (!hit) return { false, ray.tfar };
    // transform ray to index-space
    const vec3 ipos = world_to_index(vec4(ray.org, 1.f));
    const vec3 idir = world_to_index(vec4(ray.dir, 0.f)); // non-normalized!

    // TODO: ASSIGNMENT5: implement volume sampling using delta tracking in the ray segment [near, far]
    // Hint: see the class members absorption_cross_section and scattering_cross_section
    // Hint: use NVDBGrid::lookup_density*() to fetch density values from the index-space grid
    // Hint: use NVDBGrid::max_value() to fetch the maximum density value in volume

    return { false, ray.tfar };
}

float Volume::transmittance(const Ray& ray) const {
    if (unbiased_estimators)
        return transmittance_ratio_tracking(ray);
    else
        return transmittance_raymarching(ray);
}

std::tuple<bool, float> Volume::sample(const Ray& ray) const {
    if (unbiased_estimators)
        return sample_delta_tracking(ray);
    else
        return sample_raymarching(ray);
}

json11::Json Volume::to_json() const {
    return json11::Json::object {
        { "density_scale", grid.density_scale },
        { "modelmatrix", json11::Json::array{
            json11::Json::array{model[0][0], model[0][1], model[0][2], model[0][3]},
            json11::Json::array{model[1][0], model[1][1], model[1][2], model[1][3]},
            json11::Json::array{model[2][0], model[2][1], model[2][2], model[2][3]},
            json11::Json::array{model[3][0], model[3][1], model[3][2], model[3][3]}}},
        { "absorption_cross_section", absorption_cross_section },
        { "scattering_cross_section", scattering_cross_section },
        { "phase_g", phase_g },
        { "unbiased_estimators", unbiased_estimators },
        { "raymarch_dt", raymarch_dt }
    };
}

void Volume::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        json_set_float(cfg, "density_scale", grid.density_scale);
        if (cfg["modelmatrix"].is_array() && cfg["modelmatrix"].array_items().size() >= 4) {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    model[i][j] = cfg["modelmatrix"][i][j].number_value();
        }
        json_set_float(cfg, "absorption_cross_section", absorption_cross_section);
        json_set_float(cfg, "scattering_cross_section", scattering_cross_section);
        json_set_float(cfg, "phase_g", phase_g);
        json_set_bool(cfg, "unbiased_estimators", unbiased_estimators);
        json_set_float(cfg, "raymarch_dt", raymarch_dt);
    }
}
