#pragma once

#include <omp.h>
#include <random>
#include <vector>
#include <climits>
#include <algorithm>
#include "glm/glm.hpp"

// -------------------------------------
// Random generator interface

/**
 * @brief Random generator engine for concurrent usage in multiple OMP threads
 *
 * The C++ random engine is hidden behind a lazy instantiated singleton (yes.) instance.
 */
class RNG {
public:

    /**
     * @brief Generic function to retrieve random samples of templated type
     *
     * @tparam T Type of random samples
     *
     * @note possible types: float, vec2, vec3, vec4, uint32_t, uvec2, uvec3, uvec4
     *
     * @return Random sample(s) in [0, 1), or [0, UINT_MAX] for given type
     */
    template <typename T> inline static T uniform();

    /**
     * @brief Generate a random float
     *
     * @return Random float in [0.f, 1.f)
     */
    inline static float uniform_float() {
        // there's a bug in the C++ standard on windows, causing 1.f as possible return value
        return std::min(instance().uniform_float_distribution(instance().per_thread_rng[omp_get_thread_num()]), 0.99999f);
    }

    /**
     * @brief Generate a random unsigned integer
     *
     * @return Random unsigned integer in [0, UINT_MAX]
     */
    inline static uint32_t uniform_uint() {
        return instance().uniform_uint_distribution(instance().per_thread_rng[omp_get_thread_num()]);
    }

    /**
     * @brief Shuffle vector of samples
     *
     * @param target Vector of samples to be shuffled
     */
    template <typename T> inline static void shuffle(std::vector<T>& target) {
        std::shuffle(target.begin(), target.end(), instance().per_thread_rng[omp_get_thread_num()]);
    }

    RNG(const RNG&)             = delete;
    RNG& operator=(const RNG&)  = delete;
    RNG& operator=(const RNG&&) = delete;

protected:
    inline static RNG& instance() {
        static RNG rng;
        return rng;
    }

private:
    RNG() : per_thread_rng(omp_get_max_threads()) {
        for (int i = 0; i < omp_get_max_threads(); ++i)
            per_thread_rng[i].seed(i);
    }

    ~RNG() {}

    std::vector<std::mt19937> per_thread_rng;
    std::uniform_real_distribution<float> uniform_float_distribution{0.f, 1.f};
    std::uniform_int_distribution<uint32_t> uniform_uint_distribution{0, UINT_MAX};
};

// uniform<T>() specializations
template <> inline float RNG::uniform<float>() {
    return uniform_float();
}
template <> inline glm::vec2 RNG::uniform<glm::vec2>() {
    return glm::vec2(uniform_float(), uniform_float());
}
template <> inline glm::vec3 RNG::uniform<glm::vec3>() {
    return glm::vec3(uniform_float(), uniform_float(), uniform_float());
}
template <> inline glm::vec4 RNG::uniform<glm::vec4>() {
    return glm::vec4(uniform_float(), uniform_float(), uniform_float(), uniform_float());
}
template <> inline uint32_t RNG::uniform<uint32_t>() {
    return uniform_uint();
}
template <> inline glm::uvec2 RNG::uniform<glm::uvec2>() {
    return glm::uvec2(uniform_uint(), uniform_uint());
}
template <> inline glm::uvec3 RNG::uniform<glm::uvec3>() {
    return glm::uvec3(uniform_uint(), uniform_uint(), uniform_uint());
}
template <> inline glm::uvec4 RNG::uniform<glm::uvec4>() {
    return glm::uvec4(uniform_uint(), uniform_uint(), uniform_uint(), uniform_uint());
}
