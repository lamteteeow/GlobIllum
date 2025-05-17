#pragma once

#include "hit.h"
#include "random.h"
#include "glm/glm.hpp"
#include <vector>

// -------------------------------------------------------------------------
// Forward declarations

class Context;
class PathVertex;
struct RandomWalkCam;
struct RandomWalkLight;

// -------------------------------------------------------------------------
// Function declarations

// trace path from camera
void trace_cam_path(const Context& context, uint32_t x, uint32_t y, std::vector<PathVertex>& cam_path, RandomWalkCam& walk,
                    uint32_t max_path_len, uint32_t rr_min_path_len, float rr_threshold, bool specular_path_tracing = false);

// trace path from light source
void trace_light_path(const Context& context, std::vector<PathVertex>& light_path, RandomWalkLight& walk, 
                    uint32_t max_path_len, uint32_t rr_min_path_len, float rr_threshold);

// connect camera and light paths
glm::vec3 connect_and_shade(const Context& context, const std::vector<PathVertex>& cam_path, const std::vector<PathVertex>& light_path);

// collect N global photons
void trace_photons(const Context& context, int N, std::vector<PathVertex>& photons, bool scale_photon_power = true);

// -------------------------------------------------------------------------
// Struct definitions

class PathVertex {
public:
    // default construct as invalid
    PathVertex()
        : hit(), w_o(0), throughput(0), on_light(false), infinite(false), escaped(false) {
    }

    // construct as vertex along path
    PathVertex(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& throughput)
        : hit(hit), w_o(w_o), throughput(throughput), on_light(false), infinite(false), escaped(false) {
    }

    // construct as vertex on light source
    PathVertex(const SurfaceHit& hit, const glm::vec3& throughput, bool infinite = false)
        : hit(hit), w_o(0), throughput(throughput), on_light(true), infinite(infinite), escaped(false) {
    }

    // construct as skylight contribution from escaped camera ray
    PathVertex(const glm::vec3& skylight_contrib)
        : w_o(glm::vec3(0)), throughput(skylight_contrib), on_light(false), infinite(true), escaped(true) {
    }

    // copy construct with scaling factor
    PathVertex(const PathVertex& v, float scale)
        : hit(v.hit), w_o(v.w_o), throughput(v.throughput * scale), on_light(v.on_light), infinite(v.infinite), escaped(v.escaped) {
    }

    // data
    const SurfaceHit hit;   ///< Surface interaction for this vertex along path
    const glm::vec3 w_o;    ///< Direction vector to previous vertex (0 if on light source)
    glm::vec3 throughput;   ///< Path throughput up to this vertex
    const bool on_light;    ///< if true, vertex lies directly on a light source
    const bool infinite;    ///< if true and is light vertex, light is infinitely far away (skylight)
    const bool escaped;     ///< if true, throughput should be interpreted as skylight contribution
};

struct RandomWalkCam {
    // samplers
    HammersleySampler2D pixel_sampler;
    LDSampler2D lens_sampler;
    UniformSampler2D bounce_sampler;
    UniformSampler1D rr_sampler;

    // call before use
    void init(uint32_t N) {
        pixel_sampler.init(N);
        lens_sampler.init(N);
        bounce_sampler.init(N);
        rr_sampler.init(N);
    }
};

struct RandomWalkLight {
    StratifiedSampler1D light_sampler;
    LDSampler2D Le_sampler;
    LDSampler2D dir_sampler;
    UniformSampler2D bounce_sampler;
    UniformSampler1D rr_sampler;

    // call before use
    void init(uint32_t N) {
        light_sampler.init(N);
        Le_sampler.init(N);
        dir_sampler.init(N);
        bounce_sampler.init(N);
        rr_sampler.init(N);
    }
};
