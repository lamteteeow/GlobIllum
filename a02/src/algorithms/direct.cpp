#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/scene.h"
#include "gi/random.h"
#include "gi/light.h"
#include "gi/ray.h"
#include "gi/hit.h"

using namespace std;
using namespace glm;

struct DirectIllumination : public Algorithm {
    inline static const std::string name = "DirectIllumination";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        // some shortcuts
        const Camera& cam = context.cam;
        const Scene& scene = context.scene;
        Framebuffer& fbo = context.fbo;
        const size_t w = fbo.width(), h = fbo.height();

        for (uint32_t i = 0; i < samples; ++i) {
            vec3 L(0);
            // setup a view ray
            Ray ray = cam.view_ray(x, y, w, h, RNG::uniform<vec2>(), RNG::uniform<vec2>());
            // intersect main ray with scene
            const SurfaceHit hit = scene.intersect(ray);
            // check if a hit was found
            if (hit.valid) {
                if (hit.is_light()) // direct light source hit
                    L = hit.Le();
                else { // surface hit -> shade
                    // TODO ASSIGNMENT2
                    // modify the shading to include the BRDF via SurfaceHit::f
                    // Note that with BRDFs, vectors always point away from the surface by convention
                    const auto [light, pdf_light_source] = scene.sample_light_source(RNG::uniform<float>());
                    auto [Li, shadow_ray, pdf_light_sample] = light->sample_Li(hit.P, RNG::uniform<vec2>());
                    const float pdf = pdf_light_source * pdf_light_sample;
                    if (pdf > 0.f && !scene.occluded(shadow_ray))
                        //L = Li * hit.albedo() * fmaxf(0.f, dot(hit.N, shadow_ray.dir)) / pdf;
                        // Compute contribution using BRDF instead of albedo
                        L = Li * hit.f(-ray.dir, shadow_ray.dir) * fmaxf(0.f, dot(hit.N, shadow_ray.dir)) / pdf;
                }
            } else // ray esacped the scene
                L = scene.Le(ray);
            // add result to framebuffer
            fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<DirectIllumination> registrar;
