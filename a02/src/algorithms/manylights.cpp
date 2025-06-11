#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/bdpt.h"
#include "gi/rng.h"

using namespace std;
using namespace glm;

struct ManyLights : public Algorithm {
    inline static const std::string name = "ManyLights";

    // ManyLights parameters: trade quality for performance here
    const uint32_t NUM_VPL_PATHS = 1 << 14;
    const uint32_t VPL_PATHS_PER_SAMPLE = 4;

    // called once before each(!) rendering
    void init(Context& context) {
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }

};

static AlgorithmRegistrar<ManyLights> registrar;
