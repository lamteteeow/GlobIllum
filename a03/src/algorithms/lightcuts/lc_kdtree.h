#pragma once

#include "lc_light_tree.h"

#include <glm/gtx/norm.hpp>

class LcKdTree {
  public:
    struct Node {
        const LightTree::Node *node;

        float value;
        uint8_t axis;
        uint8_t depth;
        bool deleted;
        bool deleted_subtree;

        Node *child_left;
        Node *child_right;
    };

  public:
    explicit LcKdTree(const std::vector<LightTree::Node> &nodes);

    [[nodiscard]] std::pair<Node *, float> find_nearest(const LightTree::Node *query) const;

    static void delete_node(Node *node);
    static void update_node(Node *node, const LightTree::Node *update);

    [[nodiscard]] std::vector<Node> &all_nodes() { return m_nodes; }

  private:
    using NodeVector = std::vector<const LightTree::Node *>;
    Node *build_recurse(NodeVector::iterator lower, NodeVector::iterator upper, int depth);

    struct NearestResult {
        float cost;
        float search_radius;
        Node *node;
    };
    static void find_nearest_recurse(const LightTree::Node *query, Node *current, NearestResult &best);
    static void check_nearest(const LightTree::Node *query, Node *current, NearestResult &best);

    static bool is_subtree_deleted_recurse(Node *node);

  private:
    std::vector<Node> m_nodes;
    Node *m_root;
};

// =================================================================================================

inline LcKdTree::LcKdTree(const std::vector<LightTree::Node> &nodes) {
    m_nodes.reserve(nodes.size());

    std::vector<const LightTree::Node *> lights;
    lights.reserve(nodes.size());

    for (const auto &node : nodes) {
        lights.emplace_back(&node);
    }

    m_root = build_recurse(lights.begin(), lights.end(), 0);

    assert(m_nodes.size() == nodes.size() && "invalid kdtree node count");
}

inline LcKdTree::Node *LcKdTree::build_recurse(
    const NodeVector::iterator lower, const NodeVector::iterator upper, const int depth
) {
    assert(depth < std::numeric_limits<uint8_t>::max());
    const int dim = depth % 3;

    Node *n  = &m_nodes.emplace_back();
    n->axis  = static_cast<uint8_t>(dim);
    n->depth = static_cast<uint8_t>(depth);

    const auto count = upper - lower;
    const auto mid   = lower + count / 2;
    std::nth_element(lower, mid, upper, [dim](auto *a, auto *b) { return a->aabb.min[dim] < b->aabb.min[dim]; });

    n->node  = *mid;
    n->value = n->node->aabb.min[dim];

    if (mid - lower > 0) n->child_left = build_recurse(lower, mid, depth + 1);
    if (upper - mid > 1) n->child_right = build_recurse(mid + 1, upper, depth + 1);

    return n;
}

// =================================================================================================

inline std::pair<LcKdTree::Node *, float> LcKdTree::find_nearest(const LightTree::Node *query) const {
    NearestResult best{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), nullptr};
    find_nearest_recurse(query, m_root, best);
    return {best.node, best.cost};
}

inline void LcKdTree::find_nearest_recurse(const LightTree::Node *query, Node *current, NearestResult &best) {
    if (!current || current->deleted_subtree) return;

    if (!current->deleted && current->node != query) {
        check_nearest(query, current, best);
    }

    const float dx = query->light->pos[current->axis] - current->value;
    Node *near     = dx <= 0.f ? current->child_left : current->child_right;
    Node *far      = dx <= 0.f ? current->child_right : current->child_left;

    find_nearest_recurse(query, near, best);
    if ((dx * dx) >= best.search_radius) return;
    find_nearest_recurse(query, far, best);
}

inline void LcKdTree::check_nearest(const LightTree::Node *query, Node *current, NearestResult &best) {
    const AABB merged = query->aabb.merge(current->node->aabb);
    const float dist  = glm::distance2(merged.min, merged.max);

    if (dist >= best.search_radius) return;

    const float cost = dist * glm::length2(query->intensity + current->node->intensity);

    if (cost >= best.cost) return;

    best.cost          = cost;
    best.node          = current;
    best.search_radius = cost / glm::length2(query->intensity);
}

// =================================================================================================

inline void LcKdTree::delete_node(Node *node) {
    node->deleted = true;
    node->node    = nullptr;
    is_subtree_deleted_recurse(node);
}

inline void LcKdTree::update_node(Node *node, const LightTree::Node *update) {
    node->node = update;
}

inline bool LcKdTree::is_subtree_deleted_recurse(Node *node) {
    if (node == nullptr || node->deleted_subtree) {
        return true;
    }

    if (!node->deleted) {
        return false;
    }

    if (is_subtree_deleted_recurse(node->child_left) && is_subtree_deleted_recurse(node->child_right)) {
        node->deleted_subtree = true;
        return true;
    }

    return false;
}