#pragma once

#include <glm/gtx/norm.hpp> // glm::length2()

struct AABB {
    AABB() = default;

    AABB(const glm::vec3 min, const glm::vec3 max)
        : min(min), max(max) {}

    [[nodiscard]] AABB merge(const AABB &other) const;
    [[nodiscard]] float dist_sqr(const glm::vec3 &p) const;
    [[nodiscard]] AABB align(const glm::vec3 &p, const glm::vec3 &n) const;

    glm::vec3 min;
    glm::vec3 max;
};

inline AABB AABB::merge(const AABB &other) const {
    return {glm::min(min, other.min), glm::max(max, other.max)};
}

inline float AABB::dist_sqr(const glm::vec3 &p) const {
    const glm::vec3 closest = glm::clamp(p, min, max);
    return glm::length2(p - closest);
}

inline AABB AABB::align(const glm::vec3 &p, const glm::vec3 &n) const {
    const glm::vec3 n_cross_z(n.y, -n.x, 0.f);        // glm::cross(n, Z_AXIS);
    const float n_cross_z_sq = n.y * n.y + n.x * n.x; // glm::dot(n_cross_z, n_cross_z);
    const float n_dot_z      = n.z;                   // glm::dot(n, Z_AXIS);

    glm::mat3 m(1.f);

    if (n_cross_z_sq < 0.01f) {
        if (n_dot_z < 0.f) {
            m[0][0] = m[2][2] = -1.f;
        }
    } else {
        const glm::vec3 k = n_cross_z / glm::sqrt(n_cross_z_sq);

        const glm::mat3 K(0, k.z, -k.y, -k.z, 0, k.x, k.y, -k.x, 0);

        m += K + ((1.f - n_dot_z) / n_cross_z_sq) * (K * K);
    }

    const glm::vec3 aabb_min = min - p;
    const glm::vec3 aabb_max = max - p;

    AABB new_aabb(glm::vec3(0), glm::vec3(0));

    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            const float a    = m[j][i] * aabb_min[j];
            const float b    = m[j][i] * aabb_max[j];
            new_aabb.min[i] += a < b ? a : b;
            new_aabb.max[i] += a < b ? b : a;
        }
    }

    return new_aabb;
}
