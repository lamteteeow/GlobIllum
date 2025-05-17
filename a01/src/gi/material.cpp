#include "material.h"
#include "sampling.h"
#include "light.h"
#include "color.h"

#include <mutex>

#include <assimp/material.h>

static std::mutex mat_mutex;
std::vector<Material *> Material::instances;

Material::Material() : name("default"), type(name) {}

Material::Material(const aiMaterial *material_ai, const std::filesystem::path& base_path) {
    aiColor3D diff, spec, emis;
    material_ai->Get(AI_MATKEY_COLOR_DIFFUSE, diff);
    material_ai->Get(AI_MATKEY_COLOR_SPECULAR, spec);
    material_ai->Get(AI_MATKEY_COLOR_EMISSIVE, emis);
    albedo_col = glm::vec3(diff.r, diff.g, diff.b);
    if (luma(albedo_col) < 1e-4)
        albedo_col = glm::vec3(spec.r, spec.g, spec.b);
    emissive_strength = luma(glm::vec3(emis.r, emis.g, emis.b));

    // fetch material ior
    material_ai->Get(AI_MATKEY_REFRACTI, ior);
    // convert exponent to roughness
    float exponent;
    material_ai->Get(AI_MATKEY_SHININESS, exponent);
    roughness_val = roughness_from_exponent(exponent);

    // fetch name
    aiString name_ai;
    material_ai->Get(AI_MATKEY_NAME, name_ai);
    name = name_ai.C_Str();
    {
        // add material to global instance map
        std::lock_guard<std::mutex> guard(mat_mutex);
        instances.push_back(this);
    }

    // fetch diffuse (albedo) tex
    if (material_ai->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString path_ai;
        material_ai->GetTexture(aiTextureType_DIFFUSE, 0, &path_ai);
        albedo_tex.load(base_path / path_ai.C_Str());
    }
    // fetch normal tex
    if (material_ai->GetTextureCount(aiTextureType_HEIGHT) > 0) {
        aiString path_ai;
        material_ai->GetTexture(aiTextureType_HEIGHT, 0, &path_ai);
        normal_tex.load(base_path / path_ai.C_Str(), false);
    }
    // fetch alpha tex (or alpha channel from diffuse tex)
    if (material_ai->GetTextureCount(aiTextureType_OPACITY) > 0) {
        aiString path_ai;
        material_ai->GetTexture(aiTextureType_OPACITY, 0, &path_ai);
        alpha_tex.load(base_path / path_ai.C_Str());
    } else if (albedo_tex.has_alpha)
        alpha_tex.load_alpha(albedo_tex.path());
    // fetch roughness tex
    if (material_ai->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString path_ai;
        material_ai->GetTexture(aiTextureType_OPACITY, 0, &path_ai);
        roughness_tex.load(base_path / path_ai.C_Str());
    }
    // fetch emissive tex
    if (material_ai->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
        aiString path_ai;
        material_ai->GetTexture(aiTextureType_EMISSIVE, 0, &path_ai);
        emissive_tex.load(base_path / path_ai.C_Str());
    }

    // material preset selection hack
    type = name;
    set_to(type);
}

Material::~Material() {
    // delete from global instance map
    std::lock_guard<std::mutex> guard(mat_mutex);
    for (auto it = instances.begin(); it != instances.end(); ) {
        if (*it == this)
            it = instances.erase(it);
        else
            it++;
    }
}

glm::vec3 Material::albedo(const glm::vec2& TC) const {
    return albedo_tex ? albedo_tex(TC) : albedo_col;
}

glm::vec3 Material::emissive(const glm::vec2& TC) const {
    if (emissive_strength <= 0) return glm::vec3(0);
    return emissive_tex ? emissive_tex(TC) * emissive_strength : albedo(TC) * emissive_strength;
}

float Material::roughness(const glm::vec2& TC) const {
    return roughness_tex ? luma(roughness_tex(TC)) : roughness_val;
}

glm::vec3 Material::normalmap(const glm::vec3 &N, const glm::vec2 &TC) const {
    return normal_tex ? tangent_to_world(N, normalize(normal_tex(TC) * 2.f - 1.f)) : N;
}

float Material::alphamap(const glm::vec2& TC) const {
    return alpha_tex ? luma(alpha_tex(TC)) : 1.f;
}

void Material::set_to(const std::string& type) {
    // hacky parsing of material type
    if (type.find("emissive") != std::string::npos || type.find("light") != std::string::npos)
        set_light();
    else if (type.find("layered_ggx") != std::string::npos)
        set_layered_ggx();
    else if (type.find("diffuse") != std::string::npos || type.find("fabric") != std::string::npos)
        set_diffuse();
    else if (type.find("specular") != std::string::npos)
        set_specular();
    else if (type.find("phong") != std::string::npos)
        set_phong();
    else if (type.find("microfacet") != std::string::npos)
        set_microfacet();
    else if (type.find("plastic") != std::string::npos)
        set_layered_ggx();
    else if (type.find("translucent") != std::string::npos)
        set_translucent();
    else if (type.find("glass") != std::string::npos || type.find("staklo") != std::string::npos)
        set_glass();
    else if (type.find("water") != std::string::npos)
        set_water();
    else if (type.find("metal") != std::string::npos)
        set_metal();
    else if (type.find("gold") != std::string::npos)
        set_gold();
    else if (type.find("silver") != std::string::npos)
        set_silver();
    else if (type.find("copper") != std::string::npos)
        set_copper();
    else
        set_default();
}
void Material::set_light() {
    brdf.reset(new LambertianReflection);
    if (emissive_strength <= 0.f)
        emissive_strength = 10.f;
    type = "light";
}
void Material::set_diffuse() {
    brdf.reset(new LambertianReflection);
    type = "diffuse";
}
void Material::set_specular() {
    brdf.reset(new SpecularReflection);
    ior = 1.52f; // window glass
    type = "specular";
}
void Material::set_phong() {
    brdf.reset(new SpecularPhong);
    type = "phong";
}
void Material::set_microfacet() {
    brdf.reset(new MicrofacetReflection(coated));
    ior = 2.42f; // chrome
    type = "microfacet";
}
void Material::set_translucent() {
    brdf.reset(new LambertianTransmission);
    ior = 1.52f; // window glass
    type = "translucent";
}
void Material::set_glass() {
    brdf.reset(new SpecularFresnel);
    //brdf.reset(new GlassSurface);
    //brdf.reset(new MicrofacetTransmission);
    ior = 1.52f; // window glass
    type = "glass";
}
void Material::set_water() {
    brdf.reset(new SpecularFresnel);
    albedo_col = glm::vec3(64, 164, 223) / glm::vec3(255);
    ior = 1.33f; // water
    roughness_val = 0.0001f;
    type = "water";
}
void Material::set_metal() {
    brdf.reset(new MetallicSurface);
    ior = 2.42f; // chrome
    absorb = .95f;
    type = "metal";
}
void Material::set_gold() {
    brdf.reset(new MetallicSurface);
    ior = 0.75f;
    absorb = 2.12f;
    roughness_val = roughness_from_exponent(350.f);
    albedo_col = glm::vec3(235, 197, 73) / glm::vec3(255);
    type = "gold";
}
void Material::set_silver() {
    brdf.reset(new MetallicSurface);
    ior = 0.15f;
    absorb = 2.75f;
    roughness_val = roughness_from_exponent(100.f);
    albedo_col = glm::vec3(144, 144, 144) / glm::vec3(255);
    type = "silver";
}
void Material::set_copper() {
    brdf.reset(new MetallicSurface);
    ior = 1.12f;
    absorb = 2.5f;
    roughness_val = roughness_from_exponent(75.f);
    albedo_col = glm::vec3(176, 72, 33) / glm::vec3(255);
    type = "copper";
}
void Material::set_layered_ggx() {
    brdf.reset(new LayeredSurface);
    ior = 1.3f;
    type = "layered_ggx";
}
void Material::set_default() {
    set_layered_ggx();
    ior = 1.3f;
    type = "default";
}

json11::Json Material::to_json() const {
    return json11::Json::object {
        { "name", name },
        { "type", type },
        { "ior", ior },
        { "roughness", roughness_val },
        { "coated", coated },
        { "absorb", absorb },
        { "albedo_col", json11::Json::array{ albedo_col.x, albedo_col.y, albedo_col.z } },
        { "emissive_strength", emissive_strength }
    };
}

void Material::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        if (cfg["name"].is_string())
            name = cfg["name"].string_value();
        if (cfg["type"].is_string())
            type = cfg["type"].string_value();
        else
            type = name;
        json_set_bool(cfg, "coated", coated);
        set_to(type);
        json_set_float(cfg, "ior", ior);
        json_set_float(cfg, "roughness", roughness_val);
        json_set_float(cfg, "absorb", absorb);
        json_set_vec3(cfg, "albedo_col", albedo_col);
        json_set_float(cfg, "emissive_strength", emissive_strength);
    }
}
