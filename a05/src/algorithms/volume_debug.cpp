#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/random.h"
#include "gi/ray.h"
#include "gi/mesh.h"
#include "gi/material.h"
#include "gi/light.h"
#include "gi/timer.h"
#include "gi/color.h"

using namespace std;
using namespace glm;

struct VolumeDebug : public Algorithm {
    inline static const std::string name = "VolumeDebug";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        if (!context.scene.volume) return;
        const uint32_t w = context.fbo.width(), h = context.fbo.height();
        for (uint32_t i = 0; i < samples; ++i) {
            vec3 L(0);
            if (x < w/2) {
                if (y < h/2) {
                    Ray ray = context.cam.view_ray(x, y, w/2, h/2, RNG::uniform<vec2>(), RNG::uniform<vec2>());
                    const auto [hit, t] = context.scene.volume->sample_raymarching(ray);
                    const auto [bb_min, bb_max] = context.scene.volume->compute_AABB();
                    L = hit ? (bb_max - (ray.org + t * ray.dir)) / (bb_max - bb_min) : vec3(0);
                } else {
                    Ray ray = context.cam.view_ray(x, y-h/2, w/2, h/2, RNG::uniform<vec2>(), RNG::uniform<vec2>());
                    L = vec3(context.scene.volume->transmittance_raymarching(ray));
                }
            } else {
                if (y < h/2) {
                    Ray ray = context.cam.view_ray(x-w/2, y, w/2, h/2, RNG::uniform<vec2>(), RNG::uniform<vec2>());
                    const auto [hit, t] = context.scene.volume->sample_delta_tracking(ray);
                    const auto [bb_min, bb_max] = context.scene.volume->compute_AABB();
                    L = hit ? (bb_max - (ray.org + t * ray.dir)) / (bb_max - bb_min) : vec3(0);
                } else {
                    Ray ray = context.cam.view_ray(x-w/2, y-h/2, w/2, h/2, RNG::uniform<vec2>(), RNG::uniform<vec2>());
                    L = vec3(context.scene.volume->transmittance_ratio_tracking(ray));

                }
            }
            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<VolumeDebug> registrar;
