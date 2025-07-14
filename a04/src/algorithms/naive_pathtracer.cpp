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
            vec3 L(0);

            // TODO ASSIGNMENT4
            // - implement a (naive) pathtracer using BRDF sampling
            // - add russian roulette

            // Setup initial view ray from camera
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());

            // Initialize path throughput
            vec3 throughput(1.0f);

            // Trace path through scene
            for (uint32_t depth = 0; depth < context.MAX_CAM_PATH_LENGTH; ++depth) {
                const SurfaceHit hit = context.scene.intersect(ray);

                if (hit.valid) {
                    if (hit.is_light()) {
                        L += throughput * hit.Le();
                        break;
                    }

                    // Sample BRDF for next direction
                    const auto [brdf, w_i, pdf] = hit.sample(-ray.dir, RNG::uniform<vec2>());

                    if (pdf <= 0.0f) {
                        break;
                    }

                    // Update throughput with BRDF and cosine term
                    const float cos_theta = abs(dot(hit.N, w_i));
                    throughput *= brdf * cos_theta / pdf;

                    // Russian roulette
                    if (depth >= context.RR_MIN_PATH_LENGTH) {
                        float rr_prob = fminf(context.RR_THRESHOLD, luma(throughput));
                        if (RNG::uniform<float>() <= rr_prob) {
                            break;
                        }
                        // Compensation
                        throughput /= (1.0f - rr_prob);
                    }

                    // Setup next ray
                    ray = Ray(hit.P, w_i);

                } else {
                    L += throughput * context.scene.Le(ray);
                    break;
                }
            }

            // Ensure no negative values
            L = max(L, vec3(0.0f));

            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<NaivePathtracer> registrar;
