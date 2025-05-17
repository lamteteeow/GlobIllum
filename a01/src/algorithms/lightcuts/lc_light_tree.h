#pragma once

#include "gi/bdpt.h" // PathVertex

#include "lc_aabb.h"          // AABB
#include "lc_virtual_light.h" // VirtualLight

class LightTree {
  public:
    struct Node {
        const VirtualLight *light;
        glm::vec3 intensity;

        AABB aabb;

        const Node *child_left;
        const Node *child_right;

        explicit Node(const VirtualLight *l)
            : light(l), intensity(l->color), aabb(l->pos, l->pos), child_left(nullptr), child_right(nullptr) {}

        explicit Node(const Node *l, const Node *r)
            : light(l->light),
              intensity(l->intensity + r->intensity),
              aabb(l->aabb.merge(r->aabb)),
              child_left(l),
              child_right(r) {}

        [[nodiscard]] bool is_leaf() const { return child_left == nullptr || child_right == nullptr; }
    };

  public:
    class MemoryArena;
    using MemoryArenaRef = MemoryArena *;

    [[nodiscard]] MemoryArenaRef allocate_memory_arena() const;
    static void free_memory_arena(MemoryArenaRef);

  public:
    LightTree() = default;

    void build(std::vector<VirtualLight> lights, bool print_progress);

    [[nodiscard]] glm::vec3 eval(
        MemoryArenaRef memory, const Context &context, const PathVertex &vertex, bool debug_cut_size
    ) const;

    [[nodiscard]] bool is_empty() const { return m_root == nullptr; }

    void print_binary(const char *filename) const;
    void print_dot(const char *filename) const;

  private:
    std::vector<VirtualLight> m_lights;
    std::vector<Node> m_nodes;
    const Node *m_root{};
};