#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/bdpt.h"

struct Bidirectional : public Algorithm {
    inline static const std::string name = "Bidirectional";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }
};

static AlgorithmRegistrar<Bidirectional> registrar;
