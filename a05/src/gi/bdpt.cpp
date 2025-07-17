#include "bdpt.h"
#include "light.h"
#include "material.h"
#include "color.h"
#include "driver/context.h"
#include <mutex>

void trace_cam_path(const Context& context, uint32_t x, uint32_t y, std::vector<PathVertex>& cam_path, RandomWalkCam& walk, const uint32_t max_path_len, const uint32_t rr_min_path_len, const float rr_threshold, const bool specular_path_tracing) {
    cam_path.clear();

    const uint32_t w = context.fbo.width(), h = context.fbo.height();
    const Camera& cam = context.cam;
    const Scene& scene = context.scene;

    Ray ray = cam.view_ray(x, y, w, h, walk.pixel_sampler.next(), walk.lens_sampler.next());

    glm::vec3 throughput(1);
    for (uint32_t d = 0; d < max_path_len; ++d) {
        const SurfaceHit& hit = scene.intersect(ray);

        // handle direct light source hits
        if (hit.is_light()) {
            if (hit.valid) {
                if (d == 0 && dot(hit.N, -ray.dir) > 0) {
                    cam_path.emplace_back(hit, throughput * hit.Le());
                }
            } else {
                cam_path.emplace_back(throughput * hit.light->Le(ray));
            }
            break;
        }

        // handle escaped ray
        if (!hit.valid) break;
        const glm::vec3& w_o = -ray.dir;

        // store non-specular vertices
        if (!hit.is_type(BRDF_SPECULAR))
            cam_path.emplace_back(hit, w_o, throughput);

        // when tracing specular bounces only, terminate on diffuse hit
        if (specular_path_tracing && !hit.is_type(BRDF_SPECULAR)) break;
        if (d >= max_path_len - 1) break;

        // bounce main ray
        const auto [brdf, w_i, brdf_pdf] = hit.sample(w_o, walk.bounce_sampler.next());
        if (brdf_pdf <= 0.f || luma(brdf) <= 0.f) break;
        throughput *= brdf * abs(dot(hit.N, w_i)) / brdf_pdf;

        // russian roulette based on throughput
        const float rr_val = luma(throughput);
        if (d > rr_min_path_len && rr_val < rr_threshold) {
            const float prob = glm::max(.05f, 1 - rr_val);
            if (walk.rr_sampler.next() < prob) break;
            throughput /= 1 - prob;
        }

        // setup next ray
        ray = Ray(hit.P, w_i);
    }
}

void trace_light_path(const Context& context, std::vector<PathVertex>& light_path, RandomWalkLight& walk, const uint32_t max_path_len, const uint32_t rr_min_path_len, const float rr_threshold) {
    light_path.clear();

    const Scene &scene = context.scene;
    const auto [light, light_source_pdf] = scene.sample_light_source(walk.light_sampler.next());
    if (light_source_pdf <= 0.f) return;

    // sample light
    auto [Le, ray, light_norm, light_pos_pdf, light_dir_pdf] = light->sample_Le(walk.Le_sampler.next(), walk.dir_sampler.next());
    if (light_pos_pdf <= 0.f) return;

    // initialize throughput and emit Vertex on light source
    glm::vec3 throughput = Le / (light_source_pdf * light_pos_pdf);
    light_path.emplace_back(SurfaceHit(ray.org, light_norm), throughput, light->is_infinite());

    // adjust throughput for sampled direction
    if (light_dir_pdf <= 0.f) return;

    // trace light path
    for (uint32_t d = 1; d < max_path_len; ++d) {
        // bounce from light source
        const SurfaceHit& hit = scene.intersect(ray);
        if (!hit.valid || hit.is_light()) break;
        const glm::vec3& w_o = -ray.dir;

        // store non-specular vertices
        if (!hit.is_type(BRDF_SPECULAR))
            light_path.emplace_back(hit, w_o, throughput);

        // bounce light ray
        const auto [brdf, w_i, brdf_pdf] = hit.sample(w_o, walk.bounce_sampler.next());
        if (brdf_pdf <= 0.f || luma(brdf) <= 0.f) break;
        throughput *= brdf * abs(dot(hit.N, w_i)) / brdf_pdf;

        // correct shading normal
        const float num = abs(dot(w_o, hit.N)) * abs(dot(w_i, hit.Ng));
        const float denom = abs(dot(w_o, hit.Ng)) * abs(dot(w_i, hit.N));
        if (denom <= 0.f) break;
        throughput *= num / denom;

        // russian roulette based on throughput
        const float rr_val = luma(throughput);
        if (d > rr_min_path_len && rr_val < rr_threshold) {
            const float prob = glm::max(.05f, 1 - rr_val);
            if (walk.rr_sampler.next() < prob) break;
            throughput /= 1 - prob;
        }

        // setup next ray
        ray = Ray(hit.P, w_i);
    }
}

glm::vec3 connect_and_shade(const Context& context, const std::vector<PathVertex>& cam_path, const std::vector<PathVertex>& light_path) {
    glm::vec3 L(0);

    // TODO ASSIGNMENT5: connect each camera vertex with each light vertex via a shadow ray
    // Note: see the PathVertex class in src/gi/bdpt.h and check for the on_light, infinite and escaped flags
    // Note: to verify, when Context::MAX_LIGHT_PATH_LENGTH is set to 1, you should get idential results to a pathtracer with next event estimation

    return L;
}

void trace_photons(const Context& context, int N, std::vector<PathVertex>& photons, bool scale_photon_power) {
    photons.clear();
    const Scene& scene = context.scene;
    if (scene.lights.empty()) return;

    // init samplers
    ShuffleSampler<StratifiedSampler1D> light_sampler(N);
    ShuffleSampler<HammersleySampler2D> light_pos_sampler(N);
    ShuffleSampler<HammersleySampler2D> light_dir_sampler(N);

    // trace photons for global photon map
    std::mutex emplace_mutex;
    photons.reserve(N * 3); // guesstimate #photons per path
    #pragma omp parallel for
    for (int p = 0; p < N; ++p) {
        // select light source
        const auto [light, light_source_pdf] = scene.sample_light_source(light_sampler[p]);
        if (light_source_pdf <= 0.f) continue;
        // sample light source
        auto [Le, ray, light_norm, light_pos_pdf, light_dir_pdf] = light->sample_Le(light_pos_sampler[p], light_dir_sampler[p]);
        if (light_pos_pdf <= 0.f || light_dir_pdf <= 0.f) continue;
        glm::vec3 throughput = Le / (light_source_pdf * light_pos_pdf * light_dir_pdf);
        // trace light path
        for (uint32_t d = 0; d < context.MAX_LIGHT_PATH_LENGTH; ++d) {
            // bounce from light source
            const SurfaceHit& hit = scene.intersect(ray);
            if (!hit.valid) break;
            const glm::vec3 w_o = -ray.dir;
            // store photons at non-specular surfaces
            if (!hit.is_type(BRDF_SPECULAR)) {
                std::lock_guard<std::mutex> lock(emplace_mutex);
                photons.emplace_back(hit, w_o, throughput);
            }
            // bounce light ray
            const auto [brdf, w_i, brdf_pdf] = hit.sample(w_o, RNG::uniform<glm::vec2>());
            if (luma(brdf) <= 0.f || brdf_pdf <= 0.f) break;
            throughput *= brdf * abs(dot(hit.N, w_i)) / brdf_pdf;
            // correct shading normal
            const float num = abs(dot(w_o, hit.N)) * abs(dot(w_i, hit.Ng));
            const float denom = abs(dot(w_o, hit.Ng)) * abs(dot(w_i, hit.N));
            if (denom <= 0.f) break;
            throughput *= num / denom;
            // russian roulette based on throughput
            if (d > context.RR_MIN_PATH_LENGTH && luma(throughput) < context.RR_THRESHOLD) {
                const float prob = glm::max(.05f, 1 - luma(throughput));
                if (RNG::uniform<float>() < prob) break;
                throughput /= 1 - prob;
            }
            // setup next ray
            ray = Ray(hit.P, w_i);
        }
    }

    // scale photon power?
    if (scale_photon_power) {
        const float scale_f = 1.f / fmaxf(1.f, photons.size());
        #pragma omp parallel for
        for (int i = 0; i < int(photons.size()); ++i)
            photons[i].throughput *= scale_f;
    }
}
