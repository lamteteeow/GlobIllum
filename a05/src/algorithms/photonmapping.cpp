#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/bdpt.h"
#include <nanoflann.hpp>

using namespace glm;

// -------------------------------------
// Photon map using a kd-tree

struct PhotonMap {
    // kdtree accessors
	inline size_t kdtree_get_point_count() const { return photons.size(); }
	inline float kdtree_get_pt(const size_t idx, const size_t dim) const { return photons[idx].hit.P[dim]; }
	template <class BBOX> inline bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

    // kdtree clear
    inline void clear() {
        photons.clear();
        kd_tree.reset();
    }

    // kdtree build
    inline void build() {
        assert(!photons.empty());
        kd_tree = std::make_shared<kd_tree_t>(3, *this);
        kd_tree->buildIndex();
    }

    /**
     * @brief K nearest neighbour lookup
     *
     * @param pos Query position
     * @param K Maximum number of nearest neighbors to look for
     * @param indices std::vector to be filled inidices into photons vector
     * @param distances std::vector to be filled with SQUARED distances to photons
     *
     * @return SQUARED distance to furthest away element
     */
    inline float knn_lookup(const glm::vec3& pos, size_t K, std::vector<size_t>& indices, std::vector<float>& distances) const {
        assert(!photons.empty());
        indices.resize(K); distances.resize(K);
		const size_t n_photons = kd_tree->knnSearch(&pos[0], K, &indices[0], &distances[0]);
        indices.resize(n_photons); distances.resize(n_photons);
        return n_photons > 0 ? distances[n_photons - 1] : 0.f;
    }

    // data
    using kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<float, PhotonMap>, PhotonMap, 3, size_t>;
    std::vector<PathVertex> photons;
    std::shared_ptr<kd_tree_t> kd_tree;
};

// -------------------------------------
// Photon mapping helper functions

// shade point on camera path (direct illum)
vec3 direct_illum(Context& context, const SurfaceHit& hit, const vec3& w_o) {
    vec3 L(0);
    const auto [light, light_source_pdf] = context.scene.sample_light_source(RNG::uniform<float>());
    auto [Li, shadow_ray, light_sample_pdf] = light->sample_Li(hit.P, RNG::uniform<vec2>());
    const float pdf = light_source_pdf * light_sample_pdf;
    if (pdf > 0.f && !context.scene.occluded(shadow_ray)) {
        const float cosTheta = fmaxf(0.f, dot(hit.N, shadow_ray.dir));
        L += Li * hit.f(w_o, shadow_ray.dir) * cosTheta / pdf;
    }
    return L;
}

// get radiance estimate at given point from given photon map
vec3 radiance_estimate(Context& context, const SurfaceHit& hit, const vec3& w_o, const PhotonMap& photon_map, size_t n_photons) {
    const float k = 1.0f;           // cone filter parameter (must be >= 1)
    std::vector<size_t> indices;    // indices into photon_map.photons
    std::vector<float> dist_sqr;    // SQUARED distances between photon and hit.P
    const float radius = sqrtf(photon_map.knn_lookup(hit.P, n_photons, indices, dist_sqr));
    vec3 L(0);
    // compute cone filtered radiance estimate
    for (size_t i = 0; i < indices.size(); ++i) {
        const PathVertex& photon = photon_map.photons[indices[i]];
        if (dot(photon.w_o, hit.N) > 0) {
            const float w = fmaxf(0.f, 1 - sqrtf(dist_sqr[i]) / (k * radius));
            L += w * photon.throughput * hit.f(w_o, photon.w_o);
        }
    }
    // normalize
    L *= vec3(1.f / ((1 - 2 / (3 * k)) * M_PI * radius * radius));
    return L;
}

// perform final gathering to properly capture smooth indirect illum
vec3 final_gather(Context& context, const SurfaceHit& hit, const vec3& w_o, const PhotonMap& photon_map) {
    vec3 L(0);
    // trace a secondary bounce
    const auto [brdf, w_i, pdf] = hit.sample(w_o, RNG::uniform<vec2>());
    if (pdf <= 0.f) return L;
    Ray ray = Ray(hit.P, w_i);
    const SurfaceHit& secondary = context.scene.intersect(ray);
    if (secondary.valid) {
        const float cosTheta = fmaxf(0.f, dot(hit.N, w_i));
        const vec3 Li = radiance_estimate(context, secondary, -w_i, photon_map, 25);
        L += Li * brdf * cosTheta / pdf;
    }
    return L;
}

// -------------------------------------
// Photon mapping algorithm

struct PhotonMapping : public Algorithm {
    inline static const std::string name = "PhotonMapping";

    // Photon mapping parameters: trade quality for performance here
    const uint32_t NUM_PHOTON_PATHS = 1 << 18;
    const bool DIRECT_VISUALIZATION = false;

    // called once before each(!) rendering
    void init(Context& context) {
        if (photon_map.photons.empty()) {
            // trace photons and build kd-tree
            std::cout << "Tracing photons..." << std::endl;
            trace_photons(context, NUM_PHOTON_PATHS, photon_map.photons);
            photon_map.build();
            std::cout << "Num photons: " << photon_map.photons.size() << std::endl;
        }
    }

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        samples = std::max(1u, samples / 2); // adjust sample count for final gathering
        // init
        RandomWalkCam cam_walk;
        cam_walk.init(samples);
        std::vector<PathVertex> cam_path;
        // trace
        for (uint32_t s = 0; s < samples; ++s) {
            // construct camera "path" by tracing specular bounces only
            trace_cam_path(context, x, y, cam_path, cam_walk, context.MAX_CAM_PATH_LENGTH, context.RR_MIN_PATH_LENGTH, context.RR_THRESHOLD, true);
            // shade
            vec3 L(0);
            if (!cam_path.empty()) {
                const PathVertex& vertex = cam_path[cam_path.size() - 1];
                if (vertex.escaped || vertex.on_light)
                    L += vertex.throughput;
                else {
                    if (DIRECT_VISUALIZATION) {
                        L += vertex.throughput * radiance_estimate(context, vertex.hit, vertex.w_o, photon_map, 500);
                        s = samples; // "break"
                    } else {
                        // compute direct illumumination
                        L += direct_illum(context, vertex.hit, vertex.w_o);
                        // query indirect illumination from photon map
                        L += final_gather(context, vertex.hit, vertex.w_o, photon_map);
                        // apply throughput and pdf
                        L *= vertex.throughput;
                    }
                }
            }
            context.fbo.add_sample(x, y, L);
        }
    }

    // data
    PhotonMap photon_map;
};

static AlgorithmRegistrar<PhotonMapping> registrar;
