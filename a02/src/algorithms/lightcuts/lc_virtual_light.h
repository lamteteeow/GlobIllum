#pragma once

#include "gi/bdpt.h"   // PathVertex
#include "gi/random.h" // StratifiedSampler2D

struct VirtualLight {
    explicit VirtualLight(const PathVertex &v)
        : pos(v.hit.P), norm(v.hit.N), color(v.throughput * brdf_average(v)) {
        assert(v.hit.valid && "virtual lights can only be constructed from valid hits");
        assert(!v.infinite && "directional lights not implemented yet");
    }

  private:
    static glm::vec3 brdf_average(const PathVertex &v) {
        if (v.on_light) {
            return glm::vec3(1.f);
        }

        constexpr uint32_t N = 6 * 6;

        StratifiedSampler2D sampler;
        sampler.init(N);

        glm::vec3 rho{};
        for (uint32_t j = 0; j < N; j++) {
            const auto w_i  = align(v.hit.N, uniform_sample_hemisphere(sampler.next()));
            rho            += v.hit.f(v.w_o, w_i);
        }
        return rho / static_cast<float>(N);
    }

  public:
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
};