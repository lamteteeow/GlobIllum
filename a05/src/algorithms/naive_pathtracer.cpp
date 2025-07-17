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

struct NaivePathtracer : public Algorithm {
    inline static const std::string name = "NaivePathtracer";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        for (uint32_t i = 0; i < samples; ++i) {
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());
            vec3 L(0), throughput(1);
            for (uint32_t d = 0; d < context.MAX_CAM_PATH_LENGTH; ++d) {
                const SurfaceHit& hit = context.scene.intersect(ray);
                // handle direct frontal light source hits
                if (hit.is_light()) {
                    if (hit.valid && dot(hit.Ng, -ray.dir) <= 0.f) break; // no double sided light sources
                    L += throughput * (hit.valid ? hit.Le() : hit.light->Le(ray));
                    break;
                }
                // handle escaped ray
                if (!hit.valid) break;

                // break early if max length reached
                if (d >= context.MAX_CAM_PATH_LENGTH - 1) break;

                // bounce main ray
                const auto [brdf, w_i, pdf] = hit.sample(-ray.dir, RNG::uniform<vec2>());
                if (pdf <= 0.f || luma(brdf) <= 0.f) break;
                throughput *= brdf * fabsf(dot(hit.N, w_i)) / pdf;

                // russian roulette based on throughput
                /*
                if (d > context.RR_MIN_PATH_LENGTH && luma(throughput) < context.RR_THRESHOLD) {
                    const float prob = fmaxf(.05f, 1 - luma(throughput));
                    if (RNG::uniform<float>() < prob) break;
                    throughput /= 1 - prob;
                }
                */

                ray = Ray(hit.P, w_i);
            }
            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<NaivePathtracer> registrar;
