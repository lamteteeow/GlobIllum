#pragma once
#include <tuple>
#include <cstdint>
#include <embree4/rtcore.h>
#include <vector>
#include <memory>

#include "par_shapes.h"
#include "material.h"
#include "distribution.h"
#include "light.h"

class aiMesh;
class AreaLight;
class SurfaceHit;

class Mesh {
public:
    Mesh(RTCDevice& device, RTCScene& scene, const std::shared_ptr<Material>& mat, const aiMesh* ai_mesh);
    Mesh(RTCDevice& device, RTCScene& scene, const std::shared_ptr<Material>& mat, const par_shapes_mesh_s* par_mesh);
    ~Mesh();

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    inline size_t num_vertices() const { return vbo.size(); }

    inline size_t num_triangles() const { return ibo.size(); }

    inline float surface_area() const { assert(area_distribution); return area_distribution->integral(); }

    inline bool is_light() const { assert(mat); return mat->emissive_strength > 0.f; }

    /**
     * @brief Importance sample a triangle of the mesh according to the relative area
     *
     * @param sample Random sample in [0, 1)
     * @param pdf PDF of returned sample
     *
     * @return Sampled differential geometry on mesh surface
     */
    std::tuple<SurfaceHit, float> sample(const glm::vec2& sample) const;

    // data
    RTCGeometry geom;                                   ///< Embree geometry
    uint32_t geomID;                                    ///< Embree geometry ID
    std::vector<glm::vec3> vbo;                         ///< Vertex buffer
    std::vector<glm::uvec3> ibo;                        ///< Index buffer
    std::vector<glm::vec3> normals;                     ///< Normals buffer
    std::vector<glm::vec2> tcs;                         ///< Texture coor buffer
    std::shared_ptr<Material> mat;                      ///< Pointer to material
    std::shared_ptr<Distribution1D> area_distribution;  ///< Area distribution of triangles for importance sampling
    glm::vec3 bb_min;                                   ///< AABB (lower left corner)
    glm::vec3 bb_max;                                   ///< AABB (upper right corner)
    glm::vec3 center;                                   ///< Center point of disk approximation
    float radius;                                       ///< Radius of disk approximation
    RTCScene& scene;                                    ///< Embree scene
    std::shared_ptr<AreaLight> area_light;              ///< Area light pointer for handling direct light source hits
};
