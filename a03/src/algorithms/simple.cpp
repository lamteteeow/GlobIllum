#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/scene.h"
#include "gi/random.h"
#include "gi/light.h"
#include "gi/ray.h"
#include "gi/hit.h"

using namespace std;
using namespace glm;

struct SimpleRenderer : public Algorithm {
    inline static const std::string name = "SimpleRenderer";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        for (uint32_t i = 0; i < samples; ++i) {
            vec3 L(0);
            // setup a view ray
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());
            // intersect main ray with scene
            const SurfaceHit hit = context.scene.intersect(ray);
            // check if a hit was found
            if (hit.valid) {
                if (hit.is_light()) // direct light source hit
                    L = hit.Le();
                else { // surface hit -> shading
                    const auto [light, pdf_light_source] = context.scene.sample_light_source(RNG::uniform<float>());
                    auto [Li, shadow_ray, pdf_light_sample] = light->sample_Li(hit.P, RNG::uniform<vec2>());
                    const float pdf = pdf_light_source * pdf_light_sample;
                    if (pdf > 0.f && !context.scene.occluded(shadow_ray))
                        L = Li * hit.albedo() * fmaxf(0.f, glm::dot(hit.N, shadow_ray.dir)) / pdf;
                }
            } else // ray esacped the scene
                L = context.scene.Le(ray);
            // add result to framebuffer
            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<SimpleRenderer> registrar;
