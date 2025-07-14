#pragma once

#include <cmath>
#include <tuple>

#include "ray.h"
#include "mesh.h"
#include "light.h"
#include "volume.h"
#include "material.h"
#include "sampling.h"

// ---------------------------------------------------------
// The Hit interface provides an abstraction layer over surface and volume interactions.

class Hit {
public:
    Hit(const bool valid = false) : valid(valid) {}
    virtual ~Hit() {}

    inline explicit operator bool() { return valid; }

    /**
     * @brief Evaluate the BRDF or phase function of this interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return BRDF or phase function value
     */
    virtual glm::vec3 f(const glm::vec3& w_o, const glm::vec3& w_i) const = 0;

    /**
     * @brief Sample the BRDF or phase function of this interaction
     *
     * @param w_o World space out direction
     * @param sample Random sample used for sampling the BRDF
     *
     * @return BRDF/phase function value of sampled direction
     * @return Sampled direction in world space (w_i)
     * @return PDF value of sampled direction
     */
    virtual std::tuple<glm::vec3, glm::vec3, float> sample(const glm::vec3& w_o, const glm::vec2& sample) const = 0;

    /**
     * @brief Evaluate the pdf of this interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return pdf value
     */
    virtual float pdf(const glm::vec3& w_o, const glm::vec3& w_i) const = 0;

    // data
    const bool valid;   // Use this to check for a valid surface of volume interaction
};

// ---------------------------------------------------------
// The SurfaceHit class provides an abstraction layer over surface interactions and materials.

class SurfaceHit : public Hit {
public:
    // Default construct as invalid surface interaction with an optional sky light contribution on miss
    SurfaceHit(const SkyLight* sky = 0);

    // Construct as ray surface interaction and perform hit point interpolation
    SurfaceHit(const Ray& ray, const Mesh* mesh);

    // Construct as mesh sample, for example when sampling a mesh light source
    SurfaceHit(const glm::vec2& sample, uint32_t primID, const Mesh* mesh);

    // Construct as simple point without mesh or material, e.g. a camera sample for BDPT
    SurfaceHit(const glm::vec3& position, const glm::vec3& normal);

    /**
     * @brief Evaluate the BRDF or phase function of this interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return BRDF value
     */
    glm::vec3 f(const glm::vec3& w_o, const glm::vec3& w_i) const;

    /**
     * @brief Sample the BRDF or phase function of this interaction
     *
     * @param w_o World space out direction
     * @param sample Random sample used for sampling the BRDF
     *
     * @return BRDF/phase function value of sampled direction
     * @return Sampled direction in world space (w_i)
     * @return PDF value of sampled direction
     */
    std::tuple<glm::vec3, glm::vec3, float> sample(const glm::vec3& w_o, const glm::vec2& sample) const;

    /**
     * @brief Evaluate the pdf of this interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return pdf value
     */
    float pdf(const glm::vec3& w_o, const glm::vec3& w_i) const;

    // Compute surface color (albedo)
    inline glm::vec3 albedo() const {
        assert(mat);
        return mat->albedo(TC);
    }

    // Compute surface roughness
    inline float roughness() const {
        assert(mat);
        return mat->roughness(TC);
    }

    // Check whether hit is light source
    inline bool is_light() const { return light != 0; }

    // Query emitted light of emissive material
    inline glm::vec3 Le() const {
        assert(mat);
        return mat->emissive(TC);
    }

    // Check type of surface interaction (based on underlying BRDF type)
    inline bool is_type(const BRDFType& type) const {
        assert(mat);
        return mat->brdf->is_type(type);
    }

    // Convert direction from world to tangent space at this surface interaction
    inline glm::vec3 to_tangent(const glm::vec3& world_dir) const {
        assert(valid);
        return world_to_tangent(N, world_dir);
    }

    // Convert direction from tangent to world space at this surface interaction
    inline glm::vec3 to_world(const glm::vec3& tangent_dir) const     {
        assert(valid);
        return tangent_to_world(N, tangent_dir);
    }

    // data
    // const bool valid;       ///< Use this to check for a valid surface of volume interaction
    glm::vec3 P;            ///< World space position
    glm::vec3 Ng;           ///< World space geometry normal
    glm::vec3 N;            ///< World space shading normal (including normalmapping)
    glm::vec2 TC;           ///< Texture coordinates, or glm::vec2(0) if none available
    float area;             ///< Hit primitive surface area
    const Mesh* mesh;       ///< Mesh pointer, may be 0 for abstract surfaces
    const Material* mat;    ///< Material pointer, may be 0 for abstract surfaces
    const Light* light;     ///< Light source pointer, set if a light source was hit (type is: valid ? AreaLight : SkyLight)
};

// ---------------------------------------------------------
// The VolumeHit class provides an abstraction layer over volume interactions and materials.

class VolumeHit : public Hit {
public:
    VolumeHit();
    VolumeHit(const glm::vec3& position, const Volume* volume);

    /**
     * @brief Evaluate the BRDF or phase function of this interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return Phase function value
     */
    glm::vec3 f(const glm::vec3& w_o, const glm::vec3& w_i) const;

    /**
     * @brief Sample the BRDF or phase function of this interaction
     *
     * @param w_o World space out direction
     * @param sample Random sample used for sampling the BRDF
     *
     * @return BRDF/phase function value of sampled direction
     * @return Sampled direction in world space (w_i)
     * @return PDF value of sampled direction
     */
    std::tuple<glm::vec3, glm::vec3, float> sample(const glm::vec3& w_o, const glm::vec2& sample) const;

    /**
     * @brief Evaluate the pdf of this interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return pdf value
     */
    float pdf(const glm::vec3& w_o, const glm::vec3& w_i) const;

    // data
    glm::vec3 P;
    const Volume* volume;
};
