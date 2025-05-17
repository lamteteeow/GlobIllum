#pragma once
#include "ray.h"
#include "json11.h"

/**
 * @brief Simple perspective pinhole or environment camera model
 */
class Camera {
public:
    glm::vec3 pos = glm::vec3(0, 0, 0); ///< Camera world space position
    glm::vec3 dir = glm::vec3(1, 0, 0); ///< Camera world space view direction
    glm::vec3 up = glm::vec3(0, 1, 0);  ///< Camera world space up direction
    float fov = 70.f;                   ///< Camera field of view in degrees
    bool perspective = true;            ///< Switch between perspective and environmental projection
    glm::mat3 eye_to_world;             ///< Transformation matrix from eye- to world-space view direction
    // DOF
    float lens_radius = 0.025f;         ///< Lens radius for Depth of Field (DOF)
    float focal_depth = 1.f;            ///< Focal distance for Depth of Field (DOF)

    /**
     * @brief Prepare the camera for rendering, e.g. compute the transformation matrix
     * @note This will be called once before rendering from the driver module.
     */
    void commit();

    /**
     * @brief Compute a view ray for a given pixel
     *
     * @param x Pixel coordinate
     * @param y Pixel coordinate
     * @param w Image/Framebuffer width
     * @param h Image/Framebuffer height
     * @param pixel_sample Random sample in [0, 1) for anti-aliasing (optional)
     * @param lens_sample Random sample in [0, 1) for sampling the lens for DOF (optional)
     *
     * @return View ray through pixel with the coordinates (x, y) optionally jittered for AA and/or DOF
     */
    Ray view_ray(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const glm::vec2& pixel_sample = glm::vec2(.5f), const glm::vec2& lens_sample = glm::vec2(.5)) const;

    /**
     * @brief Compute a perspective view ray for a given pixel
     *
     * @param x Pixel coordinate
     * @param y Pixel coordinate
     * @param w Image/Framebuffer width
     * @param h Image/Framebuffer height
     * @param pixel_sample Random sample in [0, 1) for anti-aliasing (optional)
     *
     * @return View ray through pixel with the coordinates (x, y), optionally jittered for AA
     */
    Ray perspective_view_ray(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const glm::vec2& pixel_sample = glm::vec2(.5f)) const;

    /**
     * @brief Compute an environment view ray for a given pixel
     *
     * @param x Pixel coordinate
     * @param y Pixel coordinate
     * @param w Image/Framebuffer width
     * @param h Image/Framebuffer height
     * @param pixel_sample Random sample in [0, 1) for anti-aliasing (optional)
     *
     * @return View ray through pixel with the coordinates (x, y) optionally jittered for AA
     */
    Ray environment_view_ray(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const glm::vec2& pixel_sample = glm::vec2(.5f)) const;

    /**
     * @brief Add depth of field (DOF) to a view ray, using a simple thin lens model
     *
     * @param ray View ray (in world space) to apply DOF to
     * @param lens_sample Random sample in [0, 1) for sampling the lens
     */
    void apply_DOF(Ray& ray, const glm::vec2& lens_sample) const;

private:
    friend class Context;
    friend class json11::Json;

    /**
     * @brief Export current state to JSON
     *
     * @return Json holding the current state
     */
    json11::Json to_json() const;

    /**
     * @brief Import state from JSON
     *
     * @param cfg JSON config to fetch state from
     */
    void from_json(const json11::Json& cfg);
};

