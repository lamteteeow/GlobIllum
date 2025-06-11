#pragma once
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

#include <embree4/rtcore.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>

#include "json11.h"
#include "par_shapes.h"
#include "hit.h"
#include "volume.h"

class Ray;
class Mesh;
class Material;
class Light;
class SkyLight;
class Distribution1D;

/**
 * @brief Scene class containing all meshes, light sources and materials in the scene
 * and providing an interface for intersection and occlusion tests
 */
class Scene {
public:
    Scene(RTCDevice& device);
    virtual ~Scene();

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;


    void load_mesh(const std::filesystem::path& path);
    void load_sky(const std::filesystem::path& path);
    void load_volume(const std::filesystem::path& path);

    void add(const par_shapes_mesh* par_mesh, const std::shared_ptr<Material>& mat);

    /**
     * @brief Clear all contents of the scene
     */
    void clear();

    /**
     * @brief Commit scene and prepare for rendering
     * @note This function has to be called in between adding or removing meshes or light sources
     * and performing intersection or occlusion tests or sampling a light source!
     * @note This will be called once before rendering from the driver module.
     */
    void commit();

    const SurfaceHit intersect(Ray& ray) const;
    const VolumeHit intersect_volume(Ray& ray) const;

    bool occluded(Ray& ray) const;          // only check for opaque geometry
    float transmittance(Ray& ray) const;    // only check for volumetric occlusion
    float visibility(Ray& ray) const;       // both opaque and volumetric occlusion

    /**
     * @brief Sample a light source accoring to their respective intensities
     * @note Assumes Scene::commit() has been called previously.
     *
     * @param sample Random sample in [0, 1]
     *
     * @return Tuple of pointer to sampled light source and PDF
     */
    std::tuple<std::shared_ptr<Light>, float> sample_light_source(float sample) const;

    /**
     * @brief Query PDF for given light source
     *
     * @param light Light to query PDF for
     * 
     * @return PDF for selecting this light source
     */
    float light_source_pdf(const Light* light) const;

    inline bool has_sky() const { return sky.operator bool(); }
    inline glm::vec3 Le(const Ray& ray) const { return has_sky() ? sky->Le(ray) : glm::vec3(0.f); }

    /**
     * @brief Query the total power of all light sources present in the scene
     *
     * @return Total light power in the scene
     */
    float total_light_source_power() const;

    /**
     * @brief Translate a geometry ID into a mesh pointer, e.g. Ray::geomID
     *
     * @param geomID Geometry ID to translate
     *
     * @return Mesh pointer or nullptr if not found
     */
    Mesh* get_mesh(uint32_t geomID) const;

private:
    friend class Context;
    friend class json11::Json;

    json11::Json to_json() const;
    void from_json(const json11::Json& cfg);

public:
    // data
    RTCScene scene;                                     ///< Embree4 scene
    RTCDevice& device;                                  ///< Embree4 device
    Assimp::Importer importer;                          ///< Assimp importer
    Assimp::Exporter exporter;                          ///< Assimp exporter
    std::vector<std::filesystem::path> mesh_files;      ///< File paths of present meshes
    std::vector<std::shared_ptr<Mesh>> meshes;          ///< All present meshes
    std::vector<std::shared_ptr<Material>> materials;   ///< All present materials
    std::shared_ptr<SkyLight> sky;                      ///< Sky light (if present)
    std::vector<std::shared_ptr<Light>> lights;         ///< All light source currently in the scene
    std::shared_ptr<Distribution1D> light_distribution; ///< For importance sampling light sources
    std::shared_ptr<Volume> volume;                     ///< Volume (if present)
    std::filesystem::path volume_path;                  ///< File path to current volume (if present)
    glm::vec3 bb_min;                                   ///< AABB (lower left corner)
    glm::vec3 bb_max;                                   ///< AABB (upper right corner)
    glm::vec3 center;                                   ///< Center point of disk approximation
    float radius;                                       ///< Radius of disk approximation
};
