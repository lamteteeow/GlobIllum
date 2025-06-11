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
        // some shortcuts
        Camera& cam = context.cam;
        Scene& scene = context.scene;
        Framebuffer& fbo = context.fbo;
        size_t w = fbo.width(), h = fbo.height();

		UniformSampler2D hemisphere_sampler;
        hemisphere_sampler.init(samples);
		glm::vec2 hemisphere_sample;

        for (uint32_t i = 0; i < samples; ++i) {
            vec3 L(0);
            // setup view ray
            Ray ray = cam.view_ray(x, y, w, h, RNG::uniform<vec2>(), RNG::uniform<vec2>());
            // intersect main ray with scene
            const SurfaceHit hit = scene.intersect(ray);
            // check if a hit was found
            if (hit.valid) {
                if (hit.is_light())
                    L = hit.Le();
                else {
                    constexpr bool UNIFORM = true;
                    if (UNIFORM) {
                        // TODO ASSIGNMENT2
                        // implement Monte Carlo integration via uniform hemisphere sampling here
                        // - draw a uniform random sample on the hemisphere in tangent space and transform it into world-space
                        // - intersect the ray with the scene and check if you hit a light source
                        // - if a light source was hit, compute the irradiance via the given equation
						// L = hit.albedo(); 

						constexpr size_t hemisphere_samples = 16;

                        for (size_t j = 0; j < hemisphere_samples; ++j) {
                            // Next sample from the hemisphere sampler
                            hemisphere_sample = hemisphere_sampler.next();
                            // Generate a uniform direction in tangent space
                            //vec3 w_i_local = uniform_sample_hemisphere(hemisphere_sample);
                            vec3 w_i_local = cosine_sample_hemisphere(hemisphere_sample);
                            vec3 w_i = hit.to_world(w_i_local);
                            Ray secondary_ray(hit.P + 1e-4f * hit.N, w_i);
                            // Intersect with scene
                            SurfaceHit light_hit = scene.intersect(secondary_ray);
                            // Check if we hit a light source
                            if (light_hit.valid && light_hit.is_light()) {
                                vec3 w_o = -ray.dir;
                                // Evaluate BRDF
                                vec3 brdf = hit.f(w_o, w_i);
                                float cos_term = glm::max(0.0f, dot(hit.N, w_i));
                                //float pdf = uniform_hemisphere_pdf();
                                float pdf = cosine_hemisphere_pdf(cos_term);
                                L += brdf * light_hit.Le() * cos_term / pdf;
                            }
                        }
                        L /= hemisphere_samples;
                    } else {
                        // TODO ASSIGNMENT2
                        // implement Monte Carlo integration via BRDF imporance sampling here
                        // - sample the brdf (BRDF::sample) for a outgoing direction instead of uniform sampling of the hemisphere
                        // - intersect the ray with the scene and check if you hit a light source
                        // - if a light source was hit, compute the irradiance via the given equation
                        //L = hit.albedo();

                        const auto [brdf, w_i, pdf] = hit.sample(-ray.dir, RNG::uniform<vec2>());
                        Ray imp_ray{ hit.P + 1e-4f * hit.N, w_i };
                        const auto imp_hit = scene.intersect(imp_ray);
                        if (imp_hit.is_light()) {
                            L = imp_hit.Le() * brdf * dot(hit.N, w_i) / pdf;
                        }
                    }
                }
            } else // ray esacped the scene
                L = scene.Le(ray);
            // add result to framebuffer
            fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<BRDFImportance> registrar;
