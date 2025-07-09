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
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// get radiance estimate at given point from given photon map
vec3 radiance_estimate(Context& context, const SurfaceHit& hit, const vec3& w_o, const PhotonMap& photon_map, size_t n_photons) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// perform final gathering to properly capture smooth indirect illum
vec3 final_gather(Context& context, const SurfaceHit& hit, const vec3& w_o, const PhotonMap& photon_map) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
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
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }

    // data
    PhotonMap photon_map;
};

static AlgorithmRegistrar<PhotonMapping> registrar;
