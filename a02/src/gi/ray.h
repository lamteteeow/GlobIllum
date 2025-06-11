#pragma once

#include <cmath>
#include <cfloat>
#include <glm/glm.hpp>
#include <embree4/rtcore_ray.h>

/**
 * @brief Single ray structure, containing both ray and hit data
 */
class RTC_ALIGN(16) Ray {
    const static inline float EPSILON = 1e-4f;
public:
    /**
     * @brief Default construct as invalid ray
     */
    Ray() :
        tnear(EPSILON),
        tfar(0),
        flags(0),
        primID(RTC_INVALID_GEOMETRY_ID),
        geomID(RTC_INVALID_GEOMETRY_ID),
        instID(RTC_INVALID_GEOMETRY_ID) {}

    /**
     * @brief Construct as valid ray
     *
     * @param o Origin of ray
     * @param d Direction of ray
     * @param len Length of ray (optional)
     */
    Ray(const glm::vec3& o, const glm::vec3& d, float len = FLT_MAX) :
        org(o),
        tnear(EPSILON),
        dir(d),
        tfar(len - 2 * EPSILON),
        mask(-1),
        flags(0),
        primID(RTC_INVALID_GEOMETRY_ID),
        geomID(RTC_INVALID_GEOMETRY_ID),
        instID(RTC_INVALID_GEOMETRY_ID) {}

    /**
     * @brief Test if ray has hit something
     *
     * @return true if ray has hit something, false otherwise
     */
    inline explicit operator bool() const { return geomID != RTC_INVALID_GEOMETRY_ID; }

    /**
     * @brief Return point on ray at given time
     *
     * @param t Time along ray
     *
     * @return Position along ray at time t
     */
    inline glm::vec3 operator()(float t) const { return org + t * dir; }

    // ray data
    glm::vec3 org;              ///< World space ray origin
    float tnear;                ///< Start of ray segment
    glm::vec3 dir;              ///< World space ray direction
    float time;                 ///< Ray time for motion blur (unused)
    float tfar;                 ///< End of ray segment (will be set to hit distance)
    uint32_t mask;              ///< Ray hit mask
    uint32_t id;                ///< Ray ID
    uint32_t flags;             ///< Ray flags
    // hit data
    glm::vec3 Ng;               ///< Object space geometry normal
    float u;                    ///< Barycentric u coordinate of hit
    float v;                    ///< Barycentric v coordinate of hit
    unsigned int primID;        ///< Hit primitive ID
    unsigned int geomID;        ///< Hit geometry ID
    unsigned int instID;        ///< Hit instance ID
    unsigned int instPrimID;    ///< Hit instance primitive ID
};

// some helpful embree conversions
inline RTCRayHit* toRTCRayHit(Ray &ray) { return (RTCRayHit*)&ray; }
inline RTCRay* toRTCRay(Ray &ray) { return (RTCRay*)&ray; }
inline RTCHit* toRTCHit(Ray &ray) { return (RTCHit*)&(ray.Ng.x); }
