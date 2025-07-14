#include "bdpt.h"
#include "light.h"
#include "material.h"
#include "color.h"
#include "driver/context.h"
#include <mutex>

void trace_cam_path(const Context& context, uint32_t x, uint32_t y, std::vector<PathVertex>& cam_path, RandomWalkCam& walk, const uint32_t max_path_len, const uint32_t rr_min_path_len, const float rr_threshold, const bool specular_path_tracing) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

void trace_light_path(const Context& context, std::vector<PathVertex>& light_path, RandomWalkLight& walk, const uint32_t max_path_len, const uint32_t rr_min_path_len, const float rr_threshold) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 connect_and_shade(const Context& context, const std::vector<PathVertex>& cam_path, const std::vector<PathVertex>& light_path) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

void trace_photons(const Context& context, int N, std::vector<PathVertex>& photons, bool scale_photon_power) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
