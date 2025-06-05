#pragma once

#include "rng.h"
#include "timer.h"
#include "buffer.h"
#include "texture.h"
#include <chrono>
#include <iostream>
#include "glm/glm.hpp"

// --------------------------------------------------------------------------------
// Random sampler interface

/**
 * @brief Random sampler interface with generic type
 * @note Returned samples must always be in the [0, 1) range.
 *
 * @tparam T Type of the returned samples, e.g. float, glm::vec2, etc.
 */
template <typename T> class Sampler {
public:
    typedef T return_t;

    /**
     * @brief Initialize this sampler to N samples
     *
     * @param N Number of samples that will be drawn from this sampler, via N calls to next()
     */
    virtual void init(uint32_t N) = 0;

    /**
     * @brief Draw the next sample
     *
     * @return The next sample of type T from this distribution 
     */
    virtual T next() = 0;
};

// --------------------------------------------------------------------------------
// Collection of pseudorandom sampling routines

/**
 * @brief Halton low discrepancy sequence (radical inverse)
 *
 * @param i i-th number to draw from this sequence
 * @param base Base for radical inverse computation
 *
 * @return i-th sample of this sequence
 */
inline float halton(int i, uint32_t base) {
    float result = 0;
    float f = 1.f / ((float)base);
    while (i > 0) {
        result = result + (f * (i % base));
        i = i / base;
        f = f / ((float)base);
    }
    return result;
}

/**
 * @brief Van der Corput low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param scramble Seed to scramble the distribution
 *
 * @return i-th sample of this sequence
 */
inline float vandercorput(uint32_t i, uint32_t scramble) {
    i = (i << 16) | (i >> 16);
    i = ((i & 0x00ff00ff) << 8) | ((i & 0xff00ff00) >> 8);
    i = ((i & 0x0f0f0f0f) << 4) | ((i & 0xf0f0f0f0) >> 4);
    i = ((i & 0x33333333) << 2) | ((i & 0xcccccccc) >> 2);
    i = ((i & 0x55555555) << 1) | ((i & 0xaaaaaaaa) >> 1);
    i ^= scramble;
    return ((i >> 8) & 0xffffff) / float(1 << 24);
}

/**
 * @brief Hammersley set low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param n Total number of samples to draw from this sequence
 * @param scramble Seed to scramble the distribution
 *
 * @return i-th sample of this sequence
 */
inline glm::vec2 hammersley(uint32_t i, uint32_t n, uint32_t scramble) {
    return glm::vec2(float(i) / float(n), vandercorput(i, scramble));
}

/**
 * @brief Sobol low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param scramble Seed to scramble the distribution
 *
 * @return i-th sample of this sequence
 */
inline float sobol2(uint32_t i, uint32_t scramble) {
    for (uint32_t v = 1 << 31; i != 0; i >>= 1, v ^= v >> 1)
        if (i & 0x1)
            scramble ^= v;
    return ((scramble >> 8) & 0xffffff) / float(1 << 24);
}

/**
 * @brief 0-2 low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param scramble[2] Seeds to scramble the distributions
 *
 * @return i-th sample of this sequence
 */
inline glm::vec2 sample02(uint32_t i, const uint32_t scramble[2]) {
    return glm::vec2(vandercorput(i, scramble[0]), sobol2(i, scramble[1]));
}

// --------------------------------------------------------------------------------
// 1D sampler implementations

class UniformSampler1D : public Sampler<float> {
public:
    inline void init(uint32_t N) {}

    inline float next() {
        STAT("random sampling");
        return RNG::uniform<float>();
    }
};

class StratifiedSampler1D : public Sampler<float> {
public:
    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        n = N;
        i = 0;
        // Precompute the strata indices and shuffle them for better distribution
        strata.resize(N);
        for (uint32_t i = 0; i < N; ++i)
            strata[i] = i;
        RNG::shuffle(strata);
    }

    inline float next() {
        // TODO ASSIGNMENT1
        // return the next stratified sample
        if (i >= n) i = 0; // wrap around if needed

        // Get the current stratum index and compute a jittered sample within it
        uint32_t stratum = strata[i++];

        // Generate a stratified sample: (stratum + uniform_random) / n
        // This places the sample randomly within its stratum
        return (stratum + RNG::uniform<float>()) / float(n);
    }

private:
    uint32_t n;
    uint32_t i;
	std::vector<uint32_t> strata;
};

// --------------------------------------------------------------------------------
// 2D sampler implementations

class UniformSampler2D : public Sampler<glm::vec2> {
public:
    inline void init(uint32_t N) {}

    inline glm::vec2 next() {
        STAT("random sampling");
        return RNG::uniform<glm::vec2>();
    }
};

class StratifiedSampler2D : public Sampler<glm::vec2> {
public:
    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: you may assume N to be quadratic
        n = uint32_t(std::sqrt(N));
        totalStrata = n * n;
        i = 0;
        // Precompute the strata indices and 
        strata.resize(N);
        for (uint32_t i = 0; i < N; ++i)
            strata[i] = i;
        // Shuffle strata indices for better distribution
        //RNG::shuffle(strata);
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // return the next stratified sample
        if (i >= totalStrata) i = 0; // wrap around if needed

        // Get the current stratum index
        uint32_t stratum = strata[i++];

        // Convert linear index to 2D coordinates
        uint32_t x = stratum % n;
        uint32_t y = stratum / n;

        // Generate a stratified sample within the 2D cell
        return (glm::vec2(x, y) + RNG::uniform<glm::vec2>()) / float(n);
    }

private:
    uint32_t n = 0;
    uint32_t totalStrata = 0;
    uint32_t i = 0;
    std::vector<uint32_t> strata;
};

class HaltonSampler2D : public Sampler<glm::vec2> {
public:
    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: bases 2 and 3 are commonly used
        i = 0;
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // note: see helper function halton() above
        glm::vec2 sample(halton(i, 2), halton(i, 3));
        ++i;
        return sample;
    }

private:
    uint32_t i;
};

class HammersleySampler2D : public Sampler<glm::vec2> {
public:
    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: use a random seed
        n = N;
        i = 0;
        scramble = RNG::uniform<uint32_t>();
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // note: see helper function hammersley() above
        glm::vec2 sample = hammersley(i, n, scramble);
        ++i;
        return sample;
    }

private:
    uint32_t n = 0;
    uint32_t i = 0;
    uint32_t scramble = 0;
};

class LDSampler2D : public Sampler<glm::vec2> {
public:
    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: use two random seeds
        i = 0;
        scramble[0] = RNG::uniform<uint32_t>();
        scramble[1] = RNG::uniform<uint32_t>();
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // note: see helper function sample02() above
        glm::vec2 sample = sample02(i, scramble);
        ++i;
        return sample;
    }

private:
    uint32_t i = 0;
    uint32_t scramble[2] = { 0, 0 };
};

// --------------------------------------------------------------------------------
// Shuffe sampler, precompute and shuffle samples

template <typename Sampler>
class ShuffleSampler {
public:
    ShuffleSampler(uint32_t N) : samples(N) {
        Sampler s;
        s.init(N);
        for (uint32_t i = 0; i < N; ++i) samples[i] = s.next();
        RNG::shuffle(samples);
    }

    typename Sampler::return_t operator[](uint32_t i) { return samples[i]; }

    std::vector<typename Sampler::return_t> samples;
};

// --------------------------------------------------------------------------------
// Debugging utilities

inline void plot_samples(Sampler<glm::vec2>& sampler, const std::string& filename, uint32_t N = 1024) {
    // compute and plot samples
    const uint32_t w = 512, h = 512;
    Buffer<glm::vec3> buffer(w, h);
    buffer = glm::vec3(0);
    sampler.init(N);
    for (uint32_t i = 0; i < N; ++i) {
        const glm::vec2 sample = sampler.next();
        assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
        const int d = 2; // pixel width
        for (int dx = -d; dx <= d; ++dx) {
            for (int dy = -d; dy <= d; ++dy) {
                uint32_t x = sample.x * w + dx;
                uint32_t y = sample.y * h + dy;
                if (x >= 0 && y >= 0 && x < w && y < h)
                    buffer(x, y) = glm::vec3(i / float(N), 1 - i / float(N), 1 - i / float(N));
            }
        }
    }
    // output
    Texture::save_png(filename, w, h, buffer.data());
}

inline void plot_all_samplers2D() {
    UniformSampler2D uni;
    StratifiedSampler2D strat;
    HaltonSampler2D halt;
    HammersleySampler2D hamm;
    LDSampler2D ld;
    plot_samples(uni, "uniform1.png");
    plot_samples(uni, "uniform2.png");
    plot_samples(uni, "uniform3.png");
    plot_samples(strat, "stratified1.png");
    plot_samples(strat, "stratified2.png");
    plot_samples(strat, "stratified3.png");
    plot_samples(halt, "halton1.png");
    plot_samples(halt, "halton2.png");
    plot_samples(halt, "halton3.png");
    plot_samples(hamm, "hammersley1.png");
    plot_samples(hamm, "hammersley2.png");
    plot_samples(hamm, "hammersley3.png");
    plot_samples(ld, "low-discrepancy1.png");
    plot_samples(ld, "low-discrepancy2.png");
    plot_samples(ld, "low-discrepancy3.png");
}

template <typename T> void sampler_benchmark(uint32_t N) {
    // make gcc not optimize away the call
    static glm::vec2 result;
    // init
    T sampler;
    sampler.init(N);
    // benchmark
    const auto start = std::chrono::system_clock::now();
    for (uint32_t i = 0; i < N; ++i)
        result += sampler.next();
    const double dur = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - start).count();
    std::cout << "~" << round(dur / N) << " ns avg." << std::endl;
}

inline void perform_sampler_benchmarks(uint32_t num_samples = 1000000) {
    std::cout << "Sampler benchmarks using " << num_samples << " samples:" << std::endl;
    std::cout << "UniformSampler2D:    "; sampler_benchmark<UniformSampler2D>(num_samples);
    std::cout << "StratifiedSampler2D: "; sampler_benchmark<StratifiedSampler2D>(num_samples);
    std::cout << "HaltonSampler2D:     "; sampler_benchmark<HaltonSampler2D>(num_samples);
    std::cout << "HammersleySampler2D: "; sampler_benchmark<HammersleySampler2D>(num_samples);
    std::cout << "LDSampler2D:         "; sampler_benchmark<LDSampler2D>(num_samples);
}
