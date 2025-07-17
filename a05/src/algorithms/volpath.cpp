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

struct VolumetricPathtracer : public Algorithm {
    inline static const std::string name = "VolumetricPathtracer";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        for (uint32_t i = 0; i < samples; ++i) {
            Ray ray = context.cam.view_ray(x, y, context.fbo.width(), context.fbo.height(), RNG::uniform<vec2>(), RNG::uniform<vec2>());
            vec3 L(0), throughput(1);
            for (uint32_t d = 0; d < context.MAX_CAM_PATH_LENGTH; ++d) {
                const SurfaceHit& hit = context.scene.intersect(ray);
                const VolumeHit& hit_vol = context.scene.intersect_volume(ray);
                if (hit_vol.valid) {

                    // TODO: ASSIGNMENT5
                    // handle volume intersections
                    // - add weighting by albedo, if not accounted for in the intersection routines
                    // - shade volume hit using next event estimation
                    // - scatter ray according to the phase function

                } else {
                    // handle direct light source hits
                    if (hit.is_light()) {
                        if (hit.valid && dot(hit.Ng, -ray.dir) <= 0.f) break; // no double sided light sources
                        L += throughput * (hit.valid ? hit.Le() : hit.light->Le(ray));
                        break;
                    }
                    // handle escaped ray
                    if (!hit.valid) break;

                    // shade surface hit using next event estimation
                    const vec3& w_o = -ray.dir;
                    if (!hit.is_type(BRDF_SPECULAR)) {
                        const auto [light, light_source_pdf] = context.scene.sample_light_source(RNG::uniform<float>());
                        auto [Li, shadow_ray, light_sample_pdf] = light->sample_Li(hit.P, RNG::uniform<vec2>());
                        const float light_pdf = light_source_pdf * light_sample_pdf;
                        const vec3& w_i = shadow_ray.dir;
                        const float cos_theta = dot(hit.N, w_i);
                        if (cos_theta > 0.f && light_pdf > 0.f && !context.scene.occluded(shadow_ray)) {
                            const vec3& brdf = hit.f(w_o, w_i);
                            // TODO: ASSIGNMENT5
                            // account for shadowing from volumes when shading surface hits
                            L += throughput * Li * brdf * cos_theta / light_pdf;
                        }
                    }

                    // scatter surface ray
                    const auto [brdf, w_i, brdf_pdf] = hit.sample(w_o, RNG::uniform<vec2>());
                    if (brdf_pdf <= 0.f || luma(brdf) <= 0.f) break;
                    throughput *= brdf * fabsf(dot(hit.N, w_i)) / brdf_pdf;
                    ray = Ray(hit.P, w_i);
                }

                // russian roulette based on throughput
                if (d > context.RR_MIN_PATH_LENGTH && luma(throughput) < context.RR_THRESHOLD) {
                    const float prob = fmaxf(.05f, 1 - luma(throughput));
                    if (RNG::uniform<float>() < prob) break;
                    throughput /= 1 - prob;
                }
            }
            context.fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<VolumetricPathtracer> registrar;
