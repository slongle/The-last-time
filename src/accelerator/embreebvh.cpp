#include "embreebvh.h"

EmbreeBVH::EmbreeBVH(const std::vector<std::shared_ptr<Mesh>>& meshes, 
    const std::vector<Primitive>& primitives)
    : m_meshes(meshes), m_primitives(primitives),
    m_device(rtcNewDevice(nullptr)), m_scene(rtcNewScene(m_device)), m_prefix(meshes.size() + 1)
{
    m_prefix[0] = 0;
    for (int i = 0; i < m_meshes.size(); i++) {
        const auto& mesh = m_meshes[i];
        // create triangle mesh
        RTCGeometry geom = rtcNewGeometry(m_device, RTC_GEOMETRY_TYPE_TRIANGLE);
        // share data buffers
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 
            mesh->m_vertices, 0, sizeof(float) * 3, mesh->m_vertexNum);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            mesh->m_vertexIndices, 0, sizeof(int) * 3, mesh->m_triangleNum);
        // commit geometry
        rtcCommitGeometry(geom);
        // attach geometry to scene
        rtcAttachGeometryByID(m_scene, geom, i);

        m_prefix[i + 1] = m_prefix[i] + mesh->m_triangleNum;
    }
    rtcCommitScene(m_scene);
}

EmbreeBVH::~EmbreeBVH()
{
    rtcReleaseScene(m_scene);
    rtcReleaseDevice(m_device);
}

void ToRTCRay(const Ray& _ray, RTCRay& ray) {
    ray.org_x = _ray.o.x;
    ray.org_y = _ray.o.y;
    ray.org_z = _ray.o.z;
    ray.dir_x = _ray.d.x;
    ray.dir_y = _ray.d.y;
    ray.dir_z = _ray.d.z;
    ray.tnear = _ray.tMin;
    ray.tfar = _ray.tMax;
    ray.time = 0.0f;
}

void InitRTCHit(RTCHit& hit) {
    hit.geomID = RTC_INVALID_GEOMETRY_ID;
    hit.primID = RTC_INVALID_GEOMETRY_ID;
}

bool EmbreeBVH::Intersect(Ray& ray, HitRecord& hitRec) const
{
    // create intersection context
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    // create ray
    RTCRayHit query;
    ToRTCRay(ray, query.ray);
    InitRTCHit(query.hit);
    // trace ray
    rtcIntersect1(m_scene, &context, &query);
    // hit data filled on hit
    if (query.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }
    // hit data filled on hit
    float u = query.hit.u;
    float v = query.hit.v;
    float t = query.ray.tfar;
    int meshIdx = query.hit.geomID;
    int triangleIdx = query.hit.primID;
    hitRec.m_wi = -ray.d;
    ray.tMax = t;
    hitRec.m_t = t;
    hitRec.m_geoRec.m_uv = Float2(u, v);
    m_meshes[meshIdx]->SetGeometryRecord(triangleIdx, hitRec.m_geoRec);
    hitRec.m_primitive = &m_primitives[m_prefix[meshIdx] + triangleIdx];
    return true;
}

bool EmbreeBVH::Occlude(Ray& _ray) const
{
    // create intersection context    
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    // create ray
    RTCRay ray;
    ToRTCRay(_ray, ray);
    // trace ray
    rtcOccluded1(m_scene, &context, &ray);

    return ray.tfar <= 0.f;
}
