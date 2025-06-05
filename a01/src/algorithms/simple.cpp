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

    /*
    Called for each pixel, with the desired amount of samples per pixel as argument.
    */
    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        // some shortcuts
        const Camera& cam = context.cam;
        const Scene& scene = context.scene;
        Framebuffer& fbo = context.fbo;
        const size_t w = fbo.width(), h = fbo.height();

        // TODO ASSIGNMENT1
        // - add and initialize random samplers
        HammersleySampler2D pixel_sampler;
        pixel_sampler.init(samples);
		glm::vec2 pixel_sample;

        HammersleySampler2D lens_sampler;
        lens_sampler.init(samples);
        glm::vec2 lens_sample;

        // - apply supersampling over #samples and DOF using your samplers
        
        // Loop over samples
        for (uint32_t s = 0; s < samples; ++s) {
            vec3 L(0);

            // Get jittered samples for this iteration
            pixel_sample = pixel_sampler.next();
			lens_sample = lens_sampler.next();

            // setup a view ray
            Ray ray = cam.view_ray(x, y, w, h, pixel_sample, lens_sample);
            
            // intersect main ray with scene
            const SurfaceHit hit = scene.intersect(ray);
            
            // check if a hit was found
            if (hit.valid) {
                if (hit.is_light()) // direct light source hit
                    L = hit.Le();
                else { // surface hit -> shading
                    // TODO ASSIGNMENT1
                    // add area light shading via the rendering equation from the assignment sheet
                    // hint: use the following c++17 syntax to capture multiple return values:
                    // const auto [light_ptr, ignore_me] = scene.sample_light_source(...);
                    // auto [Li, shadow_ray, ignore_me2] = light_ptr->sample_Li(...);
                    
                    //StratifiedSampler1D light_select_sampler{};
                    UniformSampler1D light_select_sampler{};                    uint32_t light_select_samples = scene.lights.size();
                    constexpr uint32_t light_samples = 16;
                    light_select_sampler.init(light_select_samples);
                    for (uint32_t light_select_sample = 0; light_select_sample < light_select_samples; light_select_sample++) {
                        const auto [light, light_pdf] = scene.sample_light_source(light_select_sampler.next());
                        HammersleySampler2D light_sampler{};
						//UniformSampler2D light_sampler{};
                        light_sampler.init(light_samples);
                        for (uint32_t light_sample = 0; light_sample < light_samples; light_sample++) {
                            auto [Li, shadow_ray, dir_pdf] = light->sample_Li(hit.P, light_sampler.next());
                            if (!scene.occluded(shadow_ray)) {
                                L += Li * glm::max(glm::dot(shadow_ray.dir, hit.N), 0.f) * hit.albedo() / vec3(M_PI);
                                //L += Li * glm::max(glm::dot(shadow_ray.dir, hit.N), 0.f) * hit.albedo() / vec3(M_PI) / (light_pdf * dir_pdf);
                            }
                        }
                        //fbo.add_sample(x, y, L * (static_cast<AreaLight*>(light.get())->mesh.surface_area() / light_samples));
                    }
                    //continue;
                    //L = hit.albedo();

                    //// Sample light source randomly
                    //StratifiedSampler1D light_select_sampler{};
                    //uint32_t light_select_samples = scene.lights.size();
                    //constexpr uint32_t light_samples = 16;
                    //light_select_sampler.init(light_select_samples);

                    //const auto [light_ptr, light_pdf] = scene.sample_light_source(light_select_sampler.next());

                    //if (light_ptr && light_pdf > 0.0f) {
                    //    // Sample a point on the light
                    //    StratifiedSampler2D point_sampler;
                    //    point_sampler.init(1); // One sample per light

                    //    auto [Li, shadow_ray, dir_pdf] = light_ptr->sample_Li(hit.P, point_sampler.next());

                    //    // Check visibility
                    //    if (!scene.occluded(shadow_ray)) {
                    //        // Calculate shading with BRDF
                    //        float cos_theta = glm::max(glm::dot(shadow_ray.dir, hit.N), 0.f);
                    //        vec3 brdf = hit.albedo() / vec3(M_PI); // Lambertian BRDF

                    //        // Rendering equation (Monte Carlo estimator)
                    //        L += brdf * Li * cos_theta / (light_pdf * dir_pdf);
                    //    }
                    //}
                }
            }
            else // ray escaped the scene
                L = scene.Le(ray);

            // add result to framebuffer
            fbo.add_sample(x, y, L);
        }
    }
};

static AlgorithmRegistrar<SimpleRenderer> registrar;
