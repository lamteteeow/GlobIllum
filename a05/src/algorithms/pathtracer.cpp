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
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());
            vec3 L(0), throughput(1);
            float mis_brdf_pdf = 0.f;
            bool specular_bounce = true; // initially set to true to disable MIS on view ray light source hits
            for (uint32_t d = 0; d < context.MAX_CAM_PATH_LENGTH; ++d) {
                const SurfaceHit& hit = context.scene.intersect(ray);
                // handle direct frontal light source hits
                if (hit.is_light()) {
                    if (hit.valid && dot(hit.Ng, -ray.dir) <= 0.f) break; // no double sided light sources
                    const float mis_weight = specular_bounce ? 1.f : power_heuristic(mis_brdf_pdf, hit.light->pdf_Li(hit, ray));
                    L += throughput * mis_weight * (hit.valid ? hit.Le() : hit.light->Le(ray));
                    break;
                }
                // handle escaped ray
                if (!hit.valid) break;

                // shade using next event estimation
                const vec3& w_o = -ray.dir;
                if (!hit.is_type(BRDF_SPECULAR)) {
                    const auto [light, light_source_pdf] = context.scene.sample_light_source(RNG::uniform<float>());
                    auto [Li, shadow_ray, light_sample_pdf] = light->sample_Li(hit.P, RNG::uniform<vec2>());
                    const float light_pdf = light_source_pdf * light_sample_pdf;
                    const vec3& w_i = shadow_ray.dir;
                    const float cos_theta = dot(hit.N, w_i);
                    if (cos_theta > 0.f && light_pdf > 0.f && !context.scene.occluded(shadow_ray)) {
                        const vec3& brdf = hit.f(w_o, w_i);
                        const float weight = power_heuristic(light_pdf, hit.pdf(w_o, w_i));
                        L += throughput * weight * Li * brdf * cos_theta / light_pdf;
                    }
                }

                // break early if max length reached
                if (d >= context.MAX_CAM_PATH_LENGTH - 1) break;

                // bounce main ray
                const auto [brdf, w_i, brdf_pdf] = hit.sample(w_o, RNG::uniform<vec2>());
                if (brdf_pdf <= 0.f || luma(brdf) <= 0.f) break;
                throughput *= brdf * fabsf(dot(hit.N, w_i)) / brdf_pdf;
                mis_brdf_pdf = brdf_pdf;

                // russian roulette based on throughput
                if (d > context.RR_MIN_PATH_LENGTH && luma(throughput) < context.RR_THRESHOLD) {
                    const float prob = fmaxf(.05f, 1 - luma(throughput));
                    if (RNG::uniform<float>() < prob) break;
                    throughput /= 1 - prob;
                }

                ray = Ray(hit.P, w_i);
                specular_bounce = hit.is_type(BRDF_SPECULAR);
            }
            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<Pathtracer> registrar;
