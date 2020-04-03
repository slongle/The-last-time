#pragma once

#include "core/primitive.h"

struct Mesh {
    Mesh(
        const uint32_t& vertexNum,
        const uint32_t& texcoordNum,
        const uint32_t& normalNum,
        const uint32_t& triangleNum,
        float* vertices,
        float* texcoords,
        float* normals,
        int* indices)
        : m_vertexNum(vertexNum), m_texcoordNum(texcoordNum),
        m_normalNum(normalNum), m_triangleNum(triangleNum),
        m_vertices(vertices), m_texcoords(texcoords), 
        m_normals(normals), m_indices(indices) {};
    ~Mesh() {
        delete[] m_vertices;
        delete[] m_normals;
        delete[] m_texcoords;
        delete[] m_indices;
    }

    Float3 GetVertex(const uint32_t& triangleIdx, const uint32_t& vertexIdx) const;
    Float2 GetTexcoord(const uint32_t& triangleIdx, const uint32_t& vertexIdx) const;
    Float3 GetNormal(const uint32_t& triangleIdx, const uint32_t& vertexIdx) const;

    uint32_t m_vertexNum, m_texcoordNum, m_normalNum, m_triangleNum;
    float* m_vertices;
    float* m_normals;
    float* m_texcoords;
    int* m_indices;
};

class Triangle : public Shape{
public:
    Triangle(const std::shared_ptr<Mesh>& mesh, const uint32_t& idx) 
        :m_mesh(mesh), m_triangleIdx(idx) {}

    bool Intersect(Ray& ray, HitRecord& hitRec) const;
    void SetGeometryRecord(GeometryRecord& geoRec) const;
    void Sample(GeometryRecord& geoRec, Float2& s) const;
    float Pdf(GeometryRecord& geoRec) const;
    float Area() const;

    uint32_t m_triangleIdx;
    std::shared_ptr<Mesh> m_mesh;
};

std::shared_ptr<Mesh> LoadMesh(const std::string& filename);