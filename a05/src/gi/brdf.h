#pragma once

#include <tuple>
#include <cstdint>
#include "glm/glm.hpp"

// ------------------------------------------------
// BRDF interface

class SurfaceHit;

/**
 * @brief Enum for BRDF types
 */
enum BRDFType {
    BRDF_DIFFUSE        = 1 << 0,
    BRDF_SPECULAR       = 1 << 1,
    BRDF_GLOSSY         = 1 << 2,
    BRDF_REFLECTION     = 1 << 3,
    BRDF_TRANSMISSION   = 1 << 4,
    BRDF_ALL            = 0xFFFFFFFF
};

/**
 * @brief BRDF class, representing all types of BRDFs used in this framework to describe a surface
 */
class BRDF {
public:
    /**
     * @brief Default conclass BRDF of given type
     *
     * @param type Type of this BRDF
     */
    BRDF(BRDFType type) : type(type) {}

    /**
     * @brief Destructor
     */
    virtual ~BRDF() {}

    /**
     * @brief Check BRDF type
     *
     * @param t BRDF type to check against
     *
     * @return true if BRDF flags include the given type, false otherwise
     */
    inline bool is_type(const BRDFType& t) const { return (type & t) > 0; }

    /**
     * @brief Evaluate the BRDF for given in and out directions
     *
     * @param hit Surface interaction
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return Color of evaluated BRDF
     */
    virtual glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const = 0;

    /**
     * @brief Sample a direction according to this BRDF, whilst also computing the PDF and evaluated BRDF
     *
     * @param hit Surface interaction
     * @param w_o World space out direction
     * @param sample Random sample used for drawing a direction
     *
     * @return Tuple consisting of:
     *      evaluated BRDF value (vec3)
     *      sampled outgoing direction in world-space (normalized vec3)
     *      pdf of sampled direction (float)
     */
    virtual std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const = 0;

    /**
     * @brief Evaluate the PDF for given in and out directions
     *
     * @param hit Surface interaction
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return PDF for given surface interaction and directions
     */
    virtual float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const = 0;

private:
    // data
    const BRDFType type; ///< BRDF type flags, see BRDFType enum
};

// ------------------------------------------------
// Implementations

/**
 * @brief Diffuse reflection BRDF
 */
class LambertianReflection : public BRDF {
public:
    LambertianReflection() : BRDF(BRDFType(BRDF_DIFFUSE | BRDF_REFLECTION)) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};

/**
 * @brief Diffuse transmission BRDF
 */
class LambertianTransmission : public BRDF {
public:
    LambertianTransmission() : BRDF(BRDFType(BRDF_DIFFUSE | BRDF_TRANSMISSION)) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};

/**
 * @brief Perfect specular reflection BRDF (delta distribution)
 */
class SpecularReflection : public BRDF {
public:
    SpecularReflection() : BRDF(BRDFType(BRDF_SPECULAR | BRDF_REFLECTION)) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};

/**
 * @brief Perfect specular transmission BRDF (delta distribution)
 */
class SpecularTransmission : public BRDF {
public:
    SpecularTransmission() : BRDF(BRDFType(BRDF_SPECULAR | BRDF_TRANSMISSION)) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};

/**
 * @brief BRDF combining a perfect specular relfection and transmission via the fresnel factor. F.e. used to simulate glass.
 */
class SpecularFresnel : public BRDF {
public:
    SpecularFresnel() : BRDF(BRDFType(BRDF_SPECULAR | BRDF_REFLECTION | BRDF_TRANSMISSION)) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};

/**
 * @brief Specular phong BRDF
 */
class SpecularPhong : public BRDF {
public:
    SpecularPhong() : BRDF(BRDFType(BRDF_GLOSSY | BRDF_REFLECTION)) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};

/**
 * @brief Microfacet reflection BRDF, currently using the GTR(1) or GGX distribution
 */
class MicrofacetReflection : public BRDF {
public:
    MicrofacetReflection(bool coated = false) : BRDF(BRDFType(BRDF_GLOSSY | BRDF_REFLECTION)), coated(coated) {}
    virtual glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    virtual float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
protected:
    bool coated;
};

/**
 * @brief Microfacet transmission BRDF, using the GTR(1) or GGX distribution
 */
class MicrofacetTransmission : public BRDF {
public:
    MicrofacetTransmission(bool coated = false) : BRDF(BRDFType(BRDF_GLOSSY | BRDF_TRANSMISSION)), coated(coated) {}
    virtual glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    virtual float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
protected:
    bool coated;
};

// -------------------------------------------------------------------------------------------
// Surfaces

/**
 * @brief BRDF resembling a layered surface, e.g. mixing a diffuse base layer and a specular coat.
 */
class LayeredSurface  : public BRDF {
public:
    LayeredSurface() : BRDF(BRDFType(BRDF_DIFFUSE | BRDF_GLOSSY | BRDF_REFLECTION)), diff(), spec(true) {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
    std::tuple<glm::vec3, glm::vec3, float> sample(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec2& sample) const;
    float pdf(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
protected:
    LambertianReflection diff;
    MicrofacetReflection spec;
};

/**
 * @brief BRDF resembling a metallic surface.
 * @note The only BRDF to use the absorb material parameter
 */
class MetallicSurface : public MicrofacetReflection {
public:
    MetallicSurface() : MicrofacetReflection() {}
    glm::vec3 f(const SurfaceHit& hit, const glm::vec3& w_o, const glm::vec3& w_i) const;
};
