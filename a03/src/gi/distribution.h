#pragma once

#include <tuple>
#include <vector>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

/**
 * @brief 1D distribution for importance sampling an arbitrary discrete 1D function
 */
class Distribution1D {
public:
    Distribution1D();

    /**
     * @brief Construct from array of function values
     *
     * @param f function values
     * @param n function length
     */
    Distribution1D(const float *f, uint32_t N);

    virtual ~Distribution1D();

    inline float f(size_t i) const { assert(i < size()); return func[i]; }
    inline uint32_t size() const { return func.size(); }

    /**
     * @brief Compute absolute integral over the discrete function
     *
     * @return Absolute integral over function values
     */
    double integral() const;

    /**
     * @brief Compute normalized integral over the discrete function
     *
     * @return Normalized integral over function values
     */
    double unit_integral() const;

    /**
     * @brief Compute continuous PDF for given sample
     *
     * @param sample Continuous sample drawn from this distribution in [0, 1)
     *
     * @return PDF of continuous sample
     */
    float pdf(float sample) const;

    /**
     * @brief Compute discrete PDF for given sample
     *
     * @param index Discrete sample drawn from this distribution in [0, N)
     *
     * @return PDF of discrete sample
     */
    float pdf(size_t index) const;

    /**
     * @brief Compute an importance sampled coordinate in [0, 1) from an uniform sample
     *
     * @param sample Uniform random sample in [0, 1)
     *
     * @return Tuple consisting of:
     *      - Importance sampled coordinate in [0, 1) (float)
     *      - PDF of sampled coordinate (float)
     */
    std::tuple<float, float> sample_01(float sample) const;

    /**
     * @brief Compute an importance sampled index in [0, N) from an uniform sample
     *
     * @param sample Uniform random sample in [0, 1)
     *
     * @return Tuple consisting of:
     *      - Importance sampled index in [0, N) (uint)
     *      - PDF of sampled index (float)
     */
    std::tuple<uint32_t, float> sample_index(float sample) const;

private:
    // data
    std::vector<float> func, cdf;
    double f_integral;
};

/**
 * @brief 2D distribution for importance sampling an arbitrary discrete 2D function
 */
class Distribution2D {
public:
    /**
     * @brief Construct from linearized 2D-array of function values
     *
     * @param f function values
     * @param w function width
     * @param h function height
     */
    Distribution2D(const float* func, uint32_t w, uint32_t h);
    virtual ~Distribution2D();

    /**
     * @brief Query integral of function
     *
     * @return Integral of function
     */
    double integral() const;
    double unit_integral() const;

    /**
     * @brief Compute importance sampled 2D-coordinates in [0, 1) from given random sample in [0, 1)
     *
     * @param sample Random sample in [0, 1)
     *
     * @return Tuple consisting of:
     *      - Importance sampled coordinates in [0, 1) (vec2)
     *      - PDF of sampled coordinates (float)
     */
    std::tuple<glm::vec2, float> sample_01(const glm::vec2 &sample) const;

    /**
     * @brief Compute PDF for given sample from this distribution
     *
     * @param sample Sample in [0,1)
     *
     * @return PDF of given sample from this distribution
     */
    float pdf(const glm::vec2& sample) const;

private:
    // data
    // TODO ASSIGNMENT3
    // add data structures for conditional and marginal distributions
    Distribution1D marginal_y;
    std::vector<Distribution1D> conditional;
    double f_integral;
};

// ----------------------------------------------------
// Debug utilities

void debug_distributions();
void plot_histogram(const Distribution1D& dist);
void plot_heatmap(const Distribution2D& dist, uint32_t w, uint32_t h);
