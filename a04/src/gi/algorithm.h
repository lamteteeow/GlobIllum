#pragma once

#include <map>
#include <string>
#include <memory>
#include "json11.h"

// forward declare context
class Context;

class Algorithm {
public:
    virtual ~Algorithm() = default;

    /**
     * @brief Read json11::Json config provided by the user, e.g. for algorithm specific parameters
     *
     * @param cfg json11::Json config file to read from
     */
    virtual void read_config(const json11::Json& cfg) {}

    /**
     * @brief Preprocessing step done once before each call to render(), e.g. to build a photon map
     *
     * @param context Context reference, providing the context
     */
    virtual void init(Context& context) {}

    /**
     * @brief Actual render callback, responsible for filling the Framebuffer
     *
     * @param context reference to the Context
     */
    virtual void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) = 0;

    /**
     * @brief Called after rendering to allow algorithm to clean up data structures
     */
    virtual void post_render() {}

    /**
     * @brief Static algorithm management (populated via AlgorithmRegistrar)
     */
    inline static std::map<std::string, std::shared_ptr<Algorithm>> algorithms;

};

/**
 * @brief Register algorithms via Algorithm::algorithms for later rendering
 *
 * @note Template class must have a static member "name" of type std::string!
 *
 * @tparam T Algorithm class template
 */
template <typename T> struct AlgorithmRegistrar {
    AlgorithmRegistrar() {
        Algorithm::algorithms[T::name] = std::make_shared<T>();
    }
};
