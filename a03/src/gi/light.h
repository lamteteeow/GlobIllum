#pragma once

#include "json11.h"
#include "texture.h"
#include "distribution.h"
#include <tuple>
#include <string>
#include <memory>
#include <filesystem>

class Ray;
class Mesh;
class Scene;
class SurfaceHit;

/**
 * @brief General light source interface abstracting away the actual type of the light source
 */
class Light {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~Light() {}

    /**
     * @brief Compute incoming irradiance at point p_world, including shadow ray and pdf
     *
     * @param position Position to compute incident light for
     * @param sample Random samples in [0, 1) used to sample the light source
     *
     * @return Tuple consisting of:
     *      - Incoming irradiance at point p_world (vec3)
     *      - Shadow ray to sampled position on light source (Ray)
     *      - PDF of light source sample (float)
     */
    virtual std::tuple<glm::vec3, Ray, float> sample_Li(const glm::vec3& position, const glm::vec2& sample) const = 0;

    /**
     * @brief Compute PDF for a given ray pointing to a light source sample
     *
     * @param light Surface interaction on light source to compute PDF for
     * @param ray Ray pointing to light source sample
     *
     * @return PDF for light source sample in terms of solid angle
     */
    virtual float pdf_Li(const SurfaceHit& light, const Ray& ray) const = 0;

    /**
     * @brief Sample outgoing irradiance from a light source (i.e. emit a light ray from the light source)
     *
     * @param sample_pos Random samples in [0, 1) used for sampling a point on the light source
     * @param sample_dir Random samples in [0, 1) used for sampling an outgoing direction
     *
     * @return Tuple consisting of:
     *      - Exit irradiance from sampled position in sampled direction (vec3)
     *      - Sampled outgoing ray from the light source (Ray)
     *      - Shading normal of the sampled point on the light source (vec3)
     *      - PDF of sampled position on light source (float)
     *      - PDF of sampled direction leaving the light source (float)
     */
    virtual std::tuple<glm::vec3, Ray, glm::vec3, float, float> sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const = 0;

    virtual std::tuple<float, float> pdf_Le(const SurfaceHit& light, const glm::vec3& dir) const = 0; // currently unused

    /**
     * @brief Compute irradiance for escaped ray
     *
     * @param ray Escaped ray
     *
     * @return Irradiance for escaped ray
     */
    virtual glm::vec3 Le(const Ray& ray) const = 0;

    /**
     * @brief Total power/intensity of the light source
     *
     * Used for importance sampling in many light source settings.
     *
     * @return Power of the light source
     */
    virtual glm::vec3 power() const = 0;

    /**
     * @brief Check if this light source if infinitely far away (f.e. DirectionalLight)
     *
     * @return true if light source is infinitely far away, false otherwise
     */
    virtual bool is_infinite() const = 0;

protected:
    friend class Scene;
    friend class json11::Json;

    virtual json11::Json to_json() const = 0;
    virtual void from_json(const json11::Json& cfg) = 0;
};

/**
 * @brief Area light source defined via Mesh
 */
class AreaLight : public Light {
public:
    AreaLight(const Mesh& mesh);

    std::tuple<glm::vec3, Ray, float> sample_Li(const glm::vec3& position, const glm::vec2& sample) const;
    float pdf_Li(const SurfaceHit& light, const Ray& ray) const;

    std::tuple<glm::vec3, Ray, glm::vec3, float, float> sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const;
    std::tuple<float, float> pdf_Le(const SurfaceHit& light, const glm::vec3& dir) const;

    glm::vec3 Le(const Ray& ray) const { return glm::vec3(0); }
    glm::vec3 power() const;
    bool is_infinite() const { return false; }

    // empty json import/export since this is implicitly built
    json11::Json to_json() const { return json11::Json(); }
    void from_json(const json11::Json& cfg) {}

    // data
    const Mesh& mesh;               ///< mesh representing the light source
};

/**
 * @brief Infinitesimally far away environmental light source (i.e. sky)
 */
class SkyLight : public Light {
public:
    SkyLight();
    SkyLight(const std::filesystem::path& path, const Scene& scene, float intensity = 1.f);

    void load(const std::filesystem::path& path, const glm::vec3& scene_center, float scene_radius, float intensity = 1.f);
    void build_distribution(); // compute intensity distribution for importance sampling during rendering

    std::tuple<glm::vec3, Ray, float> sample_Li(const glm::vec3& position, const glm::vec2& sample) const;
    float pdf_Li(const SurfaceHit& light, const Ray& ray) const;

    std::tuple<glm::vec3, Ray, glm::vec3, float, float> sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const;
    std::tuple<float, float> pdf_Le(const SurfaceHit& light, const glm::vec3& dir) const;

    glm::vec3 Le(const Ray& ray) const;
    glm::vec3 power() const;
    inline bool is_infinite() const { return true; }

    json11::Json to_json() const;
    void from_json(const json11::Json& cfg);

    // data
    std::shared_ptr<Texture> texture;               ///< spherical environment map texture
    float intensity;                                ///< scalar light source intensity
    std::shared_ptr<Distribution2D> distribution;   ///< distribution for importance sampling
    glm::vec3 scene_center;                         ///< Center of disk approximation of scene
    float scene_radius;                             ///< Radius of disk approximation of scene
};
