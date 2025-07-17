#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/bdpt.h"

struct Bidirectional : public Algorithm {
    inline static const std::string name = "Bidirectional";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        // init
        RandomWalkCam cam_walk;
        RandomWalkLight light_walk;
        cam_walk.init(samples);
        light_walk.init(samples);

        // trace
        std::vector<PathVertex> cam_path, light_path;
        cam_path.reserve(context.MAX_CAM_PATH_LENGTH);
        light_path.reserve(context.MAX_LIGHT_PATH_LENGTH);

        for (uint32_t s = 0; s < samples; ++s) {
            // construct camera path
            trace_cam_path(context, x, y, cam_path, cam_walk, context.MAX_CAM_PATH_LENGTH, context.RR_MIN_PATH_LENGTH, context.RR_THRESHOLD);
            // construct light path
            trace_light_path(context, light_path, light_walk, context.MAX_LIGHT_PATH_LENGTH, context.RR_MIN_PATH_LENGTH, context.RR_THRESHOLD);
            // connect vertices and store result in fbo
            context.fbo.add_sample(x, y, connect_and_shade(context, cam_path, light_path));
        }
    }
};

static AlgorithmRegistrar<Bidirectional> registrar;
