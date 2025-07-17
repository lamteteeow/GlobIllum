#include "gi/ray.h"
#include "gi/hit.h"
#include "gi/scene.h"
#include "gi/light.h"
#include "gi/random.h"
#include "gi/algorithm.h"
#include "driver/context.h"

using namespace std;
using namespace glm;

struct BRDFImportance : public Algorithm {
    inline static const std::string name = "BRDFImportance";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        for (uint32_t i = 0; i < samples; ++i) {
            vec3 L(0);
            // setup view ray
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());
            // intersect main ray with scene
            const SurfaceHit hit = context.scene.intersect(ray);
            // check if a hit was found
            if (hit.valid) {
                if (hit.is_light())
                    L = hit.Le();
                else {
                    constexpr bool UNIFORM = true;
                    if (UNIFORM) {
                        const uint32_t N = 1;
                        for (uint i = 0; i < N; ++i) {
                            const vec3 w_i = hit.to_world(uniform_sample_hemisphere(RNG::uniform<vec2>()));
                            const float pdf = uniform_hemisphere_pdf();
                            Ray secondary_ray = Ray(hit.P, w_i);
                            const SurfaceHit secondary_hit = context.scene.intersect(secondary_ray);
                            if (secondary_hit.valid && secondary_hit.is_light() && dot(secondary_hit.N, -w_i) > 0)
                                L += secondary_hit.Le() * hit.f(-ray.dir, w_i) * fmaxf(0.f, dot(hit.N, w_i)) / pdf;
                        }
                        L /= float(N);
                    } else {
                        const uint32_t N = 1;
                        for (uint i = 0; i < N; ++i) {
                            const auto [brdf, w_i, pdf] = hit.sample(-ray.dir, RNG::uniform<vec2>());
                            Ray secondary_ray = Ray(hit.P, w_i);
                            const SurfaceHit secondary_hit = context.scene.intersect(secondary_ray);
                            if (secondary_hit.valid && secondary_hit.is_light() && dot(secondary_hit.N, -w_i) > 0)
                                L += secondary_hit.Le() * brdf * fmaxf(0.f, dot(hit.N, secondary_ray.dir)) / pdf;
                        }
                        L /= float(N);
                    }
                }
            } else // ray esacped the scene
                L = context.scene.Le(ray);
            // add result to framebuffer
            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<BRDFImportance> registrar;
