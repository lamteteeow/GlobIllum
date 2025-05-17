#define PAR_SHAPES_IMPLEMENTATION
#include "hit.h"
#include "mesh.h"
#include "random.h"
#include "timer.h"
#include "par_shapes.h"
#include <cfloat>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <embree4/rtcore_ray.h>

// intersection filter function querying the materials alpha map
void alphamapFilter(const RTCFilterFunctionNArguments* args) {
    STAT("alpha filter");
    if (!args->context || !args->geometryUserPtr) return;
    Mesh* mesh = (Mesh*)args->geometryUserPtr;
    if (!mesh->mat->alpha_tex || mesh->tcs.empty()) return;

    for (uint32_t i = 0; i < args->N; ++i) {
        if (args->valid[i] != -1) continue;
        const glm::uvec3& tri = mesh->ibo[RTCHitN_primID(args->hit, args->N, i)];
        const float u = RTCHitN_u(args->hit, args->N, i);
        const float v = RTCHitN_v(args->hit, args->N, i);
        const glm::vec2& TC = (1 - u - v) * mesh->tcs[tri[0]] + u * mesh->tcs[tri[1]] + v * mesh->tcs[tri[2]];
        // perform alpha test
        if (mesh->mat->alphamap(TC) < .1f)
            args->valid[i] = 0; // reject hit
    }
}

Mesh::Mesh(RTCDevice& device, RTCScene& scene, const std::shared_ptr<Material>& mat, const aiMesh* ai_mesh)
    : geom(rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE)), geomID(-1), mat(mat),
    bb_min(FLT_MAX), bb_max(FLT_MIN), center(0.f), radius(FLT_MIN), scene(scene) {
    rtcSetGeometryBuildQuality(geom, RTC_BUILD_QUALITY_HIGH);

    // allocate buffers
    const size_t numVertices = ai_mesh->mNumVertices;
    bool has_tcs = ai_mesh->HasTextureCoords(0);
    vbo.reserve(numVertices + 1); // ensure embree SSE padding
    normals.reserve(numVertices + 1);
    if (has_tcs)
        tcs.reserve(numVertices + 1);
    const size_t numTriangles = ai_mesh->mNumFaces;
    ibo.reserve(numTriangles);

    // extract vertices, normals, tangents, bitangents and tex coords
    for (uint32_t i = 0; i < numVertices; ++i) {
        // vertices
        const aiVector3D &v = ai_mesh->mVertices[i];
        vbo.emplace_back(v.x, v.y, v.z);
        // compute AABB
        bb_min = min(bb_min, vbo[i]);
        bb_max = max(bb_max, vbo[i]);
        // normals
        const aiVector3D &n = ai_mesh->mNormals[i];
        normals.emplace_back(n.x, n.y, n.z);
        // tex coords
        if (has_tcs) {
            const aiVector3D &tc = ai_mesh->mTextureCoords[0][i];
            tcs.emplace_back(tc.x, tc.y);
        }
    }

    // compute radius of disk approximation
    center = (bb_min + bb_max) * .5f;
    for (uint32_t i = 0; i < numVertices; ++i)
        radius = fmaxf(radius, length(vbo[i] - center));

    // extract indices
    for (uint32_t i = 0; i < numTriangles; ++i) {
        const aiFace f = ai_mesh->mFaces[i];
        ibo.emplace_back(f.mIndices[0], f.mIndices[1], f.mIndices[2]);
    }

    // tell embree about the mesh
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, vbo.data(), 0, sizeof(glm::vec3), vbo.size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, ibo.data(), 0, sizeof(glm::uvec3), ibo.size());
    rtcSetGeometryVertexAttributeCount(geom, has_tcs ? 2 : 1);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, normals.data(), 0, sizeof(glm::vec3), normals.size());
    if (has_tcs)
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2, tcs.data(), 0, sizeof(glm::vec2), tcs.size());

    // set user data pointer
    rtcSetGeometryUserData(geom, this);

    // set intersection filter function if material has alphamap
    if (mat->alpha_tex) {
        rtcSetGeometryIntersectFilterFunction(geom, alphamapFilter);
        rtcSetGeometryOccludedFilterFunction(geom, alphamapFilter);
    }

    // commit and attach geometry
    rtcCommitGeometry(geom);
    geomID = rtcAttachGeometry(scene, geom);

    // build distribution over triangle area for importance sampling
    std::vector<float> f(ibo.size());
    for (uint32_t i = 0; i < ibo.size(); ++i) {
        const glm::uvec3& tri = ibo[i];
        const glm::vec3 AB = vbo[tri[1]] - vbo[tri[0]];
        const glm::vec3 AC = vbo[tri[2]] - vbo[tri[0]];
        f[i] = 0.5f * length(cross(AB, AC));

    }
    area_distribution = std::make_shared<Distribution1D>(f.data(), f.size());

    // build area light
    area_light = std::make_shared<AreaLight>(*this);
}

Mesh::Mesh(RTCDevice& device, RTCScene& scene, const std::shared_ptr<Material>& mat, const par_shapes_mesh* par_mesh)
    : geom(rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE)), geomID(-1), mat(mat),
    bb_min(FLT_MAX), bb_max(FLT_MIN), center(0.f), radius(FLT_MIN), scene(scene) {
    rtcSetGeometryBuildQuality(geom, RTC_BUILD_QUALITY_HIGH);

    // allocate buffers
    const size_t numVertices = par_mesh->npoints;
    bool has_tcs = par_mesh->tcoords != 0;
    vbo.reserve(numVertices + 1); // ensure embree SSE padding
    normals.reserve(numVertices + 1);
    if (has_tcs)
        tcs.reserve(numVertices + 1);
    const size_t numTriangles = par_mesh->ntriangles;
    ibo.reserve(numTriangles);

    // extract vertices, normals, tangents, bitangents and tex coords
    for (uint32_t i = 0; i < numVertices; ++i) {
        // vertices
        vbo.emplace_back(par_mesh->points[3*i+0], par_mesh->points[3*i+1], par_mesh->points[3*i+2]);
        // normals
        normals.emplace_back(par_mesh->normals[3*i+0], par_mesh->normals[3*i+1], par_mesh->normals[3*i+2]);
        // tex coords
        if (has_tcs)
            tcs.emplace_back(par_mesh->tcoords[2*i+0], par_mesh->tcoords[2*i+1]);
        // compute AABB
        bb_min = min(bb_min, vbo[i]);
        bb_max = max(bb_max, vbo[i]);
    }

    // compute radius of disk approximation
    center = (bb_min + bb_max) * .5f;
    for (uint32_t i = 0; i < numVertices; ++i)
        radius = fmaxf(radius, length(vbo[i] - center));

    // extract indices TODO check if 3*numTriangles
    for (uint32_t i = 0; i < numTriangles; ++i)
        ibo.emplace_back(par_mesh->triangles[3*i+0], par_mesh->triangles[3*i+1], par_mesh->triangles[3*i+2]);

    // tell embree about the mesh
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, vbo.data(), 0, sizeof(glm::vec3), vbo.size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, ibo.data(), 0, sizeof(glm::uvec3), ibo.size());
    rtcSetGeometryVertexAttributeCount(geom, has_tcs ? 2 : 1);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, normals.data(), 0, sizeof(glm::vec3), normals.size());
    if (has_tcs)
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2, tcs.data(), 0, sizeof(glm::vec2), tcs.size());

    // set user data pointer
    rtcSetGeometryUserData(geom, this);

    // set intersection filter function if material has alphamap
    if (mat->alpha_tex) {
        rtcSetGeometryIntersectFilterFunction(geom, alphamapFilter);
        rtcSetGeometryOccludedFilterFunction(geom, alphamapFilter);
    }

    // commit and attach geometry
    rtcCommitGeometry(geom);
    geomID = rtcAttachGeometry(scene, geom);

    // build distribution over triangle area for importance sampling
    std::vector<float> f(ibo.size());
    for (uint32_t i = 0; i < ibo.size(); ++i) {
        const glm::uvec3& tri = ibo[i];
        const glm::vec3 AB = vbo[tri[1]] - vbo[tri[0]];
        const glm::vec3 AC = vbo[tri[2]] - vbo[tri[0]];
        f[i] = 0.5f * length(cross(AB, AC));

    }
    area_distribution.reset(new Distribution1D(f.data(), f.size()));

    // build area light
    area_light.reset(new AreaLight(*this));
}

Mesh::~Mesh() {
    rtcDetachGeometry(scene, geomID);
    rtcReleaseGeometry(geom);
}

std::tuple<SurfaceHit, float> Mesh::sample(const glm::vec2& sample) const {
    auto [primID, pdf] = area_distribution->sample_index(RNG::uniform<float>());
    return { SurfaceHit(sample, primID, this), pdf };
}
