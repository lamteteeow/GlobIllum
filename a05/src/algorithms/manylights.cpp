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
        vpl_paths.clear();
        RandomWalkLight light_walk;
        light_walk.init(NUM_VPL_PATHS);
        for (uint32_t i = 0; i < NUM_VPL_PATHS; ++i) {
            vpl_paths.push_back(std::vector<PathVertex>());
            trace_light_path(context, vpl_paths[i], light_walk, context.MAX_LIGHT_PATH_LENGTH, context.RR_MIN_PATH_LENGTH, context.RR_THRESHOLD);
        }
        size_t num_vpls = 0;
        for (const auto& path : vpl_paths)
            num_vpls += path.size();
        std::cout << "Num VPLs: " << num_vpls << std::endl;
    }

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        samples = std::max(1u, samples / VPL_PATHS_PER_SAMPLE); // adjust sample count for the amount of light paths per pixel
        // init
        RandomWalkCam cam_walk;
        cam_walk.init(samples);
        vector<PathVertex> cam_path;
        // trace
        for (uint32_t s = 0; s < samples; ++s) {
            cam_path.clear();
            // construct camera path
            trace_cam_path(context, x, y, cam_path, cam_walk, context.MAX_CAM_PATH_LENGTH, context.RR_MIN_PATH_LENGTH, context.RR_THRESHOLD);
            // connect vertices and store result in fbo
            for (uint32_t i = 0; i < VPL_PATHS_PER_SAMPLE; ++i)
                context.fbo.add_sample(x, y, connect_and_shade(context, cam_path, vpl_paths[int(RNG::uniform<float>() * vpl_paths.size())]));
        }
    }

    // data
    std::vector<std::vector<PathVertex>> vpl_paths;
};

static AlgorithmRegistrar<ManyLights> registrar;
