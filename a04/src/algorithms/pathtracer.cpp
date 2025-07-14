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

struct Pathtracer : public Algorithm {
    inline static const std::string name = "Pathtracer";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        for (uint32_t i = 0; i < samples; ++i) {
            vec3 L(0);
            
            // TODO ASSIGNMENT4
            // - implement a pathtracer using next event estimation
            // - add russian roulette
            // - (optional, bonus) add multiple importance sampling
            // Setup initial view ray from camera
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());

            // Initialize path throughput
            vec3 throughput(1.0f);

            // Trace path through scene
            for (uint32_t depth = 0; depth < context.MAX_CAM_PATH_LENGTH; ++depth) {
                // Intersect ray with scene
                const SurfaceHit hit = context.scene.intersect(ray);

                if (hit.valid) {
                    // Handle direct light source hit (only for first bounce to avoid double counting)
                    if (hit.is_light() && depth == 0) {
                        L += throughput * hit.Le();
                        break;
                    }

                    // If we hit a light source on subsequent bounces, just terminate
                    // (we handle lighting via next event estimation)
                    if (hit.is_light() && depth > 0) {
                        break;
                    }

                    // Next Event Estimation: Connect to light sources
                    //if (context.scene.total_light_source_power() > 0.0f) { // might be unnecessary check
                    // Sample a light source
                    const auto [light, pdf_light_source] = context.scene.sample_light_source(RNG::uniform<float>());

                    // Sample a point on the light source
                    auto [Li, shadow_ray, pdf_light_sample] = light->sample_Li(hit.P, RNG::uniform<vec2>());

                    const float pdf_light = pdf_light_source * pdf_light_sample;

                    // Check if the light sample is valid and visible
                    if (pdf_light > 0.0f && !context.scene.occluded(shadow_ray)) {
                        // Evaluate BRDF
                        const vec3 brdf = hit.f(-ray.dir, shadow_ray.dir);
                        const float cos_theta = abs(dot(hit.N, shadow_ray.dir));

                        // Add direct lighting contribution
                        L += throughput * Li * brdf * cos_theta / pdf_light;
                    }
                    //}

                    // Sample BRDF for next direction
                    const auto [brdf, w_i, pdf_brdf] = hit.sample(-ray.dir, RNG::uniform<vec2>());

                    if (pdf_brdf <= 0.0f) {
                        break;
                    }

                    // Update throughput with BRDF and cosine term
                    const float cos_theta = abs(dot(hit.N, w_i));
                    throughput *= brdf * cos_theta / pdf_brdf;

                    // Russian roulette
                    if (depth >= context.RR_MIN_PATH_LENGTH) {
                        float rr_prob = fmaxf(context.RR_THRESHOLD, luma(throughput));
                        if (RNG::uniform<float>() <= rr_prob) {
                            break;
                        }
                        // Compensation
                        throughput /= (1.0f - rr_prob);
                    }

                    // Setup next ray
                    ray = Ray(hit.P, w_i);

                }
                else {
                    // Ray escaped scene - add sky light contribution (only for first bounce to avoid double counting)
                    if (depth == 0) {
                        L += throughput * context.scene.Le(ray);
                    }
                    break;
                }
            }

            // Ensure no negative values
            L = max(L, vec3(0.0f));

            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<Pathtracer> registrar;
