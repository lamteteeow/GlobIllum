#include "lc_light_tree.h"

#include "lc_kdtree.h"
#include "lc_print.h"

#include "driver/context.h" // context.scene.occluded()
#include "gi/color.h"       // luma()

#include <glm/gtx/norm.hpp>        // glm::length2()
#include <glm/gtx/string_cast.hpp> // glm::to_string()

#include <array>
#include <queue>

// =================================================================================================

constexpr size_t MAX_CUT  = 1000;
constexpr float THRESHOLD = 0.02f;

// =================================================================================================
//
// Light Tree Construction
//
// =================================================================================================

struct Candidate {
    struct NodePair {
        const LightTree::Node *node;
        LcKdTree::Node *kd_node;

        [[nodiscard]] bool obsolete() const { return kd_node->deleted || kd_node->node != node; }
    };

    Candidate(LcKdTree::Node *kd_node1, LcKdTree::Node *kd_node2, const float distance)
        : pair1({kd_node1->node, kd_node1}), pair2({kd_node2->node, kd_node2}), distance(distance) {}

    bool operator>(const Candidate &other) const { return distance > other.distance; }

    NodePair pair1, pair2;
    float distance;
};

class CandidateQueue final : public std::priority_queue<Candidate, std::vector<Candidate>, std::greater<>> {
  public:
    void reserve(const size_t size) { c.reserve(size); }
};

void LightTree::build(std::vector<VirtualLight> lights, const bool print_progress) {
    if (lights.empty()) return;

    const size_t light_count = lights.size();
    const size_t node_count  = 2 * light_count - 1;

    // holds all the nodes of the light tree
    std::vector<Node> nodes;
    nodes.reserve(node_count);

    for (size_t i = 0; i < light_count; i++) {
        nodes.emplace_back(&lights[i]);
    }

    // acceleration structure to find nearest neighbor
    LcKdTree kd_tree(nodes);

    // holds pairs of node sorted by cost
    CandidateQueue queue;
    queue.reserve(light_count);

    const auto push_nearest = [&](LcKdTree::Node *query) {
        if (const auto [nearest, distance] = kd_tree.find_nearest(query->node); nearest) {
            queue.emplace(query, nearest, distance);
        }
    };

    LcProgressBar progress_bar;
    if (print_progress) progress_bar.init(node_count);

    for (LcKdTree::Node &node : kd_tree.all_nodes()) {
        push_nearest(&node);

        if (print_progress) progress_bar.advance();
    }

    while (!queue.empty()) {
        const Candidate candidate = queue.top();
        queue.pop();

        const bool node1_used = candidate.pair1.obsolete();
        const bool node2_used = candidate.pair2.obsolete();

        if (node1_used || node2_used) {
            if (!node1_used) push_nearest(candidate.pair1.kd_node);
            if (!node2_used) push_nearest(candidate.pair2.kd_node);
            continue;
        }

        const float i1 = luma(candidate.pair1.node->intensity);
        const float i2 = luma(candidate.pair2.node->intensity);

        if (RNG::uniform_float() < i1 / (i1 + i2)) {
            nodes.emplace_back(candidate.pair1.node, candidate.pair2.node);
        } else {
            nodes.emplace_back(candidate.pair2.node, candidate.pair1.node);
        }

        auto *remain_node = candidate.pair1.kd_node;
        auto *delete_node = candidate.pair2.kd_node;

        if (remain_node->depth > delete_node->depth) {
            std::swap(remain_node, delete_node);
        }

        LcKdTree::delete_node(delete_node);
        LcKdTree::update_node(remain_node, &nodes.back());

        push_nearest(remain_node);

        if (print_progress) progress_bar.advance();
    }

    if (print_progress) progress_bar.end();

    assert(nodes.size() == node_count && "node vector has reallocated");

    m_lights = std::move(lights);
    m_nodes  = std::move(nodes);
    m_root   = &m_nodes.back();
}

// =================================================================================================
//
// Light Tree Evaluation
//
// =================================================================================================

struct LightCluster {
    LightCluster(const LightTree::Node *light, const glm::vec3 estimate, const glm::vec3 error)
        : node(light), estimate(estimate), error(error), cost(glm::length2(error)) {}

    bool operator<(const LightCluster &other) const { return cost < other.cost; }

    const LightTree::Node *node;
    glm::vec3 estimate;
    glm::vec3 error;
    float cost;
};

class LightClusterQueue final : public std::priority_queue<LightCluster, std::vector<LightCluster>, std::less<>> {
  public:
    [[nodiscard]] size_t capacity() const { return c.capacity(); }
    void reserve(const size_t size) { c.reserve(size); }
    void clear() { c.clear(); }
};

// =================================================================================================

class LightTree::MemoryArena {
  public:
    explicit MemoryArena(const size_t light_count) {
        m_mem.resize(omp_get_max_threads());

        const size_t max_cut = std::min(MAX_CUT, light_count);
        for (auto &q : m_mem) {
            q.reserve(max_cut);
        }
    }

    [[nodiscard]] LightClusterQueue alloc_thread_local() {
        assert(m_mem[omp_get_thread_num()].capacity() > 0 && "light cut memory is empty");
        return std::move(m_mem[omp_get_thread_num()]);
    }

    void free_thread_local(LightClusterQueue q) {
        q.clear();
        m_mem[omp_get_thread_num()] = std::move(q);
    }

  private:
    std::vector<LightClusterQueue> m_mem;
};

LightTree::MemoryArenaRef LightTree::allocate_memory_arena() const {
    return new MemoryArena(m_lights.size());
}

void LightTree::free_memory_arena(MemoryArenaRef memory) {
    delete memory;
}

// =================================================================================================

static glm::vec3 calc_estimate(const LightTree::Node *light, const PathVertex &vertex, const Context &context);

static glm::vec3 reuse_estimate(const glm::vec3 &prev, const glm::vec3 &i_parent, const glm::vec3 &i_current);

static glm::vec3 bound_error(const LightTree::Node *light, const PathVertex &vertex);

// =================================================================================================

glm::vec3 LightTree::eval(
    MemoryArenaRef memory, const Context &context, const PathVertex &vertex, const bool debug_cut_size
) const {
    LightClusterQueue cut = memory->alloc_thread_local();

    const auto push_cluster = [&](const Node *node, const Node *parent, const glm::vec3 &prev_estimate) {
        const glm::vec3 estimate = (parent && node->light == parent->light)
                                       ? reuse_estimate(prev_estimate, parent->intensity, node->intensity)
                                       : calc_estimate(node, vertex, context);
        cut.emplace(node, estimate, bound_error(node, vertex));
        return estimate;
    };

    glm::vec3 L = push_cluster(m_root, nullptr, {});

    while (cut.size() < MAX_CUT) {
        const LightCluster cluster = cut.top();
        cut.pop();

        if (cluster.node->is_leaf() || glm::all(glm::lessThanEqual(cluster.error, L * THRESHOLD))) {
            break;
        }

        L -= cluster.estimate;
        L += push_cluster(cluster.node->child_left, cluster.node, cluster.estimate);
        L += push_cluster(cluster.node->child_right, cluster.node, cluster.estimate);
    }

    const size_t cut_size = cut.size() + 1;
    memory->free_thread_local(std::move(cut));

    if (debug_cut_size) {
        return heatmap(static_cast<float>(cut_size) / static_cast<float>(MAX_CUT));
    }

    return vertex.throughput * (L / static_cast<float>(m_lights.size()));
}

// =================================================================================================

glm::vec3 calc_estimate(const LightTree::Node *light, const PathVertex &vertex, const Context &context) {
    const glm::vec3 l = glm::normalize(light->light->pos - vertex.hit.P);
    const float r     = glm::length(light->light->pos - vertex.hit.P);

    if (auto shadow_ray = Ray(vertex.hit.P, l, r); context.scene.occluded(shadow_ray)) {
        return glm::vec3(0.f);
    }

    const float cos_theta_vertex = fmaxf(0.f, dot(vertex.hit.N, l));
    const float cos_theta_light  = fmaxf(0.f, glm::dot(light->light->norm, -l));
    const float G                = cos_theta_vertex * cos_theta_light / (r * r);

    const glm::vec3 brdf_cam = vertex.escaped ? glm::vec3(1) : vertex.hit.f(vertex.w_o, l);

    return light->intensity * brdf_cam * G;
}

glm::vec3 reuse_estimate(const glm::vec3 &prev, const glm::vec3 &i_parent, const glm::vec3 &i_current) {
    return {
        (i_parent.x > 0.f) ? (prev.x / i_parent.x * i_current.x) : 0.f,
        (i_parent.y > 0.f) ? (prev.y / i_parent.y * i_current.y) : 0.f,
        (i_parent.z > 0.f) ? (prev.z / i_parent.z * i_current.z) : 0.f,
    };
}

static float bound_cos_theta(const AABB &aabb) {
    if (aabb.max.z > 0.f) {
        const glm::vec3 p  = glm::clamp(glm::vec3(0.f), aabb.min, aabb.max);
        const float min_x2 = glm::min(p.x * p.x, glm::min(aabb.min.x * aabb.min.x, aabb.max.x * aabb.max.x));
        const float min_y2 = glm::min(p.y * p.y, glm::min(aabb.min.y * aabb.min.y, aabb.max.y * aabb.max.y));
        return aabb.max.z / glm::sqrt(min_x2 + min_y2 + aabb.max.z * aabb.max.z);
    }

    const float max_x2 = glm::max(aabb.min.x * aabb.min.x, aabb.max.x * aabb.max.x);
    const float max_y2 = glm::max(aabb.min.y * aabb.min.y, aabb.max.y * aabb.max.y);
    return aabb.max.z / glm::sqrt(max_x2 + max_y2 + aabb.max.z * aabb.max.z);
}

glm::vec3 bound_error(const LightTree::Node *light, const PathVertex &vertex) {
    // leaf nodes have no error
    if (light->is_leaf()) {
        return glm::vec3(0.f);
    }

    const float dist_sqr = light->aabb.dist_sqr(vertex.hit.P);

    // vertices inside bounding box have infinite error
    if (dist_sqr < 0.001f) {
        return glm::vec3(std::numeric_limits<float>::max());
    }

    const float cos_theta_vertex = fmaxf(0.f, bound_cos_theta(light->aabb.align(vertex.hit.P, vertex.hit.N)));
    const float cos_theta_light  = 1.f; // TODO
    const float G                = cos_theta_vertex * cos_theta_light / dist_sqr;

    // assume max brdf at normal
    const glm::vec3 brdf_cam = vertex.escaped ? glm::vec3(1) : vertex.hit.f(vertex.w_o, vertex.hit.N);

    return light->intensity * brdf_cam * G;
}

// =================================================================================================
//
// Light Tree Printing
//
// =================================================================================================

static void print_binary_recurse(std::ofstream &output, const LightTree::Node *node, const uint32_t level) {
    output.write(reinterpret_cast<const char *>(&node->aabb.min), sizeof(node->aabb.min));
    output.write(reinterpret_cast<const char *>(&node->aabb.max), sizeof(node->aabb.max));
    output.write(reinterpret_cast<const char *>(&node->intensity), sizeof(node->intensity));
    output.write(reinterpret_cast<const char *>(&level), sizeof(level));

    if (node->child_left) print_binary_recurse(output, node->child_left, level + 1);
    if (node->child_right) print_binary_recurse(output, node->child_right, level + 1);
}

void LightTree::print_binary(const char *filename) const {
    std::ofstream output(filename, std::ios::out | std::ios::binary);

    // write header
    const auto header = std::array<uint32_t, 4>{
        0x44556677,
        static_cast<uint32_t>(m_lights.size()),
        static_cast<uint32_t>(m_nodes.size()),
        0x44556677,
    };
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));

    // write lights
    for (const auto &light : m_lights) {
        output.write(reinterpret_cast<const char *>(&light.pos), sizeof(light.pos));
        output.write(reinterpret_cast<const char *>(&light.norm), sizeof(light.norm));
        output.write(reinterpret_cast<const char *>(&light.color), sizeof(light.color));
    }

    // write nodes
    print_binary_recurse(output, m_root, 0);

    output.close();

    std::cout << "LightCuts: " << filename << " written" << std::endl;
}

static void print_dot_recurse(std::ofstream &output, const LightTree::Node *node, const LightTree::Node *parent) {
    output << "  n" << node << " [label=\"" << glm::to_string(node->intensity) << "\"]" << std::endl;

    if (parent) output << "  n" << parent << " -> n" << node << std::endl;
    if (node->child_left) print_dot_recurse(output, node->child_left, node);
    if (node->child_right) print_dot_recurse(output, node->child_right, node);
}

void LightTree::print_dot(const char *filename) const {
    std::ofstream output(filename, std::ios::out);

    output << "digraph G {\n";
    output << "  graph [ordering=\"out\"];\n";
    output << std::setprecision(2) << std::fixed;

    print_dot_recurse(output, m_root, nullptr);

    output << "}\n";

    output.close();

    std::cout << "LightCuts: " << filename << " written" << std::endl;
}
