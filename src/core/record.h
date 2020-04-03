#pragma once

#include "vector.h"
#include "sampling.h"
#include "camera.h"

class Primitive;

class GeometryRecord {
public:
    Float2 m_uv;
    Float3 m_p;
    Float2 m_st;
    Float3 m_ng, m_ns;
    float m_pdf;
};

class HitRecord {
public:

    float m_t;
    GeometryRecord m_geoRec;
    const Primitive* m_primitive = nullptr;
};

class MaterialRecord {
public:
    MaterialRecord() {}
    MaterialRecord(const Float3& wi, const Float3& ns, const Float2& st)
        :m_frame(ns), m_wi(m_frame.ToLocal(wi)), m_st(st) {}

    Float3 ToLocal(const Float3& v) const { return m_frame.ToLocal(v); }
    Float3 ToWorld(const Float3& v) const { return m_frame.ToWorld(v); }

    Frame m_frame;
    Float2 m_st;

    Float3 m_wo, m_wi;
    float m_pdf;

};

class LightRecord {
public:
    LightRecord() {}
    LightRecord(const Float3& _ref)
        :m_ref(_ref) {}
    LightRecord(const Float3& _ref, const GeometryRecord& _geoRec) 
        :m_ref(_ref), m_geoRec(_geoRec) {}

    Float3 m_ref;

    GeometryRecord m_geoRec;
    Float3 m_wi;
    float m_pdf;
    Ray m_shadowRay;
};