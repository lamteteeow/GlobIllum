#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "brdf.h"
#include "texture.h"
#include "json11.h"

class aiMaterial;

/**
 * @brief Material class, describing a surface
 */
class Material {
public:
    Material();
    Material(const aiMaterial *material_ai, const std::filesystem::path& base_path);
    virtual ~Material();

    // material lookups (texture or static)
    glm::vec3 albedo(const glm::vec2& TC) const;
    float roughness(const glm::vec2& TC) const;
    glm::vec3 emissive(const glm::vec2& TC) const;
    glm::vec3 normalmap(const glm::vec3& N, const glm::vec2& TC) const;
    float alphamap(const glm::vec2& TC) const;

    // (lossy) translatation between phong exponent and roughness
    inline static float roughness_from_exponent(float exponent) {
        return sqrtf(2.f / (exponent + 2.f));
    }
    inline static float exponent_from_roughness(float roughness) {
        return 2 / (roughness * roughness) - 2;
    }

    // some material presets
    void set_to(const std::string& type);
    void set_light();
    void set_diffuse();
    void set_specular();
    void set_phong();
    void set_microfacet();
    void set_translucent();
    void set_glass();
    void set_water();
    void set_metal();
    void set_gold();
    void set_silver();
    void set_copper();
    void set_layered_ggx();
    void set_default();

    // JSON import/export
    json11::Json to_json() const;
    void from_json(const json11::Json& cfg);

    // data
    std::string name;                       ///< Material name string
    std::string type;                       ///< Material type string
    std::unique_ptr<BRDF> brdf;             ///< BRDF, describing surface properties
    // BRDF parameters:                        [min, max]
    float ior = 1.3f;                       ///< [1, 3] material index of refraction
    float roughness_val = 0.1;              ///< [0, 1] specular (microfacet) roughness
    bool coated = false;                    ///< [false, true] coated microfacet BRDF?
    float absorb = 0;                       ///< [0, 3] absorbtion rate of conductor materials
    glm::vec3 albedo_col  = glm::vec3(1);   ///< Base albedo (if no texture present)
    float emissive_strength = 0;            ///< Strength of light emission, is light source if > 0
    // textures
    Texture albedo_tex;                     ///< Albedo texture
    Texture normal_tex;                     ///< Normal map texture
    Texture alpha_tex;                      ///< Alpha map texture
    Texture roughness_tex;                  ///< Roughness map texture
    Texture emissive_tex;                   ///< Emissive texture

public:
    static std::vector<Material*> instances; ///< global vector of all materials (for UI)
};
