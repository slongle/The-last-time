#include "triangle.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Mesh* LoadObjMesh(std::string filename) 
{
    std::cout << "Loading " << filename << std::endl;
    filename = GetFileResolver()->string() + "/" + filename;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(),
        nullptr);        
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "ERR: " << err << std::endl;
    }
    if (!ret) {
        printf("Failed to load/parse .obj.\n");        
        assert(false);
    }

    assert(shapes.size() == 1);
    size_t vertexNum = attrib.vertices.size() / 3;
    size_t texcoordNum = attrib.texcoords.size() / 2;
    size_t normalNum = attrib.normals.size() / 3;
    size_t triangleNum = shapes[0].mesh.num_face_vertices.size();
    float* vertices = new float[vertexNum * 3];
    float* texcoords = new float[texcoordNum * 2];
    float* normals = new float[normalNum * 3];
    int* indices = new int[triangleNum * 9];

    //TODO : faster memory copy
    for (size_t i = 0; i < vertexNum * 3; i++) {
        vertices[i] = static_cast<const float>(attrib.vertices[i]);
    }
    for (size_t i = 0; i < texcoordNum * 2; i++) {
        texcoords[i] = static_cast<const float>(attrib.texcoords[i]);
    }
    for (size_t i = 0; i < normalNum * 3; i++) {
        normals[i] = static_cast<const float>(attrib.normals[i]);
    }
    for (size_t i = 0; i < triangleNum; i++) {
        for (size_t j = 0; j < 3; j++) {
            indices[i * 9 + j * 3 + 0] = shapes[0].mesh.indices[i * 3 + j].vertex_index;
            indices[i * 9 + j * 3 + 1] = shapes[0].mesh.indices[i * 3 + j].texcoord_index;
            indices[i * 9 + j * 3 + 2] = shapes[0].mesh.indices[i * 3 + j].normal_index;
        }
    }
    
    return new Mesh(vertexNum, texcoordNum, normalNum, triangleNum, vertices, texcoords, normals, indices);
}

std::shared_ptr<Mesh> LoadMesh(const std::string& filename)
{
    std::string ext = GetFileExtension(filename);
    Mesh* mesh = nullptr;
    if (ext == "obj") {
        mesh = LoadObjMesh(filename);
    }
    else {
        std::cout << ext << std::endl;
        assert(false);
    }
    return std::shared_ptr<Mesh>(mesh);
}

Float3 Mesh::GetVertex(const uint32_t& triangleIdx, const uint32_t& vertexIdx) const
{
    int idx = m_indices[triangleIdx * 9 + vertexIdx * 3];
    return Float3(m_vertices[idx * 3 + 0],
        m_vertices[idx * 3 + 1],
        m_vertices[idx * 3 + 2]);
}

Float2 Mesh::GetTexcoord(const uint32_t& triangleIdx, const uint32_t& vertexIdx) const
{
    int idx = m_indices[triangleIdx * 9 + vertexIdx * 3 + 1];
    return Float2(m_texcoords[idx * 2 + 0],
        m_texcoords[idx * 2 + 1]);
}

Float3 Mesh::GetNormal(const uint32_t& triangleIdx, const uint32_t& vertexIdx) const
{
    int idx = m_indices[triangleIdx * 9 + vertexIdx * 3 + 2];
    return Float3(m_normals[idx * 3 + 0],
        m_normals[idx * 3 + 1],
        m_normals[idx * 3 + 2]);
}

bool Triangle::Intersect(Ray& ray, HitRecord& hitRec) const
{    
    Float3 p0 = m_mesh->GetVertex(m_triangleIdx, 0);
    Float3 p1 = m_mesh->GetVertex(m_triangleIdx, 1);
    Float3 p2 = m_mesh->GetVertex(m_triangleIdx, 2);

    /* Find vectors for two edges sharing v[0] */
    Float3 edge1 = p1 - p0, edge2 = p2 - p0;

    /* Begin calculating determinant - also used to calculate U parameter */
    Float3 pvec = Cross(ray.d,edge2);

    /* If determinant is near zero, ray lies in plane of triangle */
    float det = Dot(edge1,pvec);

    if (det > -1e-8f && det < 1e-8f)
        return false;
    float inv_det = 1.0f / det;

    /* Calculate distance from v[0] to ray origin */
    Float3 tvec = ray.o - p0;

    /* Calculate U parameter and test bounds */
    float u = Dot(tvec, pvec) * inv_det;
    if (u < 0.0 || u > 1.0)
        return false;

    /* Prepare to test V parameter */
    Float3 qvec = Cross(tvec, edge1);

    /* Calculate V parameter and test bounds */
    float v = Dot(ray.d, qvec) * inv_det;
    if (v < 0.0 || u + v > 1.0)
        return false;

    /* Ray intersects triangle -> compute t */
    float t = Dot(edge2, qvec) * inv_det;

    if (t >= ray.tMin && t <= ray.tMax) {
        hitRec.m_t = t;
        hitRec.m_geoRec.m_uv = Float2(u, v);
        return true;
    }
    else {
        return false;
    }
}

void Triangle::SetGeometryRecord(GeometryRecord& geoRec) const
{
    Float3 p0 = m_mesh->GetVertex(m_triangleIdx, 0);
    Float3 p1 = m_mesh->GetVertex(m_triangleIdx, 1);
    Float3 p2 = m_mesh->GetVertex(m_triangleIdx, 2);

    Float3 bary(1 - geoRec.m_uv.x - geoRec.m_uv.y, geoRec.m_uv.x, geoRec.m_uv.y);
    geoRec.m_p = bary.x * p0 + bary.y * p1 + bary.z * p2;
    if (m_mesh->m_texcoordNum > 0) {
        Float2 st0 = m_mesh->GetTexcoord(m_triangleIdx, 0);
        Float2 st1 = m_mesh->GetTexcoord(m_triangleIdx, 1);
        Float2 st2 = m_mesh->GetTexcoord(m_triangleIdx, 2);
        geoRec.m_st = bary.x * st0 + bary.y * st1 + bary.z * st2;
    }
    else {
        geoRec.m_st = geoRec.m_uv;
    }

    Float3 dir = Cross(p1 - p0, p2 - p0);
    geoRec.m_pdf = 1.f / Length(dir);
    geoRec.m_ng = Normalize(dir);
    if (m_mesh->m_normalNum > 0) {
        Float3 n0 = m_mesh->GetNormal(m_triangleIdx, 0);
        Float3 n1 = m_mesh->GetNormal(m_triangleIdx, 1);
        Float3 n2 = m_mesh->GetNormal(m_triangleIdx, 2);
        geoRec.m_ns = Normalize(bary.x * n0 + bary.y * n1 + bary.z * n2);
    }
    else {
        geoRec.m_ns = geoRec.m_ng;
    }
}

void Triangle::Sample(GeometryRecord& geoRec, Float2& s) const
{
    geoRec.m_uv = SampleUniformTriangle(s);
    SetGeometryRecord(geoRec);
}

float Triangle::Pdf(GeometryRecord& geoRec) const
{
    return geoRec.m_pdf = 1.f / Area();
}

float Triangle::Area() const
{
    Float3 p0 = m_mesh->GetVertex(m_triangleIdx, 0);
    Float3 p1 = m_mesh->GetVertex(m_triangleIdx, 1);
    Float3 p2 = m_mesh->GetVertex(m_triangleIdx, 2);

    return Length(Cross(p1 - p0, p2 - p0));
}
